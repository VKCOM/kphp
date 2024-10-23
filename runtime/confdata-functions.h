// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include "runtime-common/runtime-core/runtime-core.h"

void init_confdata_functions_lib();
void free_confdata_functions_lib();


bool f$is_confdata_loaded() noexcept;

mixed f$confdata_get_value(const string &key) noexcept;

array<mixed> f$confdata_get_values_by_any_wildcard(const string &wildcard) noexcept;

array<mixed> f$confdata_get_values_by_predefined_wildcard(const string &wildcard) noexcept;
