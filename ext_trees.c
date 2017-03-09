/** \file ext_trees.c
*/


#define TREE_TEE     "├"
#define TREE_VERT    "│"
#define TREE_END     "└"
#define TREE_HORIZ   "─"


#define MAX_FIELD_LEN  80

typedef struct {
    GSequence *root_items;
    GHashTable *children;    /**< \brief Maps parent ID to sequence of children */

    gchar id_field[MAX_FIELD_LEN];
    gchar parent_id_field[MAX_FIELD_LEN];
} Forest;


static Forest *copy_forest(Forest *src) {
    if (!src) return NULL;

    Forest *result = g_new(Forest, 1);
    *result = *src;
    return result;
}


static void free_forest_entry(gpointer gp_key, gpointer gp_value, gpointer unused) {
    gchar *key = gp_key;
    GSequence *value = gp_value;

    g_sequence_free(value);
    g_free(key);
}


static void free_forest(gpointer gp_forest) {
    Forest *forest = gp_forest;

    for (GSequenceIter *iter = g_sequence_get_begin_iter(forest->root_items);
         !g_sequence_iter_is_end(iter);
         iter = g_sequence_iter_next(iter)) {

        Param *item = g_sequence_get(iter);
        free_param(item);
    }

    g_sequence_free(forest->root_items);
    g_hash_table_foreach(forest->children, free_forest_entry, NULL);
    g_hash_table_destroy(forest->children);
    g_free(forest);
}



gchar *get_id(const gchar *id_field, Param *param) {
    gchar forth_string[MAX_FORTH_LEN];
    snprintf(forth_string, MAX_FORTH_LEN, "'%s' @field", id_field);

    // Extract id from object
    push_param(param);              // (obj -- )
    execute_string(forth_string);   // (obj val --)
    Param *param_val = pop_param(); // (obj -- )
    execute_string("drop");         // (obj val --)

    // Construct result
    gchar *result = NULL;
    gchar id_str[MAX_WORD_LEN];
    switch(param_val->type) {
        case 'S':
            result = g_strdup(param_val->val_string);
            break;

        case 'I':
            snprintf(id_str, MAX_WORD_LEN, "%ld", param_val->val_int);
            result = g_strdup(id_str);
            break;

        default:
            handle_error(ERR_GENERIC_ERROR);
            fprintf(stderr, "-----> Unknown id type: '%c'\n", param_val->type);
            break;
    }
    free_param(param_val);
    return result;
}


typedef struct {
    GHashTable *item_order;
    const gchar *id_field;
    const gchar *parent_id_field;
} ItemOrderInfo;


// -----------------------------------------------------------------------------
/** Comparator used to ensure that a sequence of items is inserted in the order
    that it was presented.
*/
// -----------------------------------------------------------------------------
gint compare_item_order(gconstpointer l, gconstpointer r, gpointer gp_item_order_info) {
    ItemOrderInfo *order_info = gp_item_order_info;

    GHashTable *item_order_hash = order_info->item_order;
    const Param *l_param = l;
    const Param *r_param = r;

    gchar *l_item_id = get_id(order_info->id_field, (Param *) l_param);
    gchar *r_item_id = get_id(order_info->id_field, (Param *) r_param);
    gint64 l_val = (gint64) g_hash_table_lookup(item_order_hash, (gpointer) l_item_id);
    gint64 r_val = (gint64) g_hash_table_lookup(item_order_hash, (gpointer) r_item_id);

    g_free(l_item_id);
    g_free(r_item_id);

    return l_val - r_val;
}


/** Converts a sequence to a forest
(sequence id-field parent-id-field -- forest)
*/
static void EC_forest(gpointer gp_entry) {
    Param *param_parent_id_field = pop_param();
    Param *param_id_field = pop_param();
    Param *param_sequence = pop_param();

    const gchar *parent_id_field = param_parent_id_field->val_string;
    const gchar *id_field = param_id_field->val_string;
    GSequence *sequence = param_sequence->val_custom;

    GHashTable *parent_children = g_hash_table_new(g_str_hash, g_str_equal);
    GHashTable *item_order = g_hash_table_new(g_str_hash, g_str_equal);

    // ---------------------------------
    // Iterate over all items and add them to an item hash, a children hash, and an order hash
    // ---------------------------------
    gint64 position = 0;
    for (GSequenceIter *iter=g_sequence_get_begin_iter(sequence);
         !g_sequence_iter_is_end(iter);
         iter = g_sequence_iter_next(iter), position++) {

        Param *param = g_sequence_get(iter);
        if (!param) continue;

        gchar *item_id = get_id(id_field, param);
        g_hash_table_insert(parent_children, (gpointer) item_id, g_sequence_new(free_param));
        g_hash_table_insert(item_order, (gpointer) item_id, (gpointer) position);
    }

    // ---------------------------------
    // Split the items into those whose parents are in the table and whose parents aren't
    // ---------------------------------
    GSequence *root_items = g_sequence_new(NULL);
    GSequence *non_root_items = g_sequence_new(NULL);

    for (GSequenceIter *iter=g_sequence_get_begin_iter(sequence);
         !g_sequence_iter_is_end(iter);
         iter = g_sequence_iter_next(iter)) {

        Param *item = g_sequence_get(iter);
        if (!item) continue;

        gchar *item_parent_id = get_id(parent_id_field, item);
        if (g_hash_table_contains(parent_children, (gpointer) item_parent_id)) {
            g_sequence_append(non_root_items, item);
        }
        else {
            g_sequence_append(root_items, item);
        }
        g_free(item_parent_id);
    }

    ItemOrderInfo order_info = {
        .item_order = item_order,
        .id_field = id_field,
        .parent_id_field = parent_id_field
    };

    // ---------------------------------
    // Iterate over the nonroot items and add them to the children table as children items
    // ---------------------------------
    for (GSequenceIter *iter=g_sequence_get_begin_iter(non_root_items);
         !g_sequence_iter_is_end(iter);
         iter = g_sequence_iter_next(iter)) {

        Param *item = g_sequence_get(iter);
        gchar *item_parent_id = get_id(parent_id_field, item);
        GSequence *children = g_hash_table_lookup(parent_children, (gpointer) item_parent_id);
        g_sequence_insert_sorted(children, item, compare_item_order, &order_info);
        g_free(item_parent_id);
    }

    // Push result
    Forest forest = {
        .children = parent_children,
        .root_items = root_items
    };
    g_strlcpy(forest.id_field, id_field, MAX_FIELD_LEN);
    g_strlcpy(forest.parent_id_field, parent_id_field, MAX_FIELD_LEN);

    Forest *result = copy_forest(&forest); 
    push_param(new_custom_param(result, "Forest", free_forest));


    // Clean up
    g_sequence_free(non_root_items);
    g_hash_table_destroy(item_order);
    free_param(param_sequence);
    free_param(param_id_field);
    free_param(param_parent_id_field);
}



static void print_hierarchy(FILE *file, Param *item, Forest *forest, gint level, gboolean is_last) {
    for (gint i=level-1; i >= 0; i--) {
        fprintf(file, "     ");
        if (i == 0) {
            if (is_last) {
                fprintf(file, TREE_END TREE_HORIZ TREE_HORIZ TREE_HORIZ TREE_HORIZ);
            }
            else {
                fprintf(file, TREE_TEE TREE_HORIZ TREE_HORIZ TREE_HORIZ TREE_HORIZ);
            }
        }
        else {
            fprintf(file, "     ");
        }
    }

    print_param(file, item);

    gchar *item_id = get_id(forest->id_field, item);
    GSequence *children = g_hash_table_lookup(forest->children, (gpointer) item_id);
    g_free(item_id);

    for (GSequenceIter *iter=g_sequence_get_begin_iter(children);
         !g_sequence_iter_is_end(iter);
         iter = g_sequence_iter_next(iter)) {

        Param *subitem = g_sequence_get(iter);
        gboolean is_last = g_sequence_iter_is_end(g_sequence_iter_next(iter));
        print_hierarchy(file, subitem, forest, level+1, is_last);
    }
}


static void print_forest(FILE *file, Param *param) {
    Forest *forest = param->val_custom;

    // ---------------------------------
    // Iterate over the root tasks, descending through the parent_child tree, printing each level
    // ---------------------------------
    fprintf(file, "\n");

    for (GSequenceIter *iter=g_sequence_get_begin_iter(forest->root_items);
         !g_sequence_iter_is_end(iter);
         iter = g_sequence_iter_next(iter)) {

        Param *item = g_sequence_get(iter);
        gboolean is_last = g_sequence_iter_is_end(g_sequence_iter_next(iter));
        print_hierarchy(file, item, forest, 0, is_last);
        fprintf(file, "\n");
    }
}


void EC_add_trees_lexicon(gpointer gp_entry) {
    add_entry("forest")->routine = EC_forest;
    add_print_function("Forest", print_forest);
}
