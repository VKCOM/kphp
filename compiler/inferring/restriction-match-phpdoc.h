// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/inferring/multi-key.h"
#include "compiler/inferring/restriction-base.h"
#include "compiler/inferring/var-node.h"

class RestrictionMatchPhpdoc : public tinf::RestrictionBase {
  tinf::VarNode *restricted_node;
  tinf::Node *actual_node;
  const TypeData *expected_type;

  // this enum is used for better error descriptions
  enum UsageContext {
    usage_pass_to_argument,
    usage_assign_to_variable,
    usage_assign_to_argument,
    usage_assign_to_array_index,
    usage_return_from_function,
    usage_return_from_callback,
    usage_other_strange,
  };

  UsageContext detect_usage_context();

public:
  RestrictionMatchPhpdoc(tinf::VarNode *restricted_node, tinf::Node *actual_node, const TypeData *expected_type);

  bool is_restriction_broken() final;
  std::string get_description() final;
};
