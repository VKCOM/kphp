#pragma once
#include "PHP/common.h"

int hash (const string &s);
//string next_const_string_name (const string &str);
//string next_name (const string &prefix);
string gen_shorthand_ternary_name ();
string gen_anonymous_functinon_name ();
string gen_unique_name (string prefix, bool flag = false);
string gen_const_string_name (const string &str);
string gen_const_regexp_name (const string &str);
