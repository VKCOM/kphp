// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "vkext/vk_zend.h"

PHP_FUNCTION(vk_sp_simplify);
PHP_FUNCTION(vk_sp_full_simplify);
PHP_FUNCTION(vk_sp_deunicode);
PHP_FUNCTION(vk_sp_to_upper);
PHP_FUNCTION(vk_sp_to_lower);
PHP_FUNCTION(vk_sp_sort);
PHP_FUNCTION(vk_sp_remove_repeats);
PHP_FUNCTION(vk_sp_to_cyrillic);
PHP_FUNCTION(vk_sp_words_only);
