/** \file ec_basic.h
*/

#pragma once

void add_basic_words();

void add_variable(const gchar *word);
void find_and_execute(const gchar *word);

int set_double_cb(gpointer gp_double_ref, int num_cols, char **values, char **cols);
int set_int_cb(gpointer gp_int_ref, int num_cols, char **values, char **cols);
int set_string_cb(gpointer gp_char_p_ref, int num_cols, char **values, char **cols);
void execute_string(const gchar *str);

void EC_push_param0(gpointer gp_entry);


#define EC_DB_STR_SETTER(_ec_func_name_, _word_, _db_table_name_, _field_name_) \
    static void _ec_func_name_(gpointer gp_entry) { \
        Param *param_value = pop_param(); \
        Param *param_obj_id = pop_param(); \
 \
        gint64 obj_id = param_obj_id->val_int; \
        free_param(param_obj_id); \
 \
        gchar id_str[MAX_ID_LEN]; \
        snprintf(id_str, MAX_ID_LEN, "%ld", obj_id); \
 \
        sqlite3 *connection = get_db_connection(); \
        gchar *sql = g_strconcat("update " _db_table_name_ " set " _field_name_ "=\"", param_value->val_string, "\" ", \
                                 "where id=", id_str, \
                                 NULL); \
        free_param(param_value); \
 \
        char *error_message = NULL; \
        sqlite3_exec(connection, sql, NULL, NULL, &error_message); \
        g_free(sql); \
 \
        if (error_message) { \
            handle_error(ERR_GENERIC_ERROR); \
            fprintf(stderr, "-----> Problem executing '" _word_ "'\n----->%s", error_message); \
        } \
    }


#define EC_DB_INT_SETTER(_ec_func_name_, _word_, _db_table_name_, _field_name_) \
    static void _ec_func_name_(gpointer gp_entry) { \
        Param *param_value = pop_param(); \
        Param *param_obj_id = pop_param(); \
 \
        gint64 value = 0.0; \
        value = param_value->val_int; \
        free_param(param_value); \
 \
        gint64 obj_id = param_obj_id->val_int; \
        free_param(param_obj_id); \
 \
        gchar id_str[MAX_ID_LEN]; \
        gchar value_str[MAX_INT_LEN]; \
        snprintf(id_str, MAX_ID_LEN, "%ld", obj_id); \
        snprintf(value_str, MAX_INT_LEN, "%ld", value); \
 \
        sqlite3 *connection = get_db_connection(); \
        gchar *sql = g_strconcat("update " _db_table_name_ " set " _field_name_ "=", value_str, " ", \
                                 "where id=", id_str, \
                                 NULL); \
 \
        char *error_message = NULL; \
        sqlite3_exec(connection, sql, NULL, NULL, &error_message); \
        g_free(sql); \
 \
        if (error_message) { \
            handle_error(ERR_GENERIC_ERROR); \
            fprintf(stderr, "-----> Problem executing '" _word_ "'\n----->%s", error_message); \
        } \
    }



#define EC_DB_DOUBLE_SETTER(_ec_func_name_, _word_, _db_table_name_, _field_name_) \
    static void _ec_func_name_(gpointer gp_entry) { \
        Param *param_value = pop_param(); \
        Param *param_obj_id = pop_param(); \
 \
        double value = 0.0; \
        if (param_value->type == 'I') { \
            value = param_value->val_int; \
        } \
        if (param_value->type == 'D') { \
            value = param_value->val_double; \
        } \
        free_param(param_value); \
 \
        gint64 obj_id = param_obj_id->val_int; \
        free_param(param_obj_id); \
 \
        gchar id_str[MAX_ID_LEN]; \
        gchar value_str[MAX_DOUBLE_LEN]; \
        snprintf(id_str, MAX_ID_LEN, "%ld", obj_id); \
        snprintf(value_str, MAX_DOUBLE_LEN, "%.1lf", value); \
 \
        sqlite3 *connection = get_db_connection(); \
        gchar *sql = g_strconcat("update " _db_table_name_ " set " _field_name_ "=", value_str, " ", \
                                 "where id=", id_str, \
                                 NULL); \
 \
        char *error_message = NULL; \
        sqlite3_exec(connection, sql, NULL, NULL, &error_message); \
        g_free(sql); \
 \
        if (error_message) { \
            handle_error(ERR_GENERIC_ERROR); \
            fprintf(stderr, "-----> Problem executing '" _word_ "'\n----->%s", error_message); \
        } \
    }


#define EC_DB_DOUBLE_GETTER(_ec_func_name_, _word_, _db_table_name_, _field_name_) \
    static void _ec_func_name_(gpointer gp_entry) { \
        sqlite3 *connection = get_db_connection(); \
     \
        Param *param_obj_id = pop_param(); \
        gint64 obj_id = param_obj_id->val_int; \
        free_param(param_obj_id); \
     \
        gchar id_str[MAX_ID_LEN]; \
        snprintf(id_str, MAX_ID_LEN, "%ld", obj_id); \
     \
        gchar *query = g_strconcat("select " \
                                   _field_name_ \
                                   " from " \
                                   _db_table_name_ \
                                   " where id=", id_str, NULL); \
     \
        double value_double; \
        char *error_message = NULL; \
        sqlite3_exec(connection, query, set_double_cb, &value_double, &error_message); \
        g_free(query); \
     \
        if (error_message) { \
            handle_error(ERR_GENERIC_ERROR); \
            fprintf(stderr, "-----> Problem executing '" _word_ "'\n----->%s", error_message); \
        } \
     \
        Param *param_new = new_double_param(value_double); \
        push_param(param_new); \
    }



#define EC_DB_INT_GETTER(_ec_func_name_, _word_, _db_table_name_, _field_name_) \
    static void _ec_func_name_(gpointer gp_entry) { \
        sqlite3 *connection = get_db_connection(); \
     \
        Param *param_obj_id = pop_param(); \
        gint64 obj_id = param_obj_id->val_int; \
        free_param(param_obj_id); \
     \
        gchar id_str[MAX_ID_LEN]; \
        snprintf(id_str, MAX_ID_LEN, "%ld", obj_id); \
     \
        gchar *query = g_strconcat("select " \
                                   _field_name_ \
                                   " from " \
                                   _db_table_name_ \
                                   " where id=", id_str, NULL); \
     \
        double value_double; \
        char *error_message = NULL; \
        sqlite3_exec(connection, query, set_double_cb, &value_double, &error_message); \
        g_free(query); \
     \
        if (error_message) { \
            handle_error(ERR_GENERIC_ERROR); \
            fprintf(stderr, "-----> Problem executing '" _word_ "'\n----->%s", error_message); \
        } \
     \
        Param *param_new = new_int_param(value_double); \
        push_param(param_new); \
    }


#define EC_DB_STR_GETTER(_ec_func_name_, _word_, _db_table_name_, _field_name_) \
    static void _ec_func_name_(gpointer gp_entry) { \
        sqlite3 *connection = get_db_connection(); \
     \
        Param *param_obj_id = pop_param(); \
        gint64 obj_id = param_obj_id->val_int; \
        free_param(param_obj_id); \
     \
        gchar id_str[MAX_ID_LEN]; \
        snprintf(id_str, MAX_ID_LEN, "%ld", obj_id); \
     \
        gchar *query = g_strconcat("select " \
                                   _field_name_ \
                                   " from " \
                                   _db_table_name_ \
                                   " where id=", id_str, NULL); \
     \
        gchar *value_string; \
        char *error_message = NULL; \
        sqlite3_exec(connection, query, set_string_cb, &value_string, &error_message); \
        g_free(query); \
     \
        if (error_message) { \
            handle_error(ERR_GENERIC_ERROR); \
            fprintf(stderr, "-----> Problem executing '" _word_ "'\n----->%s", error_message); \
        } \
     \
        Param *param_new = new_str_param(value_string); \
        g_free(value_string); \
        push_param(param_new); \
    }


#define EC_OBJ_FIELD_GETTER(_ec_func_name_, _Type_, _new_param_func_call_) \
    static void _ec_func_name_(gpointer gp_entry) { \
        const _Type_ *obj = top()->val_custom; \
        Param *param_new = _new_param_func_call_; \
        push_param(param_new); \
    }
