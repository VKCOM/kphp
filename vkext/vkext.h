// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __VKEXT_H__
#define __VKEXT_H__

#include "vkext/vk_zend.h"

#define VKEXT_VERSION "1.02"

#define VKEXT_NAME "vk_extension"

#define USE_ZEND_ALLOC 1

PHP_MINIT_FUNCTION(vkext);
PHP_RINIT_FUNCTION(vkext);
PHP_MSHUTDOWN_FUNCTION(vkext);
PHP_RSHUTDOWN_FUNCTION(vkext);

#define PHP_WARNING(t) (NULL, E_WARNING, t);
#define PHP_ERROR(t) (NULL, E_ERROR, t);
#define PHP_NOTICE(t) (NULL, E_NOTICE, t);

extern zend_module_entry vkext_module_entry;
#define phpext_vkext_ptr &vkext_module_ptr

void write_buff(const char *s, int l);
void write_buff_char(char c);
void write_buff_long(long x);
void write_buff_double(double x);
void print_backtrace();
#endif
