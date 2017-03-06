/** \file ext_notes.c

\brief Lexicon for notes

The associated schema of a notes.db is this:

- CREATE TABLE notes(type TEXT, id INTEGER PRIMARY KEY, note TEXT, timestamp TEXT, date TEXT);

*/

#define MAX_ELAPSED_LEN 16    /**< \brief Max length of an elapsed minutes string */

#define SELECT_NOTES_PHRASE  "select id, type, note, timestamp, date from notes "



// -----------------------------------------------------------------------------
/** Represents a note from a database record
*/
// -----------------------------------------------------------------------------
typedef struct {
    gint64 id;     /**< \brief Record ID */
    gchar type;    /**< \brief 'S', 'M', 'E', or 'N'  */
    gchar *note;   /**< \brief Text of note */

    gchar timestamp_text[MAX_TIMESTAMP_LEN];  /**< Note timestamp string */
    gchar date_text[MAX_TIMESTAMP_LEN];       /**< Note date string */
    struct tm timestamp;                      /**< Timestamp as tm struct */
} Note;

static Note *_current_start_note = NULL;


// -----------------------------------------------------------------------------
/** Creates a new copy of a note
*/
// -----------------------------------------------------------------------------
static Note *copy_note(Note *src) {
    Note *result = g_new(Note, 1);
    *result = *src;
    result->note = g_strdup(src->note);

    // Recompute timestamp
    struct tm *timestamp = getdate(result->timestamp_text);
    if (!timestamp) {
        handle_error(ERR_GENERIC_ERROR);
        fprintf(stderr, "----->Unable to parse timestamp: getdate_err: %d\n", getdate_err);
    }
    result->timestamp = *timestamp;
    return result;
}



// -----------------------------------------------------------------------------
/** Frees an allocated Note.
*/
// -----------------------------------------------------------------------------
static void free_note(gpointer gp_note) {
    Note *note = gp_note;
    g_free(note->note);
    g_free(note);
}


// -----------------------------------------------------------------------------
/** Computes the elapsed minutes between two time_t structs.
*/
// -----------------------------------------------------------------------------
static gint64 elapsed_min(time_t time_l, time_t time_r) {
    double delta_s = difftime(time_l, time_r);
    gint64 result = ceil(delta_s/60.0);
    return result;
}


// -----------------------------------------------------------------------------
/** Returns the number of minutes between two notes.
*/
// -----------------------------------------------------------------------------
static gint64 get_minute_difference(Note *note_l, Note *note_r) {
    time_t time_l = mktime(&note_l->timestamp);
    time_t time_r = mktime(&note_r->timestamp);
    return elapsed_min(time_l, time_r);
}



// -----------------------------------------------------------------------------
/** Writes the elapsed minutes between two notes into a char buffer.
*/
// -----------------------------------------------------------------------------
static void write_elapsed_minutes(gchar *dst, gint64 len, Note *note_l, Note *note_r) {
    if (!note_r) {
        g_strlcpy(dst, "?", len);
    }
    else {
        gint64 num_minutes = get_minute_difference(note_l, note_r);
        snprintf(dst, len, "%ld", num_minutes);
    }
}


static void set_current_start_note(Note *src) {
    if (_current_start_note) {
        free_note(_current_start_note);
    }
    _current_start_note = src ? copy_note(src) : NULL;
}


static void print_note(FILE *file, Param *param) {
    Note *note = param->val_custom;

    gchar elapsed_min_text[MAX_ELAPSED_LEN];

    switch(note->type) {
        case 'N':
            printf("%s - %ld\n%s\n\n", note->timestamp_text, note->id, note->note);
            break;

        case 'S':
            set_current_start_note(note);
            printf("\n>> %s - %ld\n%s\n\n", note->timestamp_text, note->id, note->note);
            break;

        case 'M':
            write_elapsed_minutes(elapsed_min_text, MAX_ELAPSED_LEN, note, _current_start_note);
            printf("(%s min) %s - %ld\n%s\n\n", elapsed_min_text, note->timestamp_text, note->id, note->note);
            break;

        case 'E':
            write_elapsed_minutes(elapsed_min_text, MAX_ELAPSED_LEN, note, _current_start_note);
            printf("<< (%s min) %s - %ld\n%s\n\n", elapsed_min_text, note->timestamp_text, note->id, note->note);
            set_current_start_note(NULL);
            break;

        default:
            printf("TODO: Format this:\n--> %s\n\n", note->note);
            break;
    }

    free_param(param);
}


static void print_seq_notes(FILE *file, Param *param) {
    GSequence *sequence = param->val_custom;

    for (GSequenceIter *iter = g_sequence_get_begin_iter(sequence);
         !g_sequence_iter_is_end(iter);
         iter = g_sequence_iter_next(iter)) {

        Param *p = g_sequence_get(iter);
        print_param(file, p);
    }
}



// -----------------------------------------------------------------------------
/** Gets a database connection from the "notes-db" variable.
*/
// -----------------------------------------------------------------------------
static sqlite3 *get_db_connection() {
    execute_string("notes-db @");
    Param *param_connection = pop_param();

    sqlite3 *result = param_connection->val_custom;

    free_param(param_connection);
    return result;
}



// I think we can get a generic sequence of records. The elements will be
// hash tables with column names as keys and text values.



Note *record_to_note(GHashTable *record) {
    gchar *type = g_hash_table_lookup(record, "type");

    Note note = {
        .id = STR_TO_INT(g_hash_table_lookup(record, "id")),
        .type = type ? type[0] : '?',
        .note = g_hash_table_lookup(record, "note")
    };

    g_strlcpy(note.timestamp_text, g_hash_table_lookup(record, "timestamp"), MAX_TIMESTAMP_LEN);
    g_strlcpy(note.date_text, g_hash_table_lookup(record, "date"), MAX_TIMESTAMP_LEN);

    Note *result = copy_note(&note);
    return result;
}


static GSequence *select_notes(const gchar *sql_query) {
    sqlite3 *connection = get_db_connection();

    char *error_message = NULL;
    GSequence *records = sql_select(connection, sql_query, &error_message);

    GSequence *result = g_sequence_new(NULL);
    if (error_message) {
        handle_error(ERR_GENERIC_ERROR);
        fprintf(stderr, "-----> Problem executing 'select_notes'\n----->%s", error_message);
        goto done;
    }

    for (GSequenceIter *iter = g_sequence_get_begin_iter(records);
         !g_sequence_iter_is_end(iter);
         iter = g_sequence_iter_next(iter)) {

        GHashTable *record = g_sequence_get(iter);
        Note *note = record_to_note(record);
        Param *param_new = new_custom_param(note, "Note", free_note);
        g_sequence_append(result, param_new);
    }


done:
    g_sequence_free(records);

    return result;
}



// -----------------------------------------------------------------------------
/** Helper function to write notes to the database.
*/
// -----------------------------------------------------------------------------
static void store_note(const gchar *type) {
    Param *param_note = pop_param();

    gchar query[MAX_QUERY_LEN];
    snprintf(query, MAX_QUERY_LEN, "insert into notes(note, type, timestamp, date)"
                                   "values(\"%s\", '%s', datetime('now', 'localtime'), date('now', 'localtime'))",
                                   param_note->val_string,
                                   type);

    const char* error_message = sql_execute(get_db_connection(), query);

    if (error_message) {
        handle_error(ERR_GENERIC_ERROR);
        fprintf(stderr, "-----> Problem storing '%s' note ==> %s\n", type, error_message);
    }

    free_param(param_note);
}



// -----------------------------------------------------------------------------
/** Stores a start note in the notes database.
*/
// -----------------------------------------------------------------------------
static void EC_start_chunk(gpointer gp_entry) {
    store_note("S");
}



// -----------------------------------------------------------------------------
/** Stores a middle note in the notes database.
*/
// -----------------------------------------------------------------------------
static void EC_middle_chunk(gpointer gp_entry) {
    store_note("M");
}



// -----------------------------------------------------------------------------
/** Stores an end note in the notes database.
*/
// -----------------------------------------------------------------------------
static void EC_end_chunk(gpointer gp_entry) {
    store_note("E");
}



// -----------------------------------------------------------------------------
/** Stores a generic note in the notes database.
*/
// -----------------------------------------------------------------------------
static void EC_generic_note(gpointer gp_entry) {
    store_note("N");
}



// -----------------------------------------------------------------------------
/** Gets all notes for today and pushes onto stack
*/
// -----------------------------------------------------------------------------
static void EC_notes_today(gpointer gp_entry) {
    gchar query[MAX_QUERY_LEN];
    snprintf(query, MAX_QUERY_LEN, "%s where date = date('now', 'localtime')", SELECT_NOTES_PHRASE);

    GSequence *records = select_notes(query);
    Param *param_new = new_custom_param(records, "[Note]", free_param_seq);
    push_param(param_new);
}



// -----------------------------------------------------------------------------
/** Returns the most recent Note with type 'S'
*/
// -----------------------------------------------------------------------------
static Note *get_latest_S_note() {
    gchar query[MAX_QUERY_LEN];
    snprintf(query, MAX_QUERY_LEN, "%s where type = 'S' order by id desc limit 1", SELECT_NOTES_PHRASE);

    GSequence *records = select_notes(query);

    Note *result = NULL;
    if (g_sequence_get_length(records) == 1) {
        Param *param_note = g_sequence_get(g_sequence_get_begin_iter(records));
        result = copy_note(param_note->val_custom);
        free_param(param_note);
    }

    // Cleanup
    g_sequence_free(records);

    return result;
}



// -----------------------------------------------------------------------------
/** Returns most recent note with type 'S' or 'E'.
*/
// -----------------------------------------------------------------------------
static Note *get_latest_SE_note() {
    gchar query[MAX_QUERY_LEN];
    snprintf(query, MAX_QUERY_LEN, "%s where type = 'S' or type = 'E' order by id desc limit 1", SELECT_NOTES_PHRASE);

    GSequence *records = select_notes(query);

    Note *result = NULL;
    if (g_sequence_get_length(records) == 1) {
        Param *param_note = g_sequence_get(g_sequence_get_begin_iter(records));
        result = copy_note(param_note->val_custom);
        free_param(param_note);
    }

    // Cleanup
    g_sequence_free(records);

    return result;
}



// -----------------------------------------------------------------------------
/** Prints the elapsed time since the last 'S' note.
*/
// -----------------------------------------------------------------------------
static void EC_time(gpointer gp_entry) {
    Note *note = get_latest_SE_note();

    if (!note) {
        printf("? min\n");
    }
    else {
        time_t start_note_time = mktime(&note->timestamp);
        time_t now = time(NULL);
        gint64 minutes = elapsed_min(now, start_note_time);
        printf("%ld min\n", minutes);
    }

    free_note(note);
}



// -----------------------------------------------------------------------------
/** 
*/
// -----------------------------------------------------------------------------
static void EC_notes_last_chunk(gpointer gp_entry) {
    gchar query[MAX_QUERY_LEN];
    Note *note = get_latest_S_note();
    GSequence *records = NULL;

    // If no starting note, then create an empty sequence of records
    if (!note) {
        records = g_sequence_new(NULL);
    }
    else {
        snprintf(query, MAX_QUERY_LEN, "%s where id >= %ld", SELECT_NOTES_PHRASE, note->id);
        records = select_notes(query);
        free_note(note);
    }

    Param *param_new = new_custom_param(records, "[Note]", free_param_seq);
    push_param(param_new);
}



// -----------------------------------------------------------------------------
/** Defines the notes lexicon

The following words are defined for manipulating notes:

- notes-db: This holds the sqlite database connection for notes

- S (string -- ) Creates a note that starts a work chunk
- M (string -- ) Creates a note in the middle of a work chunk
- E (string -- ) Creates a note that ends a work chunk
- N (string -- ) Creates a generic note

- time ( -- ) Prints the elapsed time since the last start or end note

- today-notes ( -- [notes from today])
- chunk-notes ( -- [notes from current chunk])
- print-notes ([notes] -- ) Prints notes
- note_ids-to-notes (Array[note ids] -- [notes])

*/
// -----------------------------------------------------------------------------
void EC_add_notes_lexicon(gpointer gp_entry) {
    // Add the lexicons that this depends on
    execute_string("lex-sqlite");

    add_variable("notes-db");

    add_entry("S")->routine = EC_start_chunk;
    add_entry("M")->routine = EC_middle_chunk;
    add_entry("N")->routine = EC_generic_note;
    add_entry("E")->routine = EC_end_chunk;

    add_entry("time")->routine = EC_time;

    add_entry("notes-today")->routine = EC_notes_today;
    add_entry("notes-last-chunk")->routine = EC_notes_last_chunk;

    add_print_function("[Note]", print_seq_notes);
    add_print_function("Note", print_note);
}
