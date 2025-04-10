// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/inferring/restriction-match-phpdoc.h"

#include "common/termformat/termformat.h"

#include "compiler/data/function-data.h"
#include "compiler/data/var-data.h"
#include "compiler/inferring/edge.h"
#include "compiler/inferring/expr-node.h"
#include "compiler/inferring/public.h"
#include "compiler/inferring/restriction-stacktrace-finder.h"
#include "compiler/inferring/type-node.h"
#include "compiler/inferring/var-node.h"

// see comments in var-node.cpp and collect-main-edges.cpp

RestrictionMatchPhpdoc::RestrictionMatchPhpdoc(tinf::VarNode* restricted_node, tinf::Node* actual_node, const TypeData* expected_type)
    : restricted_node(restricted_node),
      actual_node(actual_node),
      expected_type(expected_type) {

  if (auto* as_expr = dynamic_cast<tinf::ExprNode*>(actual_node)) {
    location = as_expr->get_expr()->location;
  }
  // else will be stage location by default
}

bool RestrictionMatchPhpdoc::is_restriction_broken() {
  return !is_less_or_equal_type(actual_node->get_type(), expected_type);
}

std::string RestrictionMatchPhpdoc::get_description() {
  std::string desc;
  std::string actual_str = this->actual_node->get_type()->as_human_readable();
  std::string expected_str = this->expected_type->as_human_readable(false);

  switch (detect_usage_context()) {

  case usage_pass_to_argument: {
    VarPtr arg = restricted_node->var_;
    std::string arg_name = arg->as_human_readable();
    std::string function_name = arg->holder_func->as_human_readable();

    desc += "pass " + actual_str + " to argument " + arg_name + " of " + function_name + "\n";
    desc += "but it's declared as @param " + expected_str + "\n";
    break;
  }

  case usage_assign_to_variable: {
    std::string var_name = restricted_node->var_->as_human_readable();

    desc += "assign " + actual_str + " to " + var_name + "\n";
    desc += "but it's declared as @var " + expected_str + "\n";
    break;
  }

  case usage_assign_to_argument: {
    std::string arg_name = restricted_node->var_->as_human_readable();

    desc += "assign " + actual_str + " to " + arg_name + ", modifying a function argument" + "\n";
    desc += "but it's declared as @param " + expected_str + "\n";
    break;
  }

  case usage_assign_to_array_index: {
    std::string var_name = restricted_node->var_->as_human_readable();

    desc += "insert " + actual_str + " into " + var_name + "[]\n";
    desc += "but it's declared as @var " + expected_str + "\n";
    break;
  }

  case usage_return_from_function: {
    std::string function_name = restricted_node->function_->as_human_readable(false);

    desc += "return " + actual_str + " from " + function_name + "\n";
    desc += "but it's declared as @return " + expected_str + "\n";
    break;
  }

  case usage_return_from_callback: {
    std::string function_name = restricted_node->function_->as_human_readable(false);

    desc += "return " + actual_str + " from " + function_name + "\n";
    desc += "but a callback was expected to return " + expected_str + "\n";
    break;
  }

  default: {
    desc += "some strange usage, couldn't detect\n";
    desc += "expected " + expected_str + ", actual " + actual_str + "\n";
    break;
  }
  }

  RestrictionStacktraceFinder stacktrace_finder(actual_node, expected_type);
  desc += stacktrace_finder.get_stacktrace_text();

  return desc;
}

RestrictionMatchPhpdoc::UsageContext RestrictionMatchPhpdoc::detect_usage_context() {
  if (restricted_node->is_argument_of_function()) {
    // it may be a function call: function f(array $arg) {...} f(3)
    // or it may be modification of an argument inside the function: function f(array $v) { $v++ }
    // this heuristics seems to work unless a function calls itself recursively
    const auto* as_expr_node = dynamic_cast<tinf::ExprNode*>(actual_node);
    if (as_expr_node && as_expr_node->get_expr()->location.function == restricted_node->var_->holder_func) {
      return usage_assign_to_argument;
    }
    return usage_pass_to_argument;
  }

  if (restricted_node->is_return_value_from_function()) {
    if (restricted_node == actual_node) {
      return usage_return_from_callback;
    }
    return usage_return_from_function;
  }

  if (restricted_node->is_variable()) {
    for (const tinf::Edge* e : actual_node->get_edges_to_this()) {
      if (e->from == restricted_node && e->from_at && !e->from_at->empty()) {
        return usage_assign_to_array_index;
      }
    }
    return usage_assign_to_variable;
  }

  return usage_other_strange;
}