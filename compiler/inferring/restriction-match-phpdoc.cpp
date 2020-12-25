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

RestrictionMatchPhpdoc::RestrictionMatchPhpdoc(tinf::VarNode *restricted_node, tinf::Node *actual_node, const TypeData *expected_type)
  : restricted_node(restricted_node)
  , actual_node(actual_node)
  , expected_type(expected_type) {

  if (auto *as_expr = dynamic_cast<tinf::ExprNode *>(actual_node)) {
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
  std::string expected_str = this->expected_type->as_human_readable();

  switch (detect_usage_context()) {

    case usage_function_argument: {
      VarPtr arg = restricted_node->var_;
      std::string arg_name = arg->get_human_readable_name();
      std::string function_name = arg->holder_func->get_human_readable_name();

      desc += "pass " + actual_str + " to " + TermStringFormat::add_text_attribute("argument " + arg_name + " of " + function_name, TermStringFormat::bold) + "\n";
      desc += "but it's declared as @param " + expected_str + "\n";
      break;
    }

    case usage_assign_to_variable: {
      std::string var_name = restricted_node->var_->get_human_readable_name();

      desc += "assign " + actual_str + " to " + TermStringFormat::add_text_attribute(var_name, TermStringFormat::bold) + "\n";
      desc += "but it's declared as @var " + expected_str + "\n";
      break;
    }

    case usage_assign_to_array_index: {
      std::string var_name = restricted_node->var_->get_human_readable_name();

      desc += "insert " + actual_str + " into " + TermStringFormat::add_text_attribute(var_name, TermStringFormat::bold) + "[]\n";
      desc += "but it's declared as @var " + expected_str + "\n";
      break;
    }

    case usage_return_from_function: {
      std::string function_name = restricted_node->function_->get_human_readable_name(false);

      desc += TermStringFormat::add_text_attribute("return ", TermStringFormat::bold) + actual_str + " from " + function_name + "\n";
      desc += "but it's declared as @return " + expected_str + "\n";
      break;
    }

    case usage_return_from_callback: {
      std::string function_name = restricted_node->function_->is_lambda() ? "lambda" : restricted_node->function_->get_human_readable_name(false);

      desc += TermStringFormat::add_text_attribute("return ", TermStringFormat::bold) + actual_str + " from " + function_name + "\n";
      desc += "but a callback was expected to return " + expected_str + "\n";
      break;
    }

    default: {
      desc += "some strange usage, couldn't detect\n";
      desc += "expected " + expected_str + ", actual " + actual_str + "\n";
      break;
    }
  }

  desc += "\nWhy? Follow this stacktrace from top to bottom:\n";

  RestrictionStacktraceFinder stacktrace_finder(actual_node, expected_type);
  desc += stacktrace_finder.get_stacktrace_text();

  return desc;
}

RestrictionMatchPhpdoc::UsageContext RestrictionMatchPhpdoc::detect_usage_context() {
  if (restricted_node->is_argument_of_function()) {
    // it may be a function call: function f(array $arg) {...} f(3)
    // or it may be modification of an argument inside the function: function f(array $v) { $v++ }
    // (it's undetectable, so an error description is for passing as a common case, for modification it looks strange)
    return usage_function_argument;
  }

  if (restricted_node->is_return_value_from_function()) {
    if (restricted_node == actual_node) {
      return usage_return_from_callback;
    }
    return usage_return_from_function;
  }

  if (restricted_node->is_variable()) {
    for (const tinf::Edge *e : actual_node->get_edges_to_this()) {
      if (e->from == restricted_node && e->from_at && !e->from_at->empty()) {
        return usage_assign_to_array_index;
      }
    }
    return usage_assign_to_variable;
  }

  return usage_other_strange;
}