/** \file ext_tasks.c

\brief Lexicon for tasks

The schema of the tasks.db is:

- CREATE TABLE tasks(is_done INTEGER, id INTEGER PRIMARY KEY, name TEXT, value REAL);
- CREATE TABLE parent_child(parent_id INTEGER, child_id INTEGER);
- CREATE TABLE task_notes(task_id INTEGER, note_id INTEGER);

*/

#define MAX_NAME_LEN   256  /**< \brief Length of string to hold task names */

#define TREE_TEE     "├"
#define TREE_VERT    "│"
#define TREE_END     "└"
#define TREE_HORIZ   "─"

#define SELECT_TASKS_PHRASE "select id, pc.parent_id as parent_id, name, is_done, value " \
                            "from tasks inner join parent_child as pc on pc.child_id=id "

// -----------------------------------------------------------------------------
/** Represents a note from a database record
*/
// -----------------------------------------------------------------------------
typedef struct {
    gint64 id;
    gint64 parent_id;
    gchar name[MAX_NAME_LEN];
    gboolean is_done;
    double value;              /**< \brief Used to rank tasks */
} Task;


static Task _root_task = {
    .id = 0,
    .parent_id = 0,
    .name = "_root_task",
    .is_done = 0,
    .value = 0
};


// -----------------------------------------------------------------------------
/** Gets a database connection from the "tasks-db" variable.
*/
// -----------------------------------------------------------------------------
static sqlite3 *get_db_connection() {
    execute_string("tasks-db @");
    Param *param_connection = pop_param();

    sqlite3 *result = param_connection->val_custom;

    free_param(param_connection);
    return result;
}



// -----------------------------------------------------------------------------
/** Creates a new copy of a Task.
*/
// -----------------------------------------------------------------------------
static Task *copy_task(Task *src) {
    if (!src) return NULL;

    Task *result = g_new(Task, 1);
    *result = *src;
    return result;
}


static void free_task(gpointer gp_task) {
    g_free(gp_task);
}

// -----------------------------------------------------------------------------
/** Adds a task to the tasks-db
*/
// -----------------------------------------------------------------------------
static void add_task(const gchar *name, gint64 parent_id) {
    gchar query[MAX_QUERY_LEN];
    snprintf(query, MAX_QUERY_LEN,
             "insert into tasks(name, is_done)"
             "values(\"%s\", 0)",
             name);

    sqlite3 *connection = get_db_connection();
    const char *error_message = sql_execute(connection, query);

    if (error_message) {
        handle_error(ERR_GENERIC_ERROR);
        fprintf(stderr, "-----> Problem storing task '%s' ==> %s\n", name, error_message);
    }

    gint64 task_id = sqlite3_last_insert_rowid(connection);
    snprintf(query, MAX_QUERY_LEN,
             "insert into parent_child(parent_id, child_id)"
             "values(%ld, %ld)",
             parent_id, task_id);

    error_message = sql_execute(get_db_connection(), query);

    if (error_message) {
        handle_error(ERR_GENERIC_ERROR);
        fprintf(stderr, "-----> Problem adding parent (%ld) child (%ld) ==> %s\n", parent_id, task_id, error_message);
    }
}


static void set_cur_task_id(gint64 task_id) {
    gchar str[MAX_QUERY_LEN];
    snprintf(str, MAX_QUERY_LEN, "%ld cur-task-id !", task_id);
    execute_string(str);
}


static gint64 get_cur_task_id() {
    execute_string("cur-task-id @");
    Param *param_id = pop_param();
    gint64 result = param_id->val_int;
    free_param(param_id);
    return result;
}



static Task *record_to_task(GHashTable *record) {
    Task task = {
        .id = STR_TO_INT(g_hash_table_lookup(record, "id")),
        .parent_id = STR_TO_INT(g_hash_table_lookup(record, "parent_id")),
        .is_done = STR_TO_INT(g_hash_table_lookup(record, "is_done")),
        .value = STR_TO_DOUBLE(g_hash_table_lookup(record, "value"))
    };

    g_strlcpy(task.name, g_hash_table_lookup(record, "name"), MAX_NAME_LEN);

    Task *result = copy_task(&task);
    return result;
}



static GSequence *select_tasks(const gchar *sql_query) {
    sqlite3 *connection = get_db_connection();

    char *error_message = NULL;
    GSequence *records = sql_select(connection, sql_query, &error_message);

    GSequence *result = g_sequence_new(NULL);
    if (error_message) {
        handle_error(ERR_GENERIC_ERROR);
        fprintf(stderr, "-----> Problem executing 'select_tasks'\n----->%s", error_message);
        goto done;
    }

    for (GSequenceIter *iter = g_sequence_get_begin_iter(records);
         !g_sequence_iter_is_end(iter);
         iter = g_sequence_iter_next(iter)) {

        GHashTable *record = g_sequence_get(iter);
        Task *task = record_to_task(record);
        Param *param_new = new_custom_param(task, "Task", free_task);
        g_sequence_append(result, param_new);
    }

done:
    g_sequence_free(records);

    return result;
}




// -----------------------------------------------------------------------------
/** Helper function to print a task line out

Here's a sample:

   "( ) 21: Compute effort for a task using notes (50.0)"
*/
// -----------------------------------------------------------------------------
static void print_task_line(FILE *file, Task *task) {
    if (task->is_done) {
        fprintf(file, "(X)");
    }
    else {
        fprintf(file, "( )");
    }

    if (task->id == get_cur_task_id()) {
        fprintf(file, "*");
    }
    else {
        fprintf(file, " ");
    }

    fprintf(file, "%ld: %s (%.1lf)\n", task->id,
                                       task->name,
                                       task->value);
}




// -----------------------------------------------------------------------------
/** Links a task and a note together

(task-id note-id -- )
*/
// -----------------------------------------------------------------------------
static void EC_link_note(gpointer gp_entry) {
    Param *param_note_id = pop_param();
    gint64 note_id = param_note_id->val_int;
    free_param(param_note_id);

    Param *param_task_id = pop_param();
    gint64 task_id = param_task_id->val_int;
    free_param(param_task_id);

    gchar query[MAX_QUERY_LEN];
    snprintf(query, MAX_QUERY_LEN,
             "insert into task_notes(task_id, note_id) "
             "values(%ld, %ld)",
             task_id, note_id);

    sqlite3 *connection = get_db_connection();
    const char *error_message = sql_execute(connection, query);

    if (error_message) {
        handle_error(ERR_GENERIC_ERROR);
        fprintf(stderr, "-----> Problem executing 'link-note'\n----->%s", error_message);
    }
}





static void define_open_db() {
    execute_string(": open-db   'tasks.db' sqlite3-open tasks-db ! "
                   "             tasks-db @ notes-db ! ;");
}



static void define_close_db() {
    execute_string(": close-db   tasks-db @ sqlite3-close ;");
}



/** Pushes a task by ID onto the stack.

(id -- Task)
*/
static void EC_get_task(gpointer gp_entry) {
    Task *task = NULL;
    Param *param_id = pop_param();    
    gchar query[MAX_QUERY_LEN];

    if (param_id->val_int == 0) {
        task = copy_task(&_root_task);
        push_param(new_custom_param(task, "Task", free_task));
    }
    else {
        snprintf(query, MAX_QUERY_LEN, "%s where id = %ld", SELECT_TASKS_PHRASE, param_id->val_int);
        GSequence *records = select_tasks(query);
        if (g_sequence_get_length(records) != 1) {
            handle_error(ERR_GENERIC_ERROR);
            fprintf(stderr, "-----> Problem executing 'get_task'\n");
            goto done;
        }
        Param *param_task = g_sequence_get(g_sequence_get_begin_iter(records));
        push_param(param_task);
    }

done:
    free_param(param_id);
}

static void print_seq_tasks(FILE *file, Param *param) {
    GSequence *tasks = param->val_custom;
    for (GSequenceIter *iter = g_sequence_get_begin_iter(tasks);
         !g_sequence_iter_is_end(iter);
         iter = g_sequence_iter_next(iter)) {

        Param *param_task = g_sequence_get(iter);
        print_task_line(file, param_task->val_custom);
    }
}


static void print_task(FILE *file, Param *param) {
    Task *task = param->val_custom;

    if (!task->id) {
        fprintf(file, "%s\n", task->name);
    }
    else {
        print_task_line(file, task);
    }
}

// -----------------------------------------------------------------------------
/** Creates a subtask of the specified task

(task name -- )
*/
// -----------------------------------------------------------------------------
static void EC_add_subtask(gpointer gp_entry) {
    Param *param_task_name = pop_param();

    Param *param_parent_task = pop_param();
    Task *parent_task = param_parent_task->val_custom;

    add_task(param_task_name->val_string, parent_task->id);

    free_param(param_task_name);
    free_param(param_parent_task);
}


/**
( -- [Task])
*/
static void EC_all(gpointer gp_entry) {
    GSequence *records = select_tasks(SELECT_TASKS_PHRASE);
    push_param(new_custom_param(records, "[Task]", free_param_seq));
}


/**
(Task field-name -- Task value)
*/
static void EC_get_field(gpointer gp_entry) {
    Param *param_field_name = pop_param();
    const Param *param_task = top();
    Task *task = param_task->val_custom;

    gchar *field_name = param_field_name->val_string;

    if (STR_EQ(field_name, "id")) {
        push_param(new_int_param(task->id));
    }
    else if (STR_EQ(field_name, "parent_id")) {
        push_param(new_int_param(task->parent_id));
    }
    else {
        handle_error(ERR_GENERIC_ERROR);
        fprintf(stderr, "-----> Unknown Task field: %s\n", field_name);
    }

    free_param(param_field_name);
}

/** Updates cur-task-id
G: (task -- )
g: (task-id -- )
*/
static void define_g() {
    execute_string(": G    'id' @field  cur-task-id ! ;");
    execute_string(": g    T G ;");
}


// -----------------------------------------------------------------------------
/** Defines the tasks lexicon.

The following words are defined for manipulating Tasks:

### Add tasks
- + (string -- ) 
- ++ (string -- ) Creates a subtask of the specified task

- link-note (note-id -- ) Connects the current task with the specified note

### Misc
- tasks-db - This holds the sqlite database connection for tasks
- last-active-id ( -- task-id ) Pushes last active task ID onto the stack

*/
// -----------------------------------------------------------------------------
void EC_add_tasks_lexicon(gpointer gp_entry) {
    // Add the lexicons that this depends on
    execute_string("lex-sequence");
    execute_string("lex-sqlite");
    execute_string("lex-notes");
    execute_string("lex-trees");

    add_variable("tasks-db");

    // Holds the current task
    add_variable("cur-task-id");
    set_cur_task_id(0);

    add_entry("all")->routine = EC_all;
    add_entry("T")->routine = EC_get_task;

    add_entry("++")->routine = EC_add_subtask;

    add_entry("link-note")->routine = EC_link_note;

    add_entry("@field")->routine = EC_get_field;

    add_print_function("[Task]", print_seq_tasks);
    add_print_function("Task", print_task);

    define_open_db();
    define_close_db();
    define_g();
}
