#pragma once

#include "compiler/common.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"

string gen_anonymous_function_name(FunctionPtr parent_function);
bool is_anonymous_function_name(vk::string_view name);
string gen_unique_name(const string& prefix, FunctionPtr function = FunctionPtr{});
string gen_const_string_name(const string &str);
string gen_const_regexp_name(const string &str);
inline long long array_hash(VertexPtr vertex);
bool is_array_suitable_for_hashing(VertexPtr vertex);
string gen_const_array_name(const VertexAdaptor<op_array> &array);

string resolve_uses(FunctionPtr current_function, string class_name, char delim = '$');
string resolve_constructor_func_name(FunctionPtr function, VertexAdaptor<op_constructor_call> ctor_call);
string resolve_instance_func_name(FunctionPtr function, VertexAdaptor<op_func_call> arrow_call);
ClassPtr resolve_class_of_arrow_access(FunctionPtr function, VertexPtr v);
string get_context_by_prefix(FunctionPtr function, const string &class_name, char delim = '$');
string get_full_static_member_name(FunctionPtr function, const string &name, bool append_with_context = false);
string resolve_define_name(string name);
