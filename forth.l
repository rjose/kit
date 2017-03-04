
%{
int fileno(FILE *stream);
void handle_word(const char *word);

/** Defines an input stack that can be used for nested executions and
    string-based executions of the interpreter.
*/
#define MAX_INPUT_DEPTH 10
YY_BUFFER_STATE input_stack[MAX_INPUT_DEPTH];
int input_stack_ptr = 0;

%}

DIGIT  [0-9]

%%

#.*\n                  /* Skip comments */

\"[^"]*\"              {return 'S';}

[[:space:]]+           /* Skip whitespace */

-?{DIGIT}+             {return 'I';}
{DIGIT}+"."{DIGIT}*    {return 'D';}
[^[:space:]]+          {return 'W';}

<<EOF>>                {
                           // Move the input stack ptr back one and
                           // delete the buffer at that position
                           input_stack_ptr--;
                           yy_delete_buffer(input_stack[input_stack_ptr]);

                           // If we're at the start of the input stack, we're done.
                           // Otherwise, switch to the previous input buffer.
                           if (input_stack_ptr == 0) {
                               return EOF;
                           }
                           else {
                               yy_switch_to_buffer(input_stack[input_stack_ptr-1]);
                               return '^';
                           }
                       }


%%

// This is just to silence the unused function warnings
void use_unused() {
    input();
    yyunput(0, NULL);
}

extern int yywrap() {
    return 1;
}

// -----------------------------------------------------------------------------
/** Pushes file input onto input stack and switches to it.
*/
// -----------------------------------------------------------------------------
void scan_file(FILE* file) {
    if (input_stack_ptr >= MAX_INPUT_DEPTH) {
        fprintf(stderr, "Too many nested inputs\n");
        exit(1);
    }
    yyin = file;
    YY_BUFFER_STATE new_state = yy_create_buffer(yyin, YY_BUF_SIZE);
    yy_switch_to_buffer(new_state);
    input_stack[input_stack_ptr++] = new_state;
}



// -----------------------------------------------------------------------------
/** Pushes string input onto input stack and scans it.
*/
// -----------------------------------------------------------------------------
void scan_string(const char* str) {
    if (input_stack_ptr >= MAX_INPUT_DEPTH) {
        fprintf(stderr, "Too many nested inputs\n");
        exit(1);
    }
    input_stack[input_stack_ptr++] = yy_scan_string(str);
}


// -----------------------------------------------------------------------------
/** Cleans up any scanner inputs.
*/
// -----------------------------------------------------------------------------
void destroy_input_stack() {
    for (gint i=0; i < input_stack_ptr; i++) {
        yy_delete_buffer(input_stack[i]);
    }
}
