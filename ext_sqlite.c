/** \file ext_sqlite.c

\brief Lexicon for interacting with sqlite3

*/

const gchar *sql_execute(sqlite3* connection, const gchar *query) {
    char* result = NULL;
    sqlite3_exec(connection, query, NULL, NULL, &result);
    return result;
}


static int append_record_cb(gpointer gp_records, int num_cols, char **values, char **cols) {
    GSequence *records = gp_records;

    GHashTable *record_new = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    for (gint i=0; i < num_cols; i++) {
        g_hash_table_insert(record_new, (gpointer) g_strdup(cols[i]), (gpointer) g_strdup(values[i]));
    }

    g_sequence_append(records, record_new);

    return 0;
}


static void free_hash_table(gpointer gp_hash) {
    g_hash_table_destroy(gp_hash);
}

GSequence *sql_select(sqlite3* connection, const gchar *query, gchar **error_message_p) {
    GSequence *result = g_sequence_new(free_hash_table);
    sqlite3_exec(connection, query, append_record_cb, result, error_message_p);

    return result;
}

// -----------------------------------------------------------------------------
/** Pops a db filename, opens an sqlite3 connection to it, and pushes the
connection onto the stack.

*/
// -----------------------------------------------------------------------------
static void EC_sqlite3_open(gpointer gp_entry) {
    Param *db_file = pop_param();

    sqlite3 *connection;
    int sqlite_status = sqlite3_open(db_file->val_string, &connection);
    if (sqlite_status != SQLITE_OK) {
        handle_error(ERR_GENERIC_ERROR);
        fprintf(stderr, "-----> sqlite3_open failed\n");
        return;
    }
    Param *param_new = new_custom_param(connection, "sqlite3 connection");
    push_param(param_new);

    free_param(db_file);
}



// -----------------------------------------------------------------------------
/** Pops a database connection and closes it.
*/
// -----------------------------------------------------------------------------
static void EC_sqlite3_close(gpointer gp_entry) {
    Param *param_connection = pop_param();

    sqlite3 *connection = param_connection->val_custom;

    int sqlite_status = sqlite3_close(connection);
    if (sqlite_status != SQLITE_OK) {
        handle_error(ERR_GENERIC_ERROR);
        fprintf(stderr, "-----> sqlite3_close failed\n");
        return;
    }

    free_param(param_connection);
}



// -----------------------------------------------------------------------------
/** Pops a database connection, gets the last inserted row ID, and pushes it onto the stack
*/
// -----------------------------------------------------------------------------
static void EC_sqlite3_last_id(gpointer gp_entry) {
    Param *param_connection = pop_param();
    sqlite3 *connection = param_connection->val_custom;

    gint64 id = sqlite3_last_insert_rowid(connection);

    Param *param_new = new_int_param(id);
    push_param(param_new);

    free_param(param_connection);
}



// -----------------------------------------------------------------------------
/** Defines sqlite3 lexicon and adds it to the dictionary.


The following words are defined:

- sqlite3-open (db-name -- db-connection) Opens a connection to a database
- sqlite3-close (db-connection -- ) Closes a connection to a database
- sqlite3-last-id (-- id) Pushes most recent row ID

*/
// -----------------------------------------------------------------------------
void EC_add_sqlite_lexicon(gpointer gp_entry) {
    add_entry("sqlite3-open")->routine = EC_sqlite3_open;
    add_entry("sqlite3-close")->routine = EC_sqlite3_close;
    add_entry("sqlite3-last-id")->routine = EC_sqlite3_last_id;
}
