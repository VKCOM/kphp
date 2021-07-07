// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __VKEXT_FLEX_H__
#define __VKEXT_FLEX_H__

#include <cstddef>

char *do_flex(const char *name, size_t name_len, const char *case_name, size_t case_name_len, bool sex, const char *type, size_t type_len, int lang_id);
#endif
