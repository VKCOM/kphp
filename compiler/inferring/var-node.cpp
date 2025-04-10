// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/inferring/var-node.h"

#include "compiler/data/function-data.h"
#include "compiler/data/var-data.h"
#include "compiler/inferring/edge.h"
#include "compiler/inferring/node-recalc.h"
#include "compiler/inferring/restriction-match-phpdoc.h"
#include "compiler/vertex.h"

class VarNodeRecalc : public NodeRecalc {
  static void on_restricted_type_mismatch(const tinf::Edge* edge, const TypeData* type_restriction);

public:
  VarNodeRecalc(tinf::VarNode* node, tinf::TypeInferer* inferer);
  void do_recalc() final;
  void on_new_type_became_tpError(const TypeData* because_of_type, const RValue& because_of_rvalue) final;
};

VarNodeRecalc::VarNodeRecalc(tinf::VarNode* node, tinf::TypeInferer* inferer)
    : NodeRecalc(node, inferer) {}

void VarNodeRecalc::do_recalc() {
  tinf::VarNode* node = dynamic_cast<tinf::VarNode*>(this->node_);

  if (inferer_->is_finished()) {
    // the main reason of this error is that you have forgotten add_node() in collect-main-edges
    kphp_error(0, fmt_format("called do_recalc on finished inferer, {}", node->get_description()));
    kphp_fail();
  }
  // at first, we just calculate new_type_ without any checks
  for (const tinf::Edge* e : node->get_edges_from_this()) {
    set_lca_at(e->from_at, as_rvalue(e->to));
    inferer_->add_node(e->to);
  }

  // then we check, whether a type restriction is satisfied
  // for example, if it's @param array, but passed string/tuple — new_type_ became mixed/error, not satisfied
  if (node->type_restriction) {
    bool satisfied = !new_type_->error_flag() && is_less_or_equal_type(new_type_, node->type_restriction);
    if (satisfied) {
      return;
    }

    // on type mismatch — rollback new_type_ to the start position and calculate it once again, checking every edge
    // when we find a mismatching edge, create a postponed check (which will trigger an error of course) and just skip it
    // so, passed string/tuple won't affect @param array, but will show a mismatch error after tinf finishes
    new_type_ = node->get_type()->clone();

    for (const tinf::Edge* e : node->get_edges_from_this()) {
      TypeData* before_type = new_type_->clone();
      set_lca_at(e->from_at, as_rvalue(e->to));

      satisfied = !new_type_->error_flag() && is_less_or_equal_type(new_type_, node->type_restriction);
      if (!satisfied) {
        //          fmt_print("rollback {} from {} to {} due to restriction {}\n", node->get_description(), colored_type_out(new_type_),
        //          colored_type_out(before_type), colored_type_out(node->type_restriction));
        on_restricted_type_mismatch(e, node->type_restriction);
        new_type_ = before_type->clone();
      }

      delete before_type;
    }
  }
}

// when new_type_ became tp_Error but a node is restricted with phpdoc (example: @param string, but passed Exception)
// — then don't print it out, just skip this fact, it will be handled just a type mismatch
void VarNodeRecalc::on_new_type_became_tpError(const TypeData* because_of_type, const RValue& because_of_rvalue) {
  tinf::VarNode* node = dynamic_cast<tinf::VarNode*>(this->node_);
  if (!node->type_restriction) {
    NodeRecalc::on_new_type_became_tpError(because_of_type, because_of_rvalue);
  }
}

// when a type restriction is broken (examples: 1) @param string, but passed int 2) @return int[], but actually return mixed)
// then we have an edge (edge->from is VarNode with restriction, edge->to is a mismatched actual node)
// this error will be shown after tinf finishes
void VarNodeRecalc::on_restricted_type_mismatch(const tinf::Edge* edge, const TypeData* type_restriction) {
  static std::mutex mutex;
  static std::vector<const tinf::Edge*> already_fired_errors;

  std::lock_guard<std::mutex> guard{mutex};
  for (const tinf::Edge* e : already_fired_errors) {
    if (e->from == edge->from && e->to == edge->to) {
      return;
    }
  }
  if (edge->from_at) {
    type_restriction = type_restriction->const_read_at(*edge->from_at);
  }
  tinf::get_inferer()->add_restriction(new RestrictionMatchPhpdoc(dynamic_cast<tinf::VarNode*>(edge->from), edge->to, type_restriction));
  already_fired_errors.emplace_back(edge);
}

void tinf::VarNode::recalc(tinf::TypeInferer* inferer) {
  kphp_assert(param_i != e_uninited);
  VarNodeRecalc(this, inferer).run();
}

std::string tinf::VarNode::get_description() {
  if (is_variable()) {
    std::string in_function_name = var_->is_global_var() ? " in global scope" : var_->holder_func ? " in " + var_->holder_func->as_human_readable() : "";
    return fmt_format("VarNode ({}{}) : {}", var_->as_human_readable(), in_function_name, tinf::get_type(var_)->as_human_readable());
  } else if (is_return_value_from_function()) {
    return fmt_format("VarNode (return of {}) : {}", function_->as_human_readable(), tinf::get_type(function_, -1)->as_human_readable());
  } else if (is_argument_of_function()) {
    return fmt_format("VarNode (arg{} {} of {}) : {}", var_->param_i, var_->as_human_readable(), var_->holder_func->as_human_readable(),
                      tinf::get_type(var_)->as_human_readable());
  } else {
    return "VarNode (strange)";
  }
}

const Location& tinf::VarNode::get_location() const {
  if (is_variable()) {
    return var_->init_val ? var_->init_val->location : var_->holder_func ? var_->holder_func->root->location : stage::get_location();
  } else if (is_argument_of_function()) {
    return var_->holder_func->root->location;
  } else {
    return function_->root->location;
  }
}

void tinf::VarNode::set_type_restriction(const TypeData* r) {
  kphp_assert(!type_restriction && "Setting type_restriction to VarNode more than once");
  type_restriction = r;
}

void tinf::VarNode::init_as_variable(VarPtr var) {
  var_ = var;
  param_i = e_variable;
}

void tinf::VarNode::init_as_argument(VarPtr var) {
  kphp_assert(var->param_i >= 0);
  var_ = var;
  param_i = var->param_i;
}

void tinf::VarNode::init_as_return_value(FunctionPtr function) {
  function_ = function;
  param_i = e_return_value;
}
