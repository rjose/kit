/** \file ext_sequence.h
*/

#pragma once

void EC_add_sequence_lexicon(gpointer gp_entry);
void print_seq(FILE *file, Param *param);
void free_seq(gpointer gp_seq);
gpointer copy_seq(gpointer gp_seq);
