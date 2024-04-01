// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "vkext/vk_zend.h"

void vkext_reset_error();
void vkext_error(int, const char *s);
__attribute__((format(printf, 2, 3))) void vkext_error_format(int, const char *format, ...);
void vkext_get_errors(zval *);

#define VKEXT_ERROR_UNKNOWN -101
#define VKEXT_ERROR_NETWORK -102
#define VKEXT_ERROR_CONSTRUCTION -103
#define VKEXT_ERROR_INVALID_ARRAY_SIZE -104
#define VKEXT_ERROR_INVALID_CALL -105
#define VKEXT_ERROR_INVALID_CONSTRUCTOR -106
#define VKEXT_ERROR_NO_CONSTRUCTOR -107
#define VKEXT_ERROR_NO_FIELD -108
#define VKEXT_ERROR_NOT_ENOUGH_MEMORY -109
#define VKEXT_ERROR_UNKNOWN_CONSTRUCTOR -110
#define VKEXT_ERROR_UNKNOWN_TYPE -111
#define VKEXT_ERROR_WRONG_TYPE -112
#define VKEXT_ERROR_WRONG_VALUE -113
#define VKEXT_ERROR_TO_MANY_QUERIES -114
