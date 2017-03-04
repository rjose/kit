/** \file dictionary.c

\brief Defines functions for manipulating the global Forth dictionary: _dictionary.

A Dictionary is just a GList of Entry objects. Each Entry is added to the end
of the _dictionary. Searches for an Entry start at the end and move backwards.
This allows older entries to be overridden.

The basic dictionary is built using build_dictionary. This should be functional
as a control language. Any extensions to the dictionary should be done via
a word that can load new entries.

*/


// -----------------------------------------------------------------------------
/** Searches for an entry in the dictionary, starting from the most recent entry
and moving backwards.

\param word: The string to search for
\returns A pointer to the entry or NULL if not found
*/
// -----------------------------------------------------------------------------
Entry* find_entry(const gchar* word) {
    for (GList* l=g_list_last(_dictionary); l != NULL; l = l->prev) {
        Entry *entry = l->data;
        if (entry->complete && g_strcmp0(word, entry->word) == 0) return entry;
    }
    return NULL;
}


// -----------------------------------------------------------------------------
/** Allocates new Entry, adds it to the dictionary, and returns it.

\param word: The word to use for the new entry
\returns The newly created entry
*/
// -----------------------------------------------------------------------------
Entry *add_entry(const gchar *word) {
    Entry *result = new_entry();
    g_strlcpy(result->word, word, MAX_WORD_LEN);
    _dictionary = g_list_append(_dictionary, result);
    return result;
}


// -----------------------------------------------------------------------------
/** Adds words to add various lexicons to the dictionary.

*/
// -----------------------------------------------------------------------------
static void hook_up_extensions() {
    //add_entry("lex-sequence")->routine = EC_add_sequence_lexicon;
    //add_entry("lex-sqlite")->routine = EC_add_sqlite_lexicon;
    //add_entry("lex-notes")->routine = EC_add_notes_lexicon;
    //add_entry("lex-tasks")->routine = EC_add_tasks_lexicon;
    //add_entry("lex-root-cause")->routine = EC_add_root_cause_lexicon;
}



// -----------------------------------------------------------------------------
/** Builds the dictionary for the interpreter.

This defines the basic words for the interpreter and will allow loading
of custom extensions for various applications. This is TBD, but the intent is
that we can control the extensions dynamically.

\note Entries are new'd and so it's appropriate to do a g_list_free_full
      if the dictionary should be rebuilt.
*/
// -----------------------------------------------------------------------------
void build_dictionary() {
    add_basic_words();
    hook_up_extensions();
}


// -----------------------------------------------------------------------------
/** Returns the most recently added entry in the dictionary.

During compilation of a definition, that definition will be the
latest entry.
*/
// -----------------------------------------------------------------------------
Entry *latest_entry() {
    Entry *result = g_list_last(_dictionary)->data;
    return result;
}


// -----------------------------------------------------------------------------
/** Deallocates dictionary and all of its entries.
*/
// -----------------------------------------------------------------------------
void destroy_dictionary() {
    g_list_free_full(_dictionary, free_entry);
}
