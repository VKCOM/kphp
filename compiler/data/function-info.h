#pragma once

#include <string>

#include "compiler/data/class-members.h"
#include "compiler/data/data_ptr.h"
#include "auto/compiler/vertex/vertex-meta_op_function.h"

class FunctionInfo {
public:
  VertexAdaptor<meta_op_function> root;
  std::string namespace_name;
  ClassPtr context_class;
  bool kphp_required;
  AccessType access_type;

  FunctionInfo() :
    kphp_required(false),
    access_type(access_nonmember) {}

  FunctionInfo(VertexAdaptor<meta_op_function> root, string namespace_name, ClassPtr context_class,
               bool kphp_required, AccessType access_type) :
    root(root),
    namespace_name(std::move(namespace_name)),
    context_class(context_class),
    kphp_required(kphp_required),
    access_type(access_type) {}
};
