#pragma once

#include "compiler/data/var-data.h"
#include "compiler/function-pass.h"

/**
 * 1. Function parametres (with default values)
 * 2. Global variables
 * 3. Static local variables (with default values)
 * 4. Local variables
 * 5. Class static fields
 */
class RegisterVariablesPass : public FunctionPassBase {
private:
  map<string, VarPtr> registred_vars;
  bool global_function_flag;
  int in_param_list;

  VarPtr create_global_var(const string &name);
  VarPtr create_local_var(const string &name, VarData::Type type, bool create_flag);
  VarPtr get_global_var(const string &name);
  VarPtr get_local_var(const string &name, VarData::Type type = VarData::var_local_t);
  void register_global_var(VertexAdaptor<op_var> var_vertex);
  bool is_const(VertexPtr v);
  bool is_global_var(VertexPtr v);
  void register_function_static_var(VertexAdaptor<op_var> var_vertex, VertexPtr default_value);
  void register_class_static_var(ClassPtr class_id, ClassMemberStaticField &f);
  void register_param_var(VertexAdaptor<op_var> var_vertex, VertexPtr default_value);
  void register_var(VertexAdaptor<op_var> var_vertex);
  void visit_global_vertex(VertexAdaptor<op_global> global);
  void visit_static_vertex(VertexAdaptor<op_static> stat);
  void visit_var(VertexAdaptor<op_var> var);
  void visit_class(ClassPtr klass);

  template<class VisitT>
  void visit_func_param_list(VertexAdaptor<op_func_param_list> list, VisitT &visit) {
    for (auto i : list->params()) {
      kphp_assert (i);
      kphp_assert (i->type() == op_func_param);
      VertexAdaptor<op_func_param> param = i;
      VertexAdaptor<op_var> var = param->var();
      VertexPtr default_value;
      if (param->has_default_value() && param->default_value()) {
        default_value = param->default_value();
        visit(param->default_value());
      }
      register_param_var(var, default_value);
    }
  }


public:

  struct LocalT : public FunctionPassBase::LocalT {
    bool need_recursion_flag;

    LocalT() :
      need_recursion_flag(true) {}
  };

  RegisterVariablesPass() :
    global_function_flag(false),
    in_param_list(0) {
  }

  string get_description() {
    return "Register variables";
  }

  bool check_function(FunctionPtr function) {
    return default_check_function(function) && function->type() != FunctionData::func_extern;
  }

  bool on_start(FunctionPtr function) {
    if (!FunctionPassBase::on_start(function)) {
      return false;
    }
    global_function_flag = function->type() == FunctionData::func_global ||
                           function->type() == FunctionData::func_switch;
    if (current_function->type() == FunctionData::func_class_holder) {
      visit_class(current_function->class_id);
    }
    return true;
  }

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *local);

  bool user_recursion(VertexPtr v, LocalT *local __attribute__((unused)), VisitVertex<RegisterVariablesPass> &visit);


  bool need_recursion(VertexPtr root __attribute__((unused)), LocalT *local __attribute__((unused))) {
    return local->need_recursion_flag;
  }
};
