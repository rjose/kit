/** \file stack.h
*/

#pragma once

void push_param(Param *param);
Param *pop_param();
const Param *top();

void create_stack();
void clear_stack();
void destroy_stack();
