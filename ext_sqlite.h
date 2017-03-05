/** \file ext_sqlite.h
*/

#pragma once

void EC_add_sqlite_lexicon(gpointer gp_entry);
const gchar *sql_execute(sqlite3* connection, const gchar *query);
GSequence *sql_select(sqlite3* connection, const gchar *query, gchar **error_message_p);
