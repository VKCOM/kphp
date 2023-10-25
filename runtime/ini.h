#pragma once

#include "runtime/kphp_core.h"
#include "runtime/string_functions.h"
#include "runtime/array_functions.h"
#include "runtime/streams.h"
#include "runtime/regexp.h"

const int INI_SCANNER_NORMAL = 0;
const int INI_SCANNER_RAW = 1;
const int INI_SCANNER_TYPED = 2;

array<mixed> f$parse_ini_string(const string &ini_string, bool process_sections = false, int scanner_mode = 0);

array<mixed> f$parse_ini_file(const string &filename, bool process_sections = false, int scanner_mode = 0);
