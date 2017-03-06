/** \file param.c

\brief Functions for creating and destroying Param objects.

A Param can be pushed onto the parameter stack. They can also be added to the
params array of an Entry.

See \ref param_types "Param types" for a description of each type of parameter.
*/

static GHashTable *_custom_print_functions = NULL;

// -----------------------------------------------------------------------------
/** Creates a new Param.

\returns newly allocated Param
*/
// -----------------------------------------------------------------------------
Param *new_param() {
    Param *result = g_new(Param, 1);
    result->type = '?';
    result->val_string = NULL;
    result->val_custom = NULL;
    return result;
}



// -----------------------------------------------------------------------------
/** Creates a new int-valued Param.

\param val_int: Value of the Param being created
\returns newly allocated Param with the specified int value
*/
// -----------------------------------------------------------------------------
Param *new_int_param(gint64 val_int) {
    Param *result = new_param();
    result->type = 'I';
    result->val_int = val_int;
    return result;
}



// -----------------------------------------------------------------------------
/** Creates a new double-valued Param.

\param val_double: Value of the Param being created
\returns newly allocated Param with the specified double value
*/
// -----------------------------------------------------------------------------
Param *new_double_param(gdouble val_double) {
    Param *result = new_param();
    result->type = 'D';
    result->val_double = val_double;
    return result;
}



// -----------------------------------------------------------------------------
/** Creates a new string-valued Param

\param str: String to copy
*/
// -----------------------------------------------------------------------------
Param *new_str_param(const gchar *str) {
    Param *result = new_param();
    result->type = 'S';
    result->val_string = g_strdup(str);
    return result;
}



// -----------------------------------------------------------------------------
/** Creates a new routine-valued Param.

\param val_routine: Value of the Param being created
\returns newly allocated Param with the specified routine value
*/
// -----------------------------------------------------------------------------
Param *new_routine_param(routine_ptr val_routine) {
    Param *result = new_param();
    result->type = 'R';
    result->val_routine = val_routine;
    return result;
}



// -----------------------------------------------------------------------------
/** Creates a new entry-valued Param.

\param val_entry: Value of the Param being created
\returns newly allocated Param with the specified entry value

This is typically used when creating new dictionary entries.
*/
// -----------------------------------------------------------------------------
Param *new_entry_param(Entry *val_entry) {
    Param *result = new_param();
    result->type = 'E';
    result->val_entry = val_entry;
    return result;
}



// -----------------------------------------------------------------------------
/** Creates a new pseudoentry-valued Param.

\param val_routine: Value of the Param being created
\returns newly allocated Param with the specified pseudoentry value

This is used during the compilation of definitions. Pseudo entries are used
to implement branching during a definition as well as doing things like pushing
constants from a definition onto the param stack.
*/
// -----------------------------------------------------------------------------
Param *new_pseudo_entry_param(const gchar *word, routine_ptr routine) {
    Param *result = new_param();
    result->type = 'P';
    g_strlcpy(result->val_pseudo_entry.word, word, MAX_WORD_LEN);
    result->val_pseudo_entry.routine = routine;
    result->val_pseudo_entry.params = g_sequence_new(free_param);
    return result;
}



// -----------------------------------------------------------------------------
/** Creates a new custom-data valued Param

\param val_custom: Custom data for Param
\returns newly allocated Param with the specified entry value

*/
// -----------------------------------------------------------------------------
Param *new_custom_param(gpointer val_custom, const gchar *comment, free_ptr free_custom) {
    Param *result = new_param();
    result->type = 'C';
    result->val_custom = val_custom;
    result->free_custom = free_custom;
    g_strlcpy(result->val_custom_comment, comment, MAX_WORD_LEN);
    return result;
}



// -----------------------------------------------------------------------------
/** Copies fields of Param to another Param

\note The string value is duplicated so that the destination Param can be freed
      independently of the source Param.
*/
// -----------------------------------------------------------------------------
void copy_param(Param *dst, const Param *src) {
    *dst = *src;

    // Make a copy of the string since the dst needs to own it
    dst->val_string = g_strdup(src->val_string);
}

void create_print_functions() {
    _custom_print_functions = g_hash_table_new(g_str_hash, g_str_equal);
}



void add_print_function(const gchar *type_name, print_param_func func) {
    g_hash_table_insert(_custom_print_functions, (gpointer) type_name, (gpointer) func);
}



void destroy_print_functions() {
    g_hash_table_destroy(_custom_print_functions);
}


static void print_custom_param(FILE *file, Param* param) {
    print_param_func p_func = g_hash_table_lookup(_custom_print_functions, param->val_custom_comment);
    if (!p_func) {
        fprintf(file, "Custom param (%s)\n", param->val_custom_comment);
    }
    else {
        p_func(file, param);
    }
}

// -----------------------------------------------------------------------------
/** Prints a parameter to a file and with a prefix

\param param: Param to print
\param file: Output file
\param prefi: Prefix used when outputting string

*/
// -----------------------------------------------------------------------------
void print_param(FILE *file, Param *param) {
    Entry *entry;

    if (!param) {
        fprintf(file, "NULL param\n");
        return;
    }

    switch (param->type) {
        case 'I':
            fprintf(file, "%ld\n", param->val_int);
            break;

        case 'D':
            fprintf(file, "%lf\n", param->val_double);
            break;

        case 'S':
            fprintf(file, "\"%s\"\n", param->val_string);
            break;

        case 'E':
            entry = param->val_entry;
            fprintf(file, "Entry: %s\n", entry->word);
            break;

        case 'R':
            fprintf(file, "Routine: %ld\n", (gint64) param->val_routine);
            break;

        case 'P':
            fprintf(file, "Pseudo-entry: %s\n", param->val_pseudo_entry.word);
            break;

        case 'C':
            print_custom_param(file, param);
            break;

        default:
            fprintf(file, "%c: %s\n", param->type, "Unknown type");
            break;
    }
}



// -----------------------------------------------------------------------------
/** Frees memory for a param.

\param gp_param: Pointer to a Param to free.

For custom data

*/
// -----------------------------------------------------------------------------
void free_param(gpointer gp_param) {
    Param *param = gp_param;

    if (!param) {
        return;
    }

    if (param->type == 'S') {
        g_free(param->val_string);
    }
    else if (param->type == 'P') {
        g_sequence_free(param->val_pseudo_entry.params);
    }
    else if (param->type == 'C') {
        param->free_custom(param->val_custom);
    }

    g_free(param);
}



// -----------------------------------------------------------------------------
/** No-op free function used to do nothing for custom data.

\note If memory should be freed, the 'free_custom' function should be set to
the appropriate function.
*/
// -----------------------------------------------------------------------------
void free_nop(gpointer param) {
    return;
}
