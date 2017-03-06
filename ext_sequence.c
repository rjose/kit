/** \file ext_sequence.c

\brief Defines words for operating on sequences

*/



// -----------------------------------------------------------------------------
/** Helper function to get the value of an object given a word that can extract it.

This pushes the object onto the stack and then executes the word. It then
pops the value and the object and returns the value.

*/
// -----------------------------------------------------------------------------
static Param *get_sort_value(gconstpointer gp_param, gchar *sort_word) {
    Param *param = (Param *) gp_param;

    push_param(param);                         // (obj)
    execute_string(sort_word);                 // (obj val)
    Param *result = pop_param();               // (obj)
    pop_param();                               // ()  -- we don't need to free this because it's not ours
    return result;
}



// -----------------------------------------------------------------------------
/** Comparator for generic objects using a sort word (ascending order)
*/
// -----------------------------------------------------------------------------
static gint cmp_func(gconstpointer l, gconstpointer r, gpointer gp_word) {
    gchar *word = gp_word;
    Param *param_l_val = get_sort_value(l, word);
    Param *param_r_val = get_sort_value(r, word);

    gint result = 0;

    if (param_l_val->type == 'I' || param_l_val->type == 'D') {
        result = param_l_val->val_int - param_r_val->val_int;
    }
    else if (param_l_val->type == 'S') {
        result = g_strcmp0(param_l_val->val_string, param_r_val->val_string);
    }
    else {
        fprintf(stderr, "Don't know how to compare '%c'\n", param_l_val->type);
    }

    free_param(param_l_val);
    free_param(param_r_val);
    return result;
}



// -----------------------------------------------------------------------------
/** Sorts a sequence using a word that gets the value from an object

(seq sort-word -- seq)
*/
// ----------------------------------------------------------------------------
static void EC_sort(gpointer gp_entry) {
    // Pop the sort word
    Param *param_word = pop_param();

    // Pop the sequence
    Param *param_seq = pop_param();
    GSequence *sequence = param_seq->val_custom;

    g_sequence_sort(sequence, cmp_func, param_word->val_string);
    push_param(param_seq);

    free_param(param_word);
    return;
}



// -----------------------------------------------------------------------------
/** Gets length of sequence

(seq -- seq int)
*/
// -----------------------------------------------------------------------------
static void EC_len(gpointer gp_entry) {
    const Param *param_seq = top();

    GSequence *seq = param_seq->val_custom;
    push_param(new_int_param(g_sequence_get_length(seq)));
}



static void EC_start_seq(gpointer gp_entry) {
    Param *param_start_seq = new_param();
    param_start_seq->type = '[';
    push_param(param_start_seq);
}


void free_param_seq(gpointer gp_seq) {
    g_sequence_free(gp_seq);
}



/** Constructs a sequence of items on the stack down to next '[' param

\note The elements of the sequence need to be freed by the caller
*/
static void EC_end_seq(gpointer gp_entry) {
    Param *param = pop_param();
    if (!param) {
        handle_error(ERR_STACK_UNDERFLOW);
        fprintf(stderr, "-----> stack underflow\n");
        return;
    }
    GSequence *seq = g_sequence_new(NULL);
    while(param->type != '[') {
        g_sequence_prepend(seq, param);
        param = pop_param();
        if (!param) {
            handle_error(ERR_STACK_UNDERFLOW);
            fprintf(stderr, "-----> stack underflow\n");
            return;
        }
    }
    free_param(param);  // This will be the '[' param

    Param *param_new = new_custom_param(seq, "[?]", free_param_seq);
    push_param(param_new);
}



/** Maps a word over a seq

The word should pop a param, freeing it when done, and then pushing
a new value onto the stack.

(seq-in word -- seq-out)
*/
static void EC_map(gpointer gp_entry) {
    Param *param_word = pop_param();

    Param *param_seq = pop_param();
    GSequence *seq = param_seq->val_custom;

    execute_string("[");

    for (GSequenceIter *iter=g_sequence_get_begin_iter(seq);
         !g_sequence_iter_is_end(iter);
         iter = g_sequence_iter_next(iter)) {

        Param *param = g_sequence_get(iter);
        push_param(param);
        execute_string(param_word->val_string);  // This will consume param
    }

    execute_string("]");

    free_param(param_word);
    free_param(param_seq);
}


void print_seq(FILE *file, Param *param) {
    GSequence *sequence = param->val_custom;

    fprintf(file, "Sequence: %s\n", param->val_custom_comment);
    for (GSequenceIter *iter = g_sequence_get_begin_iter(sequence);
         !g_sequence_iter_is_end(iter);
         iter = g_sequence_iter_next(iter)) {

        Param *p = g_sequence_get(iter);
        fprintf(file, "    ");
        print_param(file, p);
    }
}





// -----------------------------------------------------------------------------
/** Defines the sequence lexicon

*/
// -----------------------------------------------------------------------------
void EC_add_sequence_lexicon(gpointer gp_entry) {
    add_entry("[")->routine = EC_start_seq;
    add_entry("]")->routine = EC_end_seq;

    add_entry("len")->routine = EC_len;
    add_entry("map")->routine = EC_map;
    add_entry("sort")->routine = EC_sort;

    add_print_function("[?]", print_seq);
}
