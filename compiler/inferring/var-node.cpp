#include "compiler/inferring/var-node.h"

#include <sstream>

#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/var-data.h"
#include "compiler/inferring/edge.h"
#include "compiler/inferring/node-recalc.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"

class VarNodeRecalc : public NodeRecalc {
public:
  VarNodeRecalc(tinf::VarNode *node, tinf::TypeInferer *inferer);
  void do_recalc();
};

VarNodeRecalc::VarNodeRecalc(tinf::VarNode *node, tinf::TypeInferer *inferer) :
  NodeRecalc(node, inferer) {
}

void VarNodeRecalc::do_recalc() {
  if (inferer_->is_finished()) {
    kphp_error (0, format("%s: %d\n", "", node_->recalc_cnt_));
    kphp_fail();
  }
  for (auto e : node_->get_next()) {
    set_lca_at(e->from_at, as_rvalue(e->to));
    inferer_->add_node(e->to);
  }
}

void tinf::VarNode::recalc(tinf::TypeInferer *inferer) {
  VarNodeRecalc(this, inferer).run();
}

string tinf::VarNode::get_var_name() {
  return (var_ ? var_->get_human_readable_name() : "$STRANGE_VAR");
}

string tinf::VarNode::get_function_name() {
  if (!function_) {
    if (var_ && var_->holder_func) {
      function_ = var_->holder_func;
    }
  }
  if (function_) {
    return string(function_->is_static_function() ? "static " : "") + "function: " + function_->get_human_readable_name();
  }
  if (var_->is_global_var()) {
    return "global scope";
  }
  if (var_->is_class_instance_var()) {
    return string("class ") + var_->class_id->name + " : " + std::to_string(var_->as_class_instance_field()->root->location.line);
  }
  if (var_->is_class_static_var()) {
    return string("class ") + var_->class_id->name + " : " + std::to_string(var_->as_class_static_field()->root->location.line);
  }
  return "";
}

string tinf::VarNode::get_var_as_argument_name() {
  int actual_num = (function_ && function_->has_implicit_this_arg() ? param_i - 1 : param_i);
  return "arg #" + std::to_string(actual_num) + " (" + get_var_name() + ")";
}

string tinf::VarNode::get_description() {
  std::stringstream ss;
  if (is_variable()) {
    //Вывод должен совпадать с выводом в соответсвующей ветке в get_expr_description, чтобы детектились и убирались дубликаты в стектрейсе
    ss << "as variable:" << "  " << get_var_name() << " : " << colored_type_out(tinf::get_type(var_));
  } else if (is_return_value_from_function()) {
    ss << "as expression:" << "  " << "return ...";
  } else {
    std::string var_type;
    if (var_) {
      var_type = " : " + colored_type_out(tinf::get_type(var_));
    }
    ss << "as argument:" << "  " << get_var_as_argument_name() << var_type;
  }
  ss << "  " << "at " + get_function_name();
  return ss.str();
}
