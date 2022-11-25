#pragma once

#include "runtime/kphp_core.h"
#include <regex>
#include <iostream>
#include <string>

/*
 * Helper functions
 */

string clearQuotes(const string &str);

string clearSingleQuotes(const string &str);

string clearDoubleQuotes(const string &str);

string clearExtraSpaces(const string &str);

string tolower(const string &str);

string trim(const string &str);

/*
 * INI parsing functions
 */

const int INI_SCANNER_NORMAL = 0;
const int INI_SCANNER_RAW = 1;
const int INI_SCANNER_TYPED = 2;

string get_ini_section(const string &ini_entry);

string get_ini_var(const string &ini_entry);

string get_ini_val(const string &ini_entry);

bool cast_str_to_bool(const string &ini_var);

array<mixed> f$parse_ini_string(const string &ini_string, bool process_sections = false, int scanner_mode = INI_SCANNER_NORMAL);
