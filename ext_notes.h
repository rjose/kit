/** \file ext_notes.h
*/

#pragma once

void free_note(gpointer gp_note);
gpointer copy_note_gp(gpointer gp_note);
GSequence *select_notes(const gchar *sql_query);
void EC_add_notes_lexicon(gpointer gp_entry);
