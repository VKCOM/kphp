#pragma once
#include "runtime/kphp_core.h"

void init_confdata_functions_lib();
void free_confdata_functions_lib();


bool f$is_confdata_loaded() noexcept;

var f$confdata_get_value(const string &key) noexcept;

array<var> f$confdata_get_values_by_any_wildcard(const string &wildcard) noexcept;

array<var> f$confdata_get_values_by_predefined_wildcard(const string &wildcard) noexcept;
