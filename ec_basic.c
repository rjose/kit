/** \file ec_basic.c

\brief Basic entry routines for builtin words.

These are entry routines for the basic builtin words as well as generic
routines used when defining entries dynamically.

*/


static void EC_execute(gpointer gp_entry);
static void EC_jmp(gpointer gp_entry);
static void EC_jmp_if_false(gpointer gp_entry);
static void EC_push_entry_address(gpointer gp_entry);


// -----------------------------------------------------------------------------
/** Convenience function to add a variable entry to the dictionary.
*/
// -----------------------------------------------------------------------------
void add_variable(const gchar *word) {
    Entry *entry_new = add_entry(word);
    entry_new->routine = EC_push_entry_address;

    // Adds an empty param to the variable entry for storing values
    Param *value = new_param();
    add_entry_param(entry_new, value);
}



// -----------------------------------------------------------------------------
/** Sets the _quit flag so the main control loop stops.
*/
// -----------------------------------------------------------------------------
static void EC_quit(gpointer gp_entry) {
    _quit = 1;
}



// -----------------------------------------------------------------------------
/** Switches the interpreter's input stream to stdin so the user can interact
    with the interpreter.

\note We'll need a different way of doing this if we want to support loading
      forth files or executing forth statements from a string buffer.

*/
// -----------------------------------------------------------------------------
static void EC_interactive(gpointer gp_entry) {
    scan_file(stdin);
}



// -----------------------------------------------------------------------------
/** Creates a new constant entry.

(val str-name -- )

This pops a string and then a value and then creates a constant using
the string for the name.

The routine for the new constant pushes this value onto the stack.

\param gp_entry: Unused entry for the "constant" word

*/
// -----------------------------------------------------------------------------
static void EC_constant(gpointer gp_entry) {
    Param *param_str = pop_param();
    Param *param0 = pop_param();

    // NOTE: When we add the popped param to the entry, its memory is
    //       managed by that entry, so we don't need to free it here.
    Entry *entry_new = add_entry(param_str->val_string);
    entry_new->routine = EC_push_param0;
    add_entry_param(entry_new, param0);

    free_param(param_str);
}



// -----------------------------------------------------------------------------
/** Creates a new variable entry.


(str-name -- )

This pops a string off the stack and uses this as the word for the entry.

The routine for the new variable pushes the address of the variable's
entry onto the stack.

*/
// -----------------------------------------------------------------------------
static void EC_variable(gpointer gp_entry) {
    Param *param_str = pop_param();
    add_variable(param_str->val_string);

    free_param(param_str);
}



// -----------------------------------------------------------------------------
/** Pops a variable entry address and a parameter value and stores in variable entry.

\param gp_entry: unused
*/
// -----------------------------------------------------------------------------
static void EC_store_variable_value(gpointer gp_entry) {
    Param *p_var = pop_param();    // Variable to store value in
    if (!p_var) {
        handle_error(ERR_STACK_UNDERFLOW);
        return;
    }

    Param *p_value = pop_param();  // Value to store
    if (!p_value) {
        handle_error(ERR_STACK_UNDERFLOW);
        return;
    }

    if (p_var->type != 'E') {
        handle_error(ERR_INVALID_PARAM);
        fprintf(stderr, "----> ");
        print_param(stderr, p_var);
        return;
    }


    // Store value in variable
    Entry *entry_var = p_var->val_entry;
    GSequenceIter *iter = g_sequence_get_iter_at_pos(entry_var->params, 0);
    Param *var_value = g_sequence_get(iter);
    copy_param(var_value, p_value);

    // Cleanup
    free_param(p_value);
    free_param(p_var);
}



// -----------------------------------------------------------------------------
/** Pops a variable and pushes its value onto the stack

\param gp_entry: unused
*/
// -----------------------------------------------------------------------------
static void EC_fetch_variable_value(gpointer gp_entry) {
    Param *p_var = pop_param();

    Entry *entry_var = p_var->val_entry;
    GSequenceIter *iter = g_sequence_get_iter_at_pos(entry_var->params, 0);
    Param *var_value = g_sequence_get(iter);

    Param *param_new = new_param();
    copy_param(param_new, var_value);
    push_param(param_new);

    // Cleanup
    free_param(p_var);
}



// -----------------------------------------------------------------------------
/** Pushes the first parameter of an entry onto the stack.

\param gp_entry: The entry with the parameter to be pushed.
*/
// -----------------------------------------------------------------------------
void EC_push_param0(gpointer gp_entry) {
    Entry *entry = gp_entry;
    GSequenceIter *begin = g_sequence_get_begin_iter(entry->params);
    Param *param0 = g_sequence_get(begin);

    Param *param_new = new_param();
    copy_param(param_new, param0);
    push_param(param_new);
}



// -----------------------------------------------------------------------------
/** Pushes address of entry onto the stack

\param gp_entry: The entry with the parameter to be pushed.
*/
// -----------------------------------------------------------------------------
static void EC_push_entry_address(gpointer gp_entry) {
    Entry *entry = gp_entry;
    Param *param = new_entry_param(entry);
    push_param(param);
}



static void gfunc_print_param(gpointer gp_param, gpointer user_data) {
    Param *param = gp_param;
    print_param(stdout, param);
}



// -----------------------------------------------------------------------------
/** Prints stack nondestructively. Top of stack is at the top.

*/
// -----------------------------------------------------------------------------
static void EC_print_stack(gpointer gp_entry) {
    // NOTE: We're assuming that this goes from the first element to the last
    g_queue_foreach(_stack, gfunc_print_param, NULL);
    printf("\n");
}



// -----------------------------------------------------------------------------
/** Routine for the define word (":")

\param gp_entry: The ":" entry

We read the next token, which will be the word for the new definition. Then we
switch to compile mode so each word we read can be added to the parameters of
the new entry as part of its definition. Different categories of tokens are
compiled differently:

- Dictionary entries: An "E" parameter is created and added to the new definition.
                      On execution, the entry is simply executed.

- Literals:           A "P" parameter is created with its first parameter
                      being the literal. The routine of the "P" parameter
                      pushes the first param onto the stack.

- Immediate words:    These are words like ';' that are executed during a
                      compilation. Macros are an example of an immediate
                      word.

The first parameter is a pointer to the routine for executing a defined word. By
allowing this to be dynamically specified, we change the behavior of defined
words.
*/
// -----------------------------------------------------------------------------
static void EC_define(gpointer gp_entry) {
    // Create new entry
    Token token = get_token();
    Entry *entry_new = add_entry(token.word);
    entry_new->complete = 0;
    entry_new->routine = EC_execute;

    _mode = 'C';
}



// -----------------------------------------------------------------------------
/** Pops return stack and stores in _ip.
*/
// -----------------------------------------------------------------------------
static void EC_pop_return_stack(gpointer gp_entry) {
    _ip = pop_param_r();
}



// -----------------------------------------------------------------------------
/** Marks the end of the definition and returns interpreter to 'E'xecute mode.
*/
// -----------------------------------------------------------------------------
static void EC_end_define(gpointer gp_entry) {
    Entry *entry_latest = latest_entry();
    entry_latest->complete = 1;
    Param *pseudo_param = new_pseudo_entry_param(";", EC_pop_return_stack);
    add_entry_param(entry_latest, pseudo_param);

    _mode = 'E';
}



// -----------------------------------------------------------------------------
/** Implements branching by compiling a conditional jump into a definition.

The behavior of the compiled code is to pop a parameter from the stack and
if it is false, jump to the end of the "if" block. Otherwise, continue through
the subsequent statements.

We compile this by adding a conditional jump "pseudo entry". Because we don't
know, at this time of the compilation, where to jump to, we push the pseudo entry
onto the stack to be filled out later by an "else" or a "then" word.
*/
// -----------------------------------------------------------------------------
static void EC_if(gpointer gp_entry) {
    Entry *entry_latest = latest_entry();
    Param *pseudo_param = new_pseudo_entry_param("jmp-if-false", EC_jmp_if_false);
    add_entry_param(entry_latest, pseudo_param);

    // Push pseudo_param Entry onto stack so we can fill it out later
    Param *param_pseudo_entry = new_entry_param(&pseudo_param->val_pseudo_entry);
    push_param(param_pseudo_entry);
}



// -----------------------------------------------------------------------------
/** Implements the "else" block of a conditional part of a definition.

The "else" should correspond to an earlier "if". At this point, we know where
the "if" should jump to if the condition is false. We store this in the
"pseudo entry" of the earlier "if" by popping it off the stack and setting
its first parameter to the offset into the definition's instruction sequence.

The "else" word marks the end of the "if" section and so an uncondtional
jmp must be added to the definition first. Similar to the "if" jmp, we
will need to fill out the jmp target later, so we push it onto the stack.
*/
// -----------------------------------------------------------------------------
static void EC_else(gpointer gp_entry) {
    Entry *entry_latest = latest_entry();

    // Pop param so we can fill out the target for the jmp
    Param *param_jmp_entry = pop_param();
    Entry *entry_jmp = param_jmp_entry->val_entry;
    Param *param_jmp_target = new_int_param(g_sequence_get_length(entry_latest->params) + 1);
    add_entry_param(entry_jmp, param_jmp_target);

    // Add the jmp param
    Param *pseudo_param = new_pseudo_entry_param("jmp", EC_jmp);
    add_entry_param(entry_latest, pseudo_param);

    // Push pseudo_param Entry onto stack so we can fill it out later
    Param *param_pseudo_entry = new_entry_param(&pseudo_param->val_pseudo_entry);
    push_param(param_pseudo_entry);

    free_param(param_jmp_entry);
}



// -----------------------------------------------------------------------------
/** Implements the end of a conditional section of code.

This pops a "pseudo entry" and sets its jmp target to be the next instruction.
*/
// -----------------------------------------------------------------------------
static void EC_then(gpointer gp_entry) {
    Entry *entry_latest = latest_entry();

    // Pop param so we can fill out the target for the jmp
    Param *param_jmp_entry = pop_param();
    Entry *entry_jmp = param_jmp_entry->val_entry;
    Param *param_jmp_target = new_int_param(g_sequence_get_length(entry_latest->params));
    add_entry_param(entry_jmp, param_jmp_target);

    free_param(param_jmp_entry);
}



// -----------------------------------------------------------------------------
/** Implements a conditional jmp by updating the instruction pointer.

This pops a param off the stack. If its val_int is 0, then it does a jmp to
the instruction at the offset specified in its first param.

*/
// -----------------------------------------------------------------------------
static void EC_jmp_if_false(gpointer gp_entry) {
    Entry *entry = gp_entry;

    Param *param_bool = pop_param();
    // If false, move the instruction pointer according to the entry's first param
    if (param_bool->val_int == 0) {
        // Get first param of entry
        GSequenceIter *iter = g_sequence_get_iter_at_pos(entry->params, 0);
        Param *param0 = g_sequence_get(iter);

        // Update the instruction pointer
        GSequence *instructions = g_sequence_iter_get_sequence(_ip);
        _ip = g_sequence_get_iter_at_pos(instructions, param0->val_int);
    }

    free_param(param_bool);
}



// -----------------------------------------------------------------------------
/** Implements an unconditional jmp by updating the instruction pointer.

This sets the instruction pointer to the offset specified in the entry's
first param.
*/
// -----------------------------------------------------------------------------
static void EC_jmp(gpointer gp_entry) {
    Entry *entry = gp_entry;

    // Get first param of entry
    GSequenceIter *iter = g_sequence_get_iter_at_pos(entry->params, 0);
    Param *param0 = g_sequence_get(iter);

    // Update the instruction pointer
    GSequence *instructions = g_sequence_iter_get_sequence(_ip);
    _ip = g_sequence_get_iter_at_pos(instructions, param0->val_int);
}



// -----------------------------------------------------------------------------
/** Prints the words in an Entry definition.
*/
// -----------------------------------------------------------------------------
static void EC_print_definition(gpointer gp_entry) {
    Param *param_word = pop_param();
    Entry *entry = find_entry(param_word->val_string);

    if (!entry) {
        handle_error(ERR_UNKNOWN_WORD);
        fprintf(stderr, "-----> %s\n", param_word->val_string);
        goto done;
    }

    for (GSequenceIter *iter = g_sequence_get_begin_iter(entry->params);
         !g_sequence_iter_is_end(iter);
         iter = g_sequence_iter_next(iter)) {

        Param *p = g_sequence_get(iter);
        print_param(stdout, p);
    }

done:

    free_param(param_word);
}



// -----------------------------------------------------------------------------
/** Executes a definition

This starts by pushing the current _ip onto the return stack and then setting
the _ip to the first parameter of the entry's definition (stored in its params
field). From there, each parameter of the definition is executed sequentially. If
one of these parameters is a Dictionary entry, it will be executed by this same
function, which will result in the return stack noting the place to return
once that execution is complete.
*/
// -----------------------------------------------------------------------------
static void EC_execute(gpointer gp_entry) {
    Entry *entry = gp_entry;
    Param *cur_param;
    Entry *pseudo_entry;

    push_param_r(_ip);

    _ip = g_sequence_get_begin_iter(entry->params);

    while (_ip) {
        cur_param = g_sequence_get(_ip);
        _ip = g_sequence_iter_next(_ip);

        switch(cur_param->type) {
            case 'E':
                execute(cur_param->val_entry);
                break;

            case 'P':
                pseudo_entry = &cur_param->val_pseudo_entry;
                pseudo_entry->routine(pseudo_entry);
                break;

            default:
                handle_error(ERR_UNKNOWN_WORD);
                fprintf(stderr, "----->");
                print_param(stderr, cur_param);
                return;
        }
    }
}



// -----------------------------------------------------------------------------
/** Pops a parameter and prints it.

*/
// -----------------------------------------------------------------------------
static void EC_print(gpointer gp_entry) {
    Param *param = pop_param();
    print_param(stdout, param);  // This frees the param
    free_param(param);
}



// -----------------------------------------------------------------------------
/** Pops a parameter from the stack

(item -- )
*/
// -----------------------------------------------------------------------------
static void EC_pop(gpointer gp_entry) {
    Param *param = pop_param();
    free_param(param);
}


// -----------------------------------------------------------------------------
/** Pops a parameter from the stack, but does not free its memory
*/
// -----------------------------------------------------------------------------
static void EC_drop(gpointer gp_entry) {
    pop_param();
}



// -----------------------------------------------------------------------------
/** Duplicates the top of the stack.

\note This may not be well-behaved for custom values

*/
// -----------------------------------------------------------------------------
static void EC_dup(gpointer gp_entry) {
    Param *param_new = new_param();
    copy_param(param_new, top());
    push_param(param_new);
}



// -----------------------------------------------------------------------------
/** Callback used with sqlite3 to store a retrieved value.

This is used with the EC_DB_DOUBLE_GETTER and EC_DB_INT_GETTER macros.
*/
// -----------------------------------------------------------------------------
int set_double_cb(gpointer gp_double_ref, int num_cols, char **values, char **cols) {
    if (num_cols != 1) {
        handle_error(ERR_GENERIC_ERROR);
        fprintf(stderr, "-----> Unexpected num cols in set_double_cb\n");
        return 1;
    }

    double *double_ref = gp_double_ref;
    *double_ref = values[0] ? g_ascii_strtod(values[0], NULL) : 0.0;
    return 0;
}



// -----------------------------------------------------------------------------
/** Callback used with sqlite3 to store a retrieved value.

*/
// -----------------------------------------------------------------------------
int set_int_cb(gpointer gp_int_ref, int num_cols, char **values, char **cols) {
    if (num_cols != 1) {
        handle_error(ERR_GENERIC_ERROR);
        fprintf(stderr, "-----> Unexpected num cols in set_int_cb\n");
        return 1;
    }

    gint64 *int_ref = gp_int_ref;
    *int_ref = values[0] ? g_ascii_strtoll(values[0], NULL, 10) : 0.0;
    return 0;
}



// -----------------------------------------------------------------------------
/** This stores a string value from a query.

This is used with the EC_DB_STR_GETTER macro.

\note Since this allocates a new string, the caller is responsible for freeing it
*/
// -----------------------------------------------------------------------------
int set_string_cb(gpointer gp_char_p_ref, int num_cols, char **values, char **cols) {
    if (num_cols != 1) {
        handle_error(ERR_GENERIC_ERROR);
        fprintf(stderr, "-----> Unexpected num cols in set_double_cb\n");
        return 1;
    }

    gchar **char_p_ref = gp_char_p_ref;

    *char_p_ref = g_strdup(values[0]);
    return 0;
}


// -----------------------------------------------------------------------------
/** Creates a new string with parameters substituted
*/
// -----------------------------------------------------------------------------
static gchar *macro_substitute(const gchar *const_str) {
    GSequence *strings = g_sequence_new(g_free);

    gchar *str = g_strdup(const_str);
    guint index = 0;
    while(str[index]) {
        if (str[index] == '\'') str[index] = '"';
        index++;
    }

    // =================================
    // Scans string breaking at any `<digit> and performing a macro expansion
    // =================================
    index = 0;
    guint start_word = 0;
    guint stack_length = g_queue_get_length(_stack);

    while(str[index]) {
        // Split string if we hit a "`"
        if (str[index] == '`') {
            if (index > start_word) {
                g_sequence_append(strings, g_strndup(str+start_word, index - start_word));
            }

            guint stack_element = str[index+1] - '0';
            guint stack_index = stack_length - stack_element - 1;
            const Param *param = g_queue_peek_nth(_stack, stack_index);
            g_sequence_append(strings, g_strdup(param->val_string));
            index++;
            start_word = index + 1;
        }
        index++;
    }
    if (index > start_word) {
        g_sequence_append(strings, g_strndup(str+start_word, index-start_word));
    }
    g_free(str);


    // =================================
    // Figure out length of result string
    // =================================
    guint result_len = 0;
    gchar *fragment;
    for (GSequenceIter *iter=g_sequence_get_begin_iter(strings);
         !g_sequence_iter_is_end(iter);
         iter = g_sequence_iter_next(iter)) {

        fragment = g_sequence_get(iter);
        result_len += strlen(fragment);
    }
    gchar *result = g_malloc(result_len+1);


    // =================================
    // Copy strings into a result string
    // =================================
    gchar *start = result;
    for (GSequenceIter *iter=g_sequence_get_begin_iter(strings);
         !g_sequence_iter_is_end(iter);
         iter = g_sequence_iter_next(iter)) {

        start = g_stpcpy(start, g_sequence_get(iter));
    }

    g_sequence_free(strings);

    return result;
}



void execute_string(const gchar *str) {
    gchar *str_new = macro_substitute(str);
    scan_string(str_new);

    while(1) {
        // TODO: Make this into a function and share with kit.c
        Token token = get_token();

        if (token.type == EOF) break;
        if (token.type == '^') break;   // If EOS, we're done

        process_token(token);
    }
    g_free(str_new);
}


void process_token(Token token) {
    // If, executing...
    if (_mode == 'E') {
        Entry *entry = find_entry(token.word);
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
/** Executes a string with macro substitutions.

(str -- ?)
*/
// -----------------------------------------------------------------------------
static void EC_execute_string(gpointer gp_entry) {
    Param *param_string = pop_param();

    // Calling execute_string makes a copy of the specified string, so its OK to free it
    execute_string(param_string->val_string);
    free_param(param_string);
}



static void EC_negate(gpointer gp_entry) {
    Param *param_val = pop_param();

    if (param_val->type == 'I') {
        param_val->val_int *= -1;
    }
    else if (param_val->type == 'D') {
        param_val->val_double *= -1;
    }
    push_param(param_val);
}


static void EC_not(gpointer gp_entry) {
    Param *param = pop_param();

    gboolean result = 0;
    switch (param->type) {
        case 'I':
            result = param->val_int == 0 ? 1 : 0;
            break;

        case 'D':
            result = param->val_double == 0 ? 1 : 0;
            break;

        case 'S':
            result = (STR_EQ(param->val_string, "")) ? 1 : 0;
            break;

        default:
            handle_error(ERR_GENERIC_ERROR);
            fprintf(stderr, "-----> Can't 'not' type '%c'\n", param->type);
            goto done;
    }

    push_param(new_int_param(result));

done:
    free_param(param);
}


// -----------------------------------------------------------------------------
/** Defines the basic words in a Forth dictionary

### Interpreter control
- .q ( -- ) Quits the interpreter
- .i ( -- ) Accepts input from the user

### Stack words
- pop: ( -- ) Pops stack
- . ( -- ) Pops stack and prints value
- .s ( -- ) Prints the values on the stack (nondestructive)

### Constants and variables
- constant: (val -- ) Creates a constant
- variable: ( -- ) Creates a variable (see '!' and '@')
- ! (val variable -- ) Stores a value in a variable
- @ (variable -- ) Fetches the value of a variable

### Definitions
- : ( -- ) Starts a new definition
- ; ( -- ) Ends a definition
- .d (str -- ) Prints the words in a definition

### Branching
- if (immediate) Used during compile to define branching
- else (immediate) Used during compile to define branching
- then (immediate) Used during compile to define branching

*/
// -----------------------------------------------------------------------------
void add_basic_words() {
    Entry *entry;

    add_entry(".q")->routine = EC_quit;
    add_entry(".i")->routine = EC_interactive;

    add_entry(".")->routine = EC_print;
    add_entry(".s")->routine = EC_print_stack;
    add_entry("pop")->routine = EC_pop;
    add_entry("drop")->routine = EC_drop;
    add_entry("dup")->routine = EC_dup;

    // TODO: Move this to a math lexicon
    add_entry("negate")->routine = EC_negate;
    add_entry("not")->routine = EC_not;

    add_entry("constant")->routine = EC_constant;
    add_entry("variable")->routine = EC_variable;
    add_entry("!")->routine = EC_store_variable_value;
    add_entry("@")->routine = EC_fetch_variable_value;

    add_entry(",")->routine = EC_execute_string;

    add_entry(":")->routine = EC_define;

    entry = add_entry(";");
    entry->immediate = 1;
    entry->routine = EC_end_define;

    add_entry(".d")->routine = EC_print_definition;

    entry = add_entry("if");
    entry->immediate = 1;
    entry->routine = EC_if;

    entry = add_entry("else");
    entry->immediate = 1;
    entry->routine = EC_else;

    entry = add_entry("then");
    entry->immediate = 1;
    entry->routine = EC_then;
}
