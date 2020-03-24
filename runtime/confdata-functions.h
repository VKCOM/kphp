#pragma once
#include "runtime/kphp_core.h"

void init_confdata_functions_lib();
void free_confdata_functions_lib();


var f$confdata_get_value(const string &key) noexcept;

array<var> f$confdata_get_values_by_wildcard(string wildcard) noexcept;
