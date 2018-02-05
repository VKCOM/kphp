#pragma once
#include "PHP/common.h"
#include "compiler/data_ptr.h"

int hash (const string &s);
//string next_const_string_name (const string &str);
//string next_name (const string &prefix);
string gen_shorthand_ternary_name ();
string gen_anonymous_function_name ();
string gen_unique_name (string prefix, bool flag = false);
string gen_const_string_name (const string &str);
string gen_const_regexp_name (const string &str);
inline long long array_hash(VertexPtr vertex);

string resolve_uses(FunctionPtr current_function, string class_name, char delim = '$');
string get_context_by_prefix(FunctionPtr function, string const &class_name, char delim = '$');
string get_full_static_member_name(FunctionPtr function, string const &name, bool append_with_context = false);
string resolve_define_name(string name);

static inline string replace_characters(string s, char from, char to) {
  for (size_t i = 0; i < s.length(); i++) {
    if (s[i] == from) {
      s[i] = to;
    }
  }
  return s;
}

