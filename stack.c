/** \file stack.c

\brief Defines functions for creating, manipulating, and freeing the parameter stack.

The parameter stack is used to pass arguments and results between words and entry
routines.

The Param objects on the stack are dynamically allocated. Clients who pop items
off the stack are responsible for freeing them. Any items left on the stack are
automatically freed when the stack is cleared or destroyed.
*/


// -----------------------------------------------------------------------------
/** Creates a new stack
*/
// -----------------------------------------------------------------------------
void create_stack() {
    _stack = g_queue_new();
}



// -----------------------------------------------------------------------------
/** Helper function used to free a parameter
*/
// -----------------------------------------------------------------------------
static void _free_param(gpointer param, gpointer unused) {
    free_param(param);
}



// -----------------------------------------------------------------------------
/** Clears stack, freeing all Param objects on the stack
*/
// -----------------------------------------------------------------------------
void clear_stack() {
    g_queue_foreach(_stack, _free_param, NULL);
    g_queue_clear(_stack);
}



// -----------------------------------------------------------------------------
/** Frees memory for a stack

The stack is cleared and all Param objects on the stack are freed as well.
*/
// -----------------------------------------------------------------------------
void destroy_stack() {
    clear_stack();
    g_queue_free(_stack);
}



// -----------------------------------------------------------------------------
/** Pushes a param onto the stack.

*/
// -----------------------------------------------------------------------------
void push_param(Param* param) {
    g_queue_push_tail(_stack, param);
}



// -----------------------------------------------------------------------------
/** Pops a parameter off the stack.

\note The caller of this is responsible for freeing the param when done with it.
*/
// -----------------------------------------------------------------------------
Param *pop_param() {
    if (g_queue_is_empty(_stack)) {
        return NULL;
    }

    return g_queue_pop_tail(_stack);
}



// -----------------------------------------------------------------------------
/** Returns top of stack so caller can peek at it.

*/
// -----------------------------------------------------------------------------
const Param *top() {
    return g_queue_peek_tail(_stack);
}
