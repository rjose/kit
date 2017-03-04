/** \file globals.h
*/

#pragma once

// =============================================================================
// Defines
// =============================================================================
// -----------------------------------------------------------------------------
// Error codes
// -----------------------------------------------------------------------------

#define ERR_UNKNOWN_WORD  1
#define ERR_UNKNOWN_TOKEN_TYPE  2
#define ERR_STACK_UNDERFLOW  3
#define ERR_INVALID_PARAM  4
#define ERR_GENERIC_ERROR  5


// =============================================================================
// External functions
// =============================================================================
extern int yyleng;
extern int yylex();          /**< \brief Gets next token from the input stream */
extern void yyrestart(FILE *file);
extern int yylex_destroy();  /**< \brief Gets next token from the input stream */
extern FILE* yyin;           /**< \brief Points to the input stream */
extern char *yytext;         /**< \brief Current token */
extern void scan_string(const char* str);
extern void scan_file(FILE* file);
extern void destroy_input_stack();


// =============================================================================
// Global entities
// =============================================================================

/** Represents a token from the input stream.

*/
typedef struct {
    /** The type of the token is represented by the following characters:

        - 'W': Word
        - 'I': Integer
        - 'D': Double
        - 'S': String
        - EOF: End of input

        The type is determined by the lexer.
    */
    gchar type;

    /** Characters parsed from the input stream.
    */
    gchar word[MAX_WORD_LEN];
} Token;


extern GList *_dictionary;
extern GQueue *_stack;
extern GQueue *_return_stack;
extern gchar _mode;
extern jmp_buf _error_jmp_buf;
extern GSequenceIter *_ip;
extern gboolean _quit;

const gchar *error_type_to_string(gint error_type);
Token get_token();
void handle_error(gint error_type);
void push_token(Token token);
