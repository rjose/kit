/** \file param.c

\brief Functions for creating and destroying Param objects.

A Param can be pushed onto the parameter stack. They can also be added to the
params array of an Entry.

See \ref param_types "Param types" for a description of each type of parameter.
*/


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
Param *new_custom_param(gpointer val_custom, const gchar *comment) {
    Param *result = new_param();
    result->type = 'C';
    result->val_custom = val_custom;
    g_strlcpy(result->val_custom_comment, comment, MAX_WORD_LEN);
    return result;
}



// -----------------------------------------------------------------------------
/** Copies fields of Param to another Param

\note The string value is duplicated so that the destination Param can be freed
      independently of the source Param.
*/
// -----------------------------------------------------------------------------
void copy_param(Param *dst, Param *src) {
    *dst = *src;

    // Make a copy of the string since the dst needs to own it
    dst->val_string = g_strdup(src->val_string);
}



// -----------------------------------------------------------------------------
/** Prints a parameter to a file and with a prefix

\param param: Param to print
\param file: Output file
\param prefi: Prefix used when outputting string

*/
// -----------------------------------------------------------------------------
void print_param(Param *param, FILE *file, const gchar *prefix) {
    Entry *entry;

    if (!param) {
        fprintf(file, "%sI: %s\n", prefix, "NULL param");
        return;
    }

    switch (param->type) {
        case 'I':
            fprintf(file, "%sI: %ld\n", prefix, param->val_int);
            break;

        case 'D':
            fprintf(file, "%sD: %lf\n", prefix, param->val_double);
            break;

        case 'S':
            fprintf(file, "%sS: %s\n", prefix, param->val_string);
            break;

        case 'E':
            entry = param->val_entry;
            fprintf(file, "%sE: %s\n", prefix, entry->word);
            break;

        case 'R':
            fprintf(file, "%sR: %ld\n", prefix, (gint64) param->val_routine);
            break;

        case 'P':
            fprintf(file, "%sP: %s\n", prefix, param->val_pseudo_entry.word);
            break;

        case 'C':
            fprintf(file, "%sC: %s\n", prefix, param->val_custom_comment);
            break;

        default:
            fprintf(file, "%s%c: %s\n", prefix, param->type, "Unknown type");
            break;
    }
}



// -----------------------------------------------------------------------------
/** Frees memory for a param.

\param gp_param: Pointer to a Param to free.

\note This function does *not* free custom data.
*/
// -----------------------------------------------------------------------------
void free_param(gpointer gp_param) {
    Param *param = gp_param;
    g_free(param->val_string);


    if (param->type == 'P') {
        g_sequence_free(param->val_pseudo_entry.params);
    }

    g_free(param);
}
