#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "drinkless/dl-utils-lite.h"

#define dl_pstr don_t_use_dl_pstr_use_format

#include "common-php-functions.h"

size_t total_mem_used __attribute__ ((weak));

using std::vector;
using std::map;
using std::set;
using std::string;

bool use_safe_integer_arithmetic __attribute__ ((weak)) = false;

