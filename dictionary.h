/** \file dictionary.h
*/

#pragma once

void build_dictionary();
Entry *add_entry(const gchar *word);
Entry* find_entry(const gchar* word);
Entry *latest_entry();
void destroy_dictionary();
