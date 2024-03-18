// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/inferring/restriction-non-void.h"

#include "compiler/data/function-data.h"
#include "compiler/inferring/type-data.h"
#include "compiler/inferring/var-node.h"

RestrictionNonVoid::RestrictionNonVoid(tinf::Node *node)
  : node(node) {}

bool RestrictionNonVoid::is_restriction_broken() {
  return node->get_type()->ptype() == tp_void;
}

std::string RestrictionNonVoid::get_description() {
  const auto *as_var_node = dynamic_cast<const tinf::VarNode *>(node);
  if (as_var_node && as_var_node->is_return_value_from_function()) {
    return as_var_node->function_->as_human_readable() + " should return something, but it is void";
  }

  return "Expression " + node->get_description() + " is not allowed to be void";
}
