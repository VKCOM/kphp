#pragma once

#include <string>

#include "compiler/class-members.h"
#include "compiler/data_ptr.h"

class FunctionInfo {
public:
  VertexPtr root;
  std::string namespace_name;
  std::string class_context;
  bool kphp_required;
  AccessType access_type;

  FunctionInfo() :
    kphp_required(false),
    access_type(access_nonmember) {}

  FunctionInfo(VertexPtr root, string namespace_name, string class_context,
               bool kphp_required, AccessType access_type) :
    root(root),
    namespace_name(std::move(namespace_name)),
    class_context(std::move(class_context)),
    kphp_required(kphp_required),
    access_type(access_type) {}
};
