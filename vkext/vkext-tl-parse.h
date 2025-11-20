// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __VKEXT_TL_PARSE_H__
#define __VKEXT_TL_PARSE_H__

#include <string>

void tl_parse_init();
int tl_parse_int();
int tl_parse_byte();
long long tl_parse_long();
double tl_parse_double();
float tl_parse_float();
int tl_parse_string(char **s);
int tl_eparse_string(char **s);
std::string tl_parse_string();
char *tl_parse_error();
void tl_set_error(const char *error);
void tl_parse_end();
int tl_parse_save_pos();
int tl_parse_restore_pos(int pos);
void tl_parse_clear_error();
#endif
