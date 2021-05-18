// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/common.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"

string gen_anonymous_function_name(FunctionPtr parent_function);
bool is_anonymous_function_name(vk::string_view name);
string gen_unique_name(const string& prefix, FunctionPtr function = FunctionPtr{});
string gen_const_string_name(const string &str);
string gen_const_regexp_name(const string &str);
bool is_array_suitable_for_hashing(VertexPtr vertex);
string gen_const_array_name(const VertexAdaptor<op_array> &array);
std::string gen_define_name(SrcFilePtr context, VertexAdaptor<op_define> def);

string resolve_uses(FunctionPtr resolve_context, string class_name, char delim);
string resolve_instance_func_name(FunctionPtr function, VertexAdaptor<op_func_call> arrow_call);
ClassPtr resolve_class_of_arrow_access(FunctionPtr function, VertexPtr v);
string get_context_by_prefix(FunctionPtr function, const string &class_name, char delim = '$');
string get_full_static_member_name(FunctionPtr function, const string &name, bool append_with_context = false);

struct ResolveSymbolResult {
  enum class Kind {
    ClassMember, // always precise, no need to make it FQN
    ResolvedName, // needs a leading `\` to become FQN
    UnqualifiedName, // needs further resolution
  };

  std::string value;
  Kind kind;

  bool is_class_member() { return kind == Kind::ClassMember; }
  bool is_resolved() { return kind == Kind::ResolvedName; }
  bool is_unqualified() { return kind == Kind::UnqualifiedName; }
};

ResolveSymbolResult resolve_func_name(FunctionPtr context, VertexPtr call, char delim = '$');
ResolveSymbolResult resolve_define_name(SrcFilePtr context, const string &const_name, char delim = '$');
