// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/inferring/restriction-stacktrace-finder.h"

#include "common/algorithms/contains.h"
#include "common/algorithms/string-algorithms.h"
#include "common/termformat/termformat.h"

#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/inferring/edge.h"
#include "compiler/inferring/public.h"
#include "compiler/inferring/type-node.h"
#include "compiler/inferring/var-node.h"
#include "compiler/type-hint.h"
#include "compiler/vertex-util.h"
#include "compiler/vertex.h"

/*
    This module finds a stacktrace to describe, why a type mismatch occurred.
    For example:
      function acInt(float $x) {}
      $f = [1]; $a = $f; acInt($a);
    Type mismatch is passing int[] to argument $x of acInt()
    This stacktrace finder answers this exact question: "Why is it int[], but not float?"
    The answer is: call acInt(int[]), because $a is int[], because $f is int[], because [1] is int[], because 1 is int.

    Some known issues, that can't be overcome:
    * as all constants are inlined, it will never point to the constant/define, only to the inlined value
    * equal strings/arrays are extracted as one const variable, which location could seem strange for a particular stacktrace
    * when mixed is a result of non-mixed operands, we actually trace into only one operand we find the most important
 */

RestrictionStacktraceFinder::RestrictionStacktraceFinder(tinf::Node *cur_node, const TypeData *expected_type) {
  assert(cur_node != nullptr);

  find_call_trace_with_error(cur_node, expected_type);

  stacktrace.push_back(cur_node);
  std::reverse(stacktrace.begin(), stacktrace.end());
}

VarPtr RestrictionStacktraceFinder::get_var_id_from_node(const tinf::Node *node) {
  if (const auto *as_var_node = dynamic_cast<const tinf::VarNode *>(node)) {
    return as_var_node->var_;
  }
  if (const auto *as_expr_node = dynamic_cast<const tinf::ExprNode *>(node)) {
    if (as_expr_node->get_expr()->type() == op_var) {
      return as_expr_node->get_expr().as<op_var>()->var_id;
    }
  }
  return {};
}

// returns, how important the 'to' node is
// if all types are equal, sorting will be done by this importance
// for example, when we are searching for why $a is float, and we have { $a = 4.5; $a = $v; } and $v is float,
// then $v is a more important reason than 4.5, as we want to see, why $v also occurred float, not just stop at 4.5
// so, a variable is more important than a constant, a func call is more important that assigning an array, and other heuristics
int RestrictionStacktraceFinder::get_importance_of_reason(const tinf::Node *from, const tinf::Node *to) {
  if (dynamic_cast<const tinf::TypeNode *>(to)) {
    return 0;
  }
  if (dynamic_cast<const tinf::VarNode *>(to)) {
    return 0;
  }

  VertexPtr expr = VertexUtil::get_actual_value(dynamic_cast<const tinf::ExprNode *>(to)->get_expr());
  switch (expr->type()) {
    case op_var:
      return get_var_id_from_node(to) == get_var_id_from_node(from) ? 4 : 5;
    case op_func_call:
      return expr->get_string() == "make_clone" ? 4 : 3;
    case op_array:
      return 2;
    case op_float_const:
    case op_int_const:
    case op_string:
    case op_true:
    case op_false:
    case op_null:
      return 0;
    default:
      return 1;
  }
}

// when there are many edges pointing from 'from' to different 'to', these edges are sorted by priority
// then we continue dfs starting from the highest priority
// this priority is now well this edge answers the question "how did from->type occur, why is it not expected_type?"
// for example, if from is mixed and to is mixed, it's the best answer: further we'll have to find out why to is mixed
int RestrictionStacktraceFinder::get_priority(const tinf::Edge *edge, const TypeData *expected_type) {
  const TypeData *from_type = edge->from->get_type();
  const TypeData *to_type = edge->to->get_type();

  int importance = get_importance_of_reason(edge->from, edge->to);

  return are_equal_types(to_type, from_type)                             ? 300 + importance
         : !is_less_or_equal_type(to_type, expected_type, edge->from_at) ? 200 + importance
         : is_less_or_equal_type(to_type, from_type, edge->from_at)      ? 100 + importance
                                                                         : importance;
}

// this is the main function to fill the stacktrace field, it's done recursively
// on each step, we have all edges pointing from cur_node and sort them by priority
// then take the highest priority and find stacktrace pointing to it
bool RestrictionStacktraceFinder::find_call_trace_with_error(tinf::Node *cur_node, const TypeData *expected_type) {
  static int limit_calls = 1000000;
  if (--limit_calls <= 0) {
    return true;
  }

  std::vector<const tinf::Edge *> ordered_edges;
  for (const tinf::Edge *e : cur_node->get_edges_from_this()) {
    ordered_edges.emplace_back(e);
  }
  std::sort(ordered_edges.begin(), ordered_edges.end(),
            [&](const tinf::Edge *a, const tinf::Edge *b) { return get_priority(a, expected_type) > get_priority(b, expected_type); });

  if (ordered_edges.empty()) {
    return true;
  }

  for (const tinf::Edge *e : ordered_edges) {
    tinf::Node *to = e->to;
    kphp_assert(e->from == cur_node && e->to != cur_node);

    if (vk::contains(node_path, to) && dynamic_cast<tinf::VarNode *>(to) == nullptr) {
      continue;
    }
    if (node_path.size() == max_cnt_nodes_in_path) {
      return false;
    }

    node_path.push_back(e->to);

    const TypeData *next_expected_type{nullptr};
    if (e->from_at) { // inside arrays/tuples
      next_expected_type = expected_type->const_read_at(*e->from_at);
    } else if (auto *as_expr_node = dynamic_cast<tinf::ExprNode *>(e->from)) {
      if (as_expr_node->get_expr()->type() == op_index) {              // outside arrays
        next_expected_type = TypeData::create_array_of(expected_type); // hard to explain, comment to see a failing test
      }
    }
    if (find_call_trace_with_error(to, next_expected_type ?: expected_type)) {
      node_path.pop_back();
      stacktrace.push_back(to);
      return true;
    }

    node_path.pop_back();
  }

  return false;
}

// this functions converts a filled stacktrace into a human readable colorful format
// if you wish to debug raw nodes in stacktrace, without skipping and prettifying, uncomment lines below
std::string RestrictionStacktraceFinder::get_stacktrace_text() {
  std::string desc;

  // to see full development stacktrace, uncomment these lines
  //  for (tinf::Node *node : stacktrace)
  //    desc += node->get_description() + "\n";
  //  return desc;

  // here we make stacktrace pretty and human-readable:
  // don't use internal get_description(), join some combinations of ExprNode+VarNode, skip some technical nodes, etc

  const tinf::Node *prev{nullptr};

  auto append_current_php_line_if_changed = [&prev, &desc](const tinf::Node *cur_node) {
    const Location &loc = cur_node->get_location();
    if (loc.line <= 0 || !loc.file) {
      desc += "    ";
      return;
    }
    if (prev && prev->get_location().function == loc.function && prev->get_location().line == loc.line) {
      desc += "    ";
      return;
    }

    std::string comment = static_cast<std::string>(loc.file->get_line(loc.line));
    comment = vk::trim(vk::replace_all(comment, "\n", "\\n"));
    desc += "\n  ";
    desc += loc.as_human_readable();
    desc += "\n    ";
    desc += TermStringFormat::paint(comment, TermStringFormat::yellow);
    desc += "\n    ";
  };

  for (const tinf::Node *node : stacktrace) {

    if (const auto *var_node = dynamic_cast<const tinf::VarNode *>(node)) {
      if (var_node->is_argument_of_function()) {
        FunctionPtr function = var_node->var_->holder_func;
        auto param = function->get_params()[var_node->param_i].as<op_func_param>();
        append_current_php_line_if_changed(var_node);
        desc += "argument " + var_node->var_->as_human_readable();
        desc += param->type_hint && !param->type_hint->has_tp_any_inside() ? " declared as " : " inferred as ";
        desc += tinf::get_type(function, var_node->param_i)->as_human_readable();
        desc += "\n";
      } else if (var_node->is_return_value_from_function()) {
        FunctionPtr function = var_node->function_;
        append_current_php_line_if_changed(var_node);
        desc += function->as_human_readable();
        desc += function->return_typehint && !function->return_typehint->has_tp_any_inside() ? " declared that returns " : " inferred that returns ";
        desc += tinf::get_type(function, -1)->as_human_readable();
        desc += "\n";
      } else {
        continue;
      }

    } else if (const auto *expr_node = dynamic_cast<const tinf::ExprNode *>(node)) {
      VertexPtr expr = expr_node->get_expr();
      bool skip = (expr->type() == op_func_call && expr.as<op_func_call>()->func_id->name == "make_clone")
                  || (expr->type() == op_return && expr.as<op_return>()->has_expr())
                  || (expr->type() == op_var && expr.as<op_var>()->extra_type == OperationExtra::op_ex_var_const);
      if (skip) {
        continue;
      }

      append_current_php_line_if_changed(expr_node);
      desc += expr_node->get_expr_human_readable();
      desc += expr->type() == op_func_call ? " returns " : " is ";
      desc += expr_node->get_type()->as_human_readable();
      desc += "\n";

      bool stop = (expr->type() == op_func_call && expr.as<op_func_call>()->func_id->is_constructor());
      if (stop) {
        break;
      }

    } else if (const auto *type_node = dynamic_cast<const tinf::TypeNode *>(node)) {
      const auto *prev_as_var = dynamic_cast<const tinf::VarNode *>(prev);
      bool skip = prev_as_var && prev_as_var->type_restriction && are_equal_types(prev_as_var->type_restriction, type_node->get_type());
      if (skip) { // then a line above is "... declared that returns T", see create_type_assign_with_restriction()
        continue;
      }

      append_current_php_line_if_changed(type_node);
      desc += "implicitly casted to " + type_node->get_type()->as_human_readable();
      desc += "\n";
      // if TypeNode is in stacktrace, it's the last one
    }

    prev = node;
  }

  return desc;
}
