/** \file entry.c

\brief Functions for creating, manipulating, executing, and destroying Entry objects.

Entry objects are elements of a Dictionary. When tokens are parsed from input,
they are looked up in the _dictionary. If a corresponding Entry is found, it
is executed.
*/


// -----------------------------------------------------------------------------
/** Executes the code associated with an Entry.

\param gp_entry: A pointer to an Entry
*/
// -----------------------------------------------------------------------------
void execute(gpointer gp_entry) {
    Entry *entry = gp_entry;
    entry->routine(entry);
}



// -----------------------------------------------------------------------------
/** Compiles a token into the latest Entry's definition.
*/
// -----------------------------------------------------------------------------
void compile(Token token) {
    Entry *entry;
    Entry *entry_latest = latest_entry();
    Param *param;
    Entry *pseudo_entry;
    Param *param_literal;
    gchar *val_string = NULL;

    switch(token.type) {
        case 'W':
            entry = find_entry(token.word);
            if (!entry) {
                handle_error(ERR_UNKNOWN_WORD);
                fprintf(stderr, "-----> %s\n", token.word);
                return;
            }
            else if(entry->immediate) {
                execute(entry);
            }
            else {
                param = new_entry_param(entry);
                add_entry_param(entry_latest, param);
            }
            break;

        case 'I':
            // Create pseudo entry that pushes an int onto the stack
            param = new_pseudo_entry_param("push-literal-I", EC_push_param0);
            pseudo_entry = &param->val_pseudo_entry;
            param_literal = new_int_param(g_ascii_strtoll(token.word, NULL, 10));
            add_entry_param(pseudo_entry, param_literal);

            add_entry_param(entry_latest, param);
            break;

        case 'D':
            // Create pseudo entry that pushes a double onto the stack
            param = new_pseudo_entry_param("push-literal-D", EC_push_param0);
            pseudo_entry = &param->val_pseudo_entry;
            param_literal = new_double_param(g_ascii_strtod(token.word, NULL));
            add_entry_param(pseudo_entry, param_literal);

            add_entry_param(entry_latest, param);
            break;

        case 'S':
            // Create pseudo entry that pushes a string onto the stack
            param = new_pseudo_entry_param("push-literal-S", EC_push_param0);
            pseudo_entry = &param->val_pseudo_entry;

            // Start copying yyext after first '"'...
            val_string =  g_strdup(yytext+1);

            // ...and NUL out second '"'
            val_string[yyleng-2] = '\0';

            param_literal = new_str_param(val_string);
            g_free(val_string);
            add_entry_param(pseudo_entry, param_literal);

            add_entry_param(entry_latest, param);
            break;

        default:
            printf("TODO: Handle token type: %c\n", token.type);
            break;
    }
}



// -----------------------------------------------------------------------------
/** Allocates memory for an Entry and returns a pointer to it

\returns A pointer to a newly allocated Entry

When an Entry is destroyed, all of its params are freed using free_param.
That means that parameters from the stack that are added to an entry
can be considered managed by that entry.
*/
// -----------------------------------------------------------------------------
Entry *new_entry() {
    Entry *result = g_new(Entry, 1);
    result->immediate = 0;
    result->complete = 1;
    result->params = g_sequence_new(free_param);
    return result;
}



// -----------------------------------------------------------------------------
/** Adds a parameter to an entry.

\param entry: Entry to add parameter to
\param param: Param to add

\note The param should be dynamically allocated because the Entry will free it.
*/
// -----------------------------------------------------------------------------
void add_entry_param(Entry *entry, Param *param) {
    g_sequence_append(entry->params, param);
}



// -----------------------------------------------------------------------------
/** Frees the memory allocated for an Entry

\param gp_entry: A pointer to an Entry to be freed.
*/
// -----------------------------------------------------------------------------
void free_entry(gpointer gp_entry) {
    Entry *entry = gp_entry;
    g_sequence_free(entry->params);
    g_free(gp_entry);
}
