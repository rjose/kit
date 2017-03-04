// =============================================================================
/** \file kit.c

\brief Defines main function and control loop.

A program that implements a Forth interpreter that is used to construct
application languages for Rino.

\mainpage

The design of this program is based on the unpublished book "Programming a
Problem-oriented-language" by Chuck Moore in 1970.

The main function sets up the initial dictionary and the main control loop that
basically gets a token from the input stream and executes it.

*/
// =============================================================================


// -----------------------------------------------------------------------------
/** \brief A convenience method to push tokens directly onto the param stack.

This is used during the parsing of the input stream. Only literals such as
integers, doubles, and strings will be converted and pushed onto the stack.

Any words that were dictionary entries would already have been handled.

\param token: Token to convert and push
*/
// -----------------------------------------------------------------------------
void push_token(Token token) {
    // Can't push a Word token
    if (token.type == 'W') {
        handle_error(ERR_UNKNOWN_WORD);
        fprintf(stderr, "----> %s\n", token.word);
        return;
    }

    Param *param_new;
    gint64 val_int;
    gdouble val_double;
    gchar *val_str;

    switch(token.type) {
        case 'I':
            val_int = g_ascii_strtoll(token.word, NULL, 10);
            param_new = new_int_param(val_int);
            push_param(param_new);
            break;

        case 'D':
            val_double = g_ascii_strtod(token.word, NULL);
            param_new = new_double_param(val_double);
            push_param(param_new);
            break;

        case 'S':
            // Start copying yyext after first '"'...
            val_str = g_strdup(yytext+1);

            // ...and NUL out second '"'
            val_str[yyleng-2] = '\0';

            param_new = new_str_param(val_str);
            push_param(param_new);

            g_free(val_str);
            break;

        default:
            handle_error(ERR_UNKNOWN_TOKEN_TYPE);
            fprintf(stderr, "----> %c: %s\n", token.type, token.word);
            return;
    }
}



// -----------------------------------------------------------------------------
/** Sets up the interpreter and then runs the main control loop.
*/
// -----------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    Entry *entry;
    FILE *input_file = NULL;

    build_dictionary();
    create_stack();
    create_stack_r();

    // Open input file if specified; otherwise stdin
    if (argc > 1) {
        input_file = fopen(argv[1], "r");
        if (!input_file) {
            fprintf(stderr, "Unable to open file: %s\n", argv[1]);
            exit(1);
        }
        scan_file(input_file);
    }
    else {
        scan_file(stdin);
    }


    // Control loop
    while(!_quit) {
        Token token = get_token();

        if (token.type == EOF) break;
        if (token.type == '^') continue;

        // If, executing...
        if (_mode == 'E') {
            entry = find_entry(token.word);
            if (entry) {
                execute(entry);
            }
            else {
                push_token(token);
            }
        }

        // ...otherwise, we're compiling
        else {
            compile(token);
        }
    }

    // Clean up
    destroy_dictionary();
    destroy_stack();
    destroy_stack_r();

    destroy_input_stack();
    yylex_destroy();

    if (input_file) fclose(input_file);
}
