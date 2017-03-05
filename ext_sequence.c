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

    // TODO: Get rid of this
//    for (GSequenceIter *iter = g_sequence_get_begin_iter(sequence);
//         !g_sequence_iter_is_end(iter);
//         iter = g_sequence_iter_next(iter)) {
//
//        Param *p = g_sequence_get(iter);
//        printf("%ld\n", p->val_int);
//    }

done:
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


static void free_param_seq(gpointer gp_seq) {
    g_sequence_free(gp_seq);
}


/** Creates a sequence by popping params up until a "start seq" param.

Since we add params to a sequence that will be pushed back onto the
stack, we don't free the params.
*/
static void end_seq(gpointer gp_entry) {
    Param *param = pop_param();
    if (!param) {
        handle_error(ERR_STACK_UNDERFLOW);
        fprintf(stderr, "-----> stack underflow\n");
        return;
    }
    GSequence *seq = g_sequence_new(free_param);
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

    Param *param_new = new_custom_param(seq, "[?]");
    param_new->free_custom = free_param_seq;
    push_param(param_new);
}


/** Constructs a sequence of items on the stack down to next '[' param

\note The elements of the sequence need to be freed by the caller
*/
static void EC_end_seq(gpointer gp_entry) {
    end_seq(gp_entry);
}



/** Maps a word over a seq

\note This leaves the input sequence on the stack because it's not clear
how its memory should be freed.

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
        execute_string(param_word->val_string);
    }

    execute_string("]");

    free_param(param_word);
    free_param_seq(param_seq);
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
}
