/** \file return_stack.h
*/

#pragma once

void push_param_r(GSequenceIter *param_iter);
GSequenceIter *pop_param_r();

void create_stack_r();
void clear_stack_r();
void destroy_stack_r();
