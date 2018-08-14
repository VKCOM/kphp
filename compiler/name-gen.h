#pragma once
#include "compiler/common.h"
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
bool is_array_suitable_for_hashing(VertexPtr vertex);
string gen_const_array_name(const VertexAdaptor<op_array> &array);

string resolve_uses (FunctionPtr current_function, string class_name, char delim = '$');
string resolve_constructor_fname (FunctionPtr current_function, VertexAdaptor <op_constructor_call> call);
string resolve_instance_fname (FunctionPtr function, VertexAdaptor <op_func_call> call);
ClassPtr resolve_expr_class (FunctionPtr function, VertexPtr v);
string get_context_by_prefix (FunctionPtr function, const string &class_name, char delim = '$');
string get_full_static_member_name (FunctionPtr function, const string &name, bool append_with_context = false);
string resolve_define_name (string name);

static inline string replace_characters(string s, char from, char to) {
  std::replace(s.begin(), s.end(), from, to);
  return s;
}

