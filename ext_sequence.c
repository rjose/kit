/** \file ext_sequence.c

\brief Defines words for operating on sequences

*/


// -----------------------------------------------------------------------------
/** Helper function to get the value of an object given a word that can extract it.

This pushes the object onto the stack and then executes the word. It then
pops the value and the object and returns the value.

*/
// -----------------------------------------------------------------------------
static Param *get_value(gconstpointer gp_param, gchar *sort_word) {
    Param *param = (Param *) gp_param;

    COPY_PARAM(param_new, param);

    push_param(param_new);                     // (obj -- )
    execute_string(sort_word);                 // (val -- )
    Param *result = pop_param();               // ()
    return result;
}



// -----------------------------------------------------------------------------
/** Comparator for generic objects using a sort word (ascending order)
*/
// -----------------------------------------------------------------------------
static gint cmp_func(gconstpointer l, gconstpointer r, gpointer gp_word) {
    gchar *word = gp_word;
    Param *param_l_val = get_value(l, word);
    Param *param_r_val = get_value(r, word);

    gint result = 0;

    if (param_l_val->type == 'I') {
        result = param_l_val->val_int - param_r_val->val_int;
    }
    if (param_l_val->type == 'D') {
        result = param_l_val->val_double - param_r_val->val_double;
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
    Param *param_seq = pop_param();

    GSequence *sequence = param_seq->val_custom;

    g_sequence_sort(sequence, cmp_func, param_word->val_string);
    push_param(param_seq);

    free_param(param_word);
    return;
}



// -----------------------------------------------------------------------------
/** Sorts a sequence using a word that gets the value from an object

(seq forth-string -- seq)
*/
// ----------------------------------------------------------------------------
static void EC_filter(gpointer gp_entry) {
    Param *param_forth = pop_param();
    Param *param_seq = pop_param();

    GSequence *seq = param_seq->val_custom;

    GSequence *filtered_seq = g_sequence_new(free_param);
    gchar seq_type[MAX_WORD_LEN] = "[?]";

    FOREACH_SEQ(iter, seq) {
        Param *item = g_sequence_get(iter);
        snprintf(seq_type, MAX_WORD_LEN, "[%s]", item->val_custom_type);  // Yeah, we should just set it once
        Param *param_val = get_value(item, param_forth->val_string);
        if (param_val->val_int) {
            COPY_PARAM(param_new, item);
            g_sequence_append(filtered_seq, param_new);
        }
        free_param(param_val);
    }

    push_param(new_custom_param(filtered_seq, seq_type, free_seq, copy_seq));

    free_param(param_forth);
    free_param(param_seq);
}



/** Concatenate a sequence of sequences

([Sequence] -- Sequence)
*/
static void EC_concat(gpointer gp_entry) {
    Param *param_seq_seq = pop_param();
    GSequence *seq_seq = param_seq_seq->val_custom;

    GSequence *result = g_sequence_new(free_param);
    gchar seq_type[MAX_WORD_LEN];
    FOREACH_SEQ(iter, seq_seq) {
        Param *param_seq = g_sequence_get(iter);
        GSequence *seq = param_seq->val_custom;
        g_strlcpy(seq_type, param_seq->val_custom_type, MAX_WORD_LEN);  // Yeah, only need to do this once...

        for (GSequenceIter *jiter = g_sequence_get_begin_iter(seq);
             !g_sequence_iter_is_end(jiter);
             jiter = g_sequence_iter_next(jiter)) {

            Param *param = g_sequence_get(jiter);
            COPY_PARAM(param_new, param);
            g_sequence_append(result, param_new);
        }
    }

    push_param(new_custom_param(result, seq_type, free_seq, copy_seq));

    free_param(param_seq_seq);
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


void free_seq(gpointer gp_seq) {
    g_sequence_free(gp_seq);
}



gpointer copy_seq(gpointer gp_seq) {
    GSequence *src = gp_seq;

    GSequence *result = g_sequence_new(free_param);
    FOREACH_SEQ(iter, src) {
        Param *param = g_sequence_get(iter);
        COPY_PARAM(param_new, param);
        g_sequence_append(result, param_new);
    }

    return result;
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

    Param *param_new = new_custom_param(seq, "[?]", free_seq, copy_seq);
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

    FOREACH_SEQ(iter, seq) {
        Param *param = g_sequence_get(iter);
        COPY_PARAM(param_new, param);
        push_param(param_new);
        execute_string(param_word->val_string);  // This will consume param
    }

    execute_string("]");

    free_param(param_word);
    free_param(param_seq);
}


void print_seq(FILE *file, Param *param) {
    GSequence *sequence = param->val_custom;

    fprintf(file, "Sequence: %s\n", param->val_custom_type);
    FOREACH_SEQ(iter, sequence) {
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
    add_entry("filter")->routine = EC_filter;

    add_entry("concat")->routine = EC_concat;

    add_print_function("[?]", print_seq);
}
