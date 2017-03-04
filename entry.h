/** \file entry.h
*/

#pragma once



Entry *new_entry();
void add_entry_param(Entry *entry, Param *param);
void execute(gpointer entry);
void compile(Token token);
void free_entry(gpointer entry);
