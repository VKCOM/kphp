#include "compiler/pipes/analyzer.h"

#include "compiler/data/define-data.h"
#include "compiler/data/var-data.h"
#include "compiler/utils/string-utils.h"

void CommonAnalyzerPass::analyzer_check_array(VertexPtr to_check) {
  bool have_arrow = false;
  bool have_int_key = false;
  std::set<string> used_keys;
  int id = 0;
  for (auto v : *to_check) {
    if (v->type() == op_double_arrow) {
      have_arrow = true;
      VertexPtr key = v.as<op_double_arrow>()->key();
      have_int_key |= key->type() == op_int_const;
      string str;
      if (key->type() == op_string || key->type() == op_int_const) {
        str = key->get_string();
      } else if (key->type() == op_var) {
        VarPtr key_var = key.as<op_var>()->var_id;
        if (key_var->is_constant()) {
          VertexPtr init = key_var->init_val;
          if (init->type() == op_string) {
            str = init->get_string();
          }
        }
      } else if (key->type() == op_define_val) {
        DefinePtr d = key.as<op_define_val>()->define_id;
        VertexPtr dval = d->val;
        if (dval->type() == op_string || dval->type() == op_int_const) {
          str = dval->get_string();
        }
      }
      if (!str.empty()) {
        if (used_keys.find(str) != used_keys.end()) {
          kphp_warning(format("Duplicate key '%s' in array", str.c_str()));
        }
        used_keys.insert(str);
      }
    } else {
      if (have_arrow && have_int_key) {
        return;
      }
      const string &str = int_to_str(id++);
      used_keys.insert(str);
    }
  }
}
void CommonAnalyzerPass::check_set(VertexAdaptor<op_set> to_check) {
  VertexPtr left = to_check->lhs();
  VertexPtr right = to_check->rhs();
  if (left->type() == op_var && right->type() == op_var) {
    VarPtr lvar = left.as<op_var>()->var_id;
    VarPtr rvar = right.as<op_var>()->var_id;
    if (lvar->name == rvar->name) {
      kphp_warning ("Assigning variable to itself\n");
    }
  }
}
void CommonAnalyzerPass::on_enter_edge(VertexPtr vertex, LocalT *, VertexPtr, LocalT *dest_local) {
  dest_local->from_seq = vertex->type() == op_seq;
}
VertexPtr CommonAnalyzerPass::on_enter_vertex(VertexPtr vertex, LocalT * local) {
  VertexPtr to_check;
  if (vertex->type() == op_array) {
    analyzer_check_array(vertex);
    return vertex;
  }
  if (vertex->type() == op_var) {
    VarPtr var = vertex.as<op_var>()->var_id;
    if (var->is_constant()) {
      VertexPtr init = var->init_val;
      run_function_pass(init, this, local);
    }
    return vertex;
  }
  if (local->from_seq) {
    if (OpInfo::P[vertex->type()].rl == rl_op && OpInfo::P[vertex->type()].cnst == cnst_const_func) {
      if (vk::none_of_equal(vertex->type(), op_log_and, op_log_or, op_ternary, op_log_and_let, op_log_or_let, op_unset)) {
        kphp_warning(format("Statement has no effect [op = %s]", OpInfo::P[vertex->type()].str.c_str()));
      }
    }
  }
  if (vertex->type() == op_set) {
    //TODO: $x = (int)$x;
    //check_set(vertex.as<op_set>());
    return vertex;
  }
  return vertex;
}
