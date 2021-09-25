// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/common.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"

string gen_anonymous_scope_name(FunctionPtr parent_function);
string gen_anonymous_function_name(FunctionPtr parent_function);
string gen_unique_name(const string& prefix, FunctionPtr function = FunctionPtr{});
string gen_const_string_name(const string &str);
string gen_const_regexp_name(const string &str);
bool is_array_suitable_for_hashing(VertexPtr vertex);
string gen_const_array_name(const VertexAdaptor<op_array> &array);

std::string resolve_uses(FunctionPtr resolve_context, const std::string &class_name);
ClassPtr resolve_class_of_arrow_access(FunctionPtr function, VertexPtr lhs, VertexPtr v);
std::string resolve_static_method_append_context(FunctionPtr f, const std::string &prefix_name, const std::string &class_name, const std::string &local_name);
std::string resolve_define_name(std::string name);
