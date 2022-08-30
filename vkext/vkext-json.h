// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __VKEXT_JSON_H__
#define __VKEXT_JSON_H__

#include <stdbool.h>

#include "vkext/vk_zend.h"

bool vk_json_encode(zval *val);

#endif
