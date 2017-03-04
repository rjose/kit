/** \file globals.c

\brief All global objects are here.

This defines the main components of the Forth interpreter: _dictionary,
_stack, and _return_stack.

*/

// =============================================================================
// Globals
// =============================================================================

GList *_dictionary = NULL;      /**< \brief Global Forth dictionary */
GQueue *_stack = NULL;          /**< \brief Global Param stack */
GQueue *_return_stack = NULL;   /**< \brief Global return stack */
jmp_buf _error_jmp_buf;         /**< \brief Global jump buffer for error handling */


/** Global interpreter mode. The legal values are:

    - E: Execution mode (normal)
    - C: Compilation mode (during word definition)
*/
gchar _mode = 'E';              /**< \brief 'E'xecuting or 'C'ompiling */

GSequenceIter *_ip = NULL;      /**< \brief Next instruction (Param) to execute in a definition */

gboolean _quit = 0;             /**< \brief To quit program cleanly, set _quit=1 */



// =============================================================================
// Error reporting
// =============================================================================

// Error strings
static gchar unknown_word[] = "Unknown word";
static gchar unknown_error[] = "Unknown error";
static gchar unknown_token_type[] = "Unknown token type";
static gchar stack_underflow[] = "Stack underflow";
static gchar invalid_param[] = "Invalid parameter";
static gchar generic_error[] = "Generic error";


// -----------------------------------------------------------------------------
/** Returns an error string for the given error code.

\param error_type  An integer error code
\returns A pointer to a static error string

*/
// -----------------------------------------------------------------------------
const gchar *error_type_to_string(gint error_type) {
    gchar *result;
    switch(error_type) {
        case ERR_UNKNOWN_WORD:
            result = unknown_word;
            break;

        case ERR_UNKNOWN_TOKEN_TYPE:
            result = unknown_token_type;
            break;

        case ERR_STACK_UNDERFLOW:
            result = stack_underflow;
            break;

        case ERR_INVALID_PARAM:
            result = invalid_param;
            break;

        case ERR_GENERIC_ERROR:
            result = generic_error;
            break;

        default:
            result = unknown_error;
            break;
    }
    return result;
}



// -----------------------------------------------------------------------------
/** Returns the next token from the input stream. If at the EOF, this returns a
token with type equal to EOF.

\returns A Token representing the next token
*/
// -----------------------------------------------------------------------------
Token get_token() {
    Token result;
    result.type = yylex();

    switch(result.type) {
        case 'S': case 'I': case 'D': case 'W':
            g_strlcpy(result.word, yytext, MAX_WORD_LEN);
            break;

        case EOF:
            g_strlcpy(result.word, "EOF", MAX_WORD_LEN);
            break;

        default:
            g_strlcpy(result.word, "?", MAX_WORD_LEN);
            break;
    }

    return result;
}



// -----------------------------------------------------------------------------
/** Prints out the error type and resets the state of the interpreter.

*/
// -----------------------------------------------------------------------------
void handle_error(gint error_type) {
    fprintf(stderr, "%s\n", error_type_to_string(error_type));

    // Reset stacks, _ip, and _mode
    _ip = NULL;
    clear_stack();
    clear_stack_r();

    _mode = 'E';
}
