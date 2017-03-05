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
/** Sets up the interpreter and then runs the main control loop.
*/
// -----------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    FILE *input_file = NULL;

    build_dictionary();
    create_print_functions();
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
        if (token.type == '^') continue;   // If EOS, keep going

        process_token(token);
    }

    // Clean up
    destroy_stack_r();
    destroy_stack();
    destroy_print_functions();
    destroy_dictionary();

    destroy_input_stack();
    yylex_destroy();

    if (input_file) fclose(input_file);
}
