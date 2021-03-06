/** \file param.h
*/

#pragma once

typedef void (*print_param_func)(FILE *file, Param *param);


Param *new_param();
void copy_param(Param *dst, const Param *src);
void free_param(gpointer param);

void free_nop(gpointer val_custom);
gpointer copy_nop(gpointer val_custom);

Param *new_int_param(gint64 val_int);
Param *new_double_param(gdouble val_double);
Param *new_str_param(const gchar *str);
Param *new_entry_param(Entry *val_entry);
Param *new_routine_param(routine_ptr val_routine);
Param *new_pseudo_entry_param(const gchar *word, routine_ptr routine);
Param *new_custom_param(gpointer val_custom, const gchar *custom_type,
                        free_custom_val_ptr free_custom,
                        copy_custom_val_ptr copy_custom);


void create_print_functions();
void add_print_function(const gchar *type_name, print_param_func func);
void destroy_print_functions();
void print_param(FILE *f, Param *param);
