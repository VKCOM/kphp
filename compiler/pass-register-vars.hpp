#pragma once
#include "compiler/compiler-core.h"
#include "compiler/analyzer.h"
#include "compiler/gentree.h"

/*** Replace constant expressions with const variables ***/
class CollectConstVarsPass : public FunctionPassBase {
  private:
    AUTO_PROF (collect_const_vars);
    int in_param_list;

    int get_dependency_level(VertexPtr vertex) {
      if (vertex->type() == op_var) {
        VertexAdaptor<op_var> var_adaptor = vertex.as<op_var>();
        return var_adaptor->get_var_id()->dependency_level;
      }

      if (vertex->type() == op_double_arrow) {
        int dep_key = get_dependency_level(vertex.as<op_double_arrow>()->key());
        int dep_value = get_dependency_level(vertex.as<op_double_arrow>()->value());

        return std::max(dep_key, dep_value);
      }

      return 0;
    }

  public:
    struct LocalT : public FunctionPassBase::LocalT {
      bool need_recursion_flag;
    };
    CollectConstVarsPass() :
      in_param_list (0) {
    }
    string get_description() {
      return "Collect constants";
    }

    VertexPtr create_const_variable (VertexPtr root, Location loc) {
      string name;
      bool global_init_flag = true;

      if (root->type() == op_string) {
        name = gen_const_string_name(root.as <op_string>()->str_val);
      } else if (root->type() == op_conv_regexp && root.as <op_conv_regexp>()->expr()->type() == op_string) {
        name = gen_const_regexp_name(root.as <op_conv_regexp>()->expr().as <op_string>()->str_val);
      } else if (is_array_suitable_for_hashing(root)) {
        name = gen_const_array_name(root.as <op_array>());
      } else {
        global_init_flag = false;
        name = gen_unique_name("const_var");
      }

      CREATE_VERTEX (var, op_var);
      var->str_val = name;
      var->extra_type = op_ex_var_const;
      var->location = loc;

      VarPtr var_id = G->get_global_var (name, VarData::var_const_t, root);
      if (root->type() != op_array) {
        var_id->dependency_level = 0;
      } else {
        int max_dep_level = 1;
        for (auto it : root.as<op_array>()->args()) {
          max_dep_level = std::max(max_dep_level, get_dependency_level(it) + 1);
        }

        var_id->dependency_level = max_dep_level;
      }

      var_id->global_init_flag = global_init_flag;

      if (in_param_list > 0) {
        current_function->header_const_var_ids.insert (var_id);
      } else {
        current_function->const_var_ids.insert (var_id);
      }

      var->set_var_id (var_id);
      return var;
    }

    bool check_function (FunctionPtr function) {
      return default_check_function (function) && function->type() != FunctionData::func_extern;
    }

    bool should_convert_to_const(VertexPtr root) {
      return root->type() == op_string || root->type() == op_array ||
             root->type() == op_concat || root->type() == op_string_build ||
             root->type() == op_func_call;
    }

    VertexPtr on_enter_vertex (VertexPtr root, LocalT *local) {
      in_param_list += root->type() == op_func_param_list;

      local->need_recursion_flag = false;

      if (root->type() == op_defined || root->type() == op_require || root->type() == op_require_once) {
        return root;
      }

      if (root->const_type == cnst_const_val) {
        if(root->type() == op_conv_regexp) {
          VertexPtr expr = GenTree::get_actual_value(root.as<op_conv_regexp>()->expr());

          bool conv_to_const =
            expr->type() == op_string ||
            expr->type() == op_concat ||
            expr->type() == op_string_build ||
            should_convert_to_const(root);

          if (conv_to_const) {
            return create_const_variable(root, root->location);
          }
        }
        if (root->type() != op_string && root->type() != op_array) {
           if (should_convert_to_const(root)) {
             return create_const_variable(root, root->location);
           }
        }
      }
      local->need_recursion_flag = true;

      return root;
    }
    bool need_recursion (VertexPtr root __attribute__((unused)), LocalT *local) {
      return local->need_recursion_flag;
    }

    VertexPtr on_exit_vertex (VertexPtr root, LocalT *local __attribute__((unused))) {
      VertexPtr res = root;

      if (root->type() == op_define_val) {
        VertexPtr expr = GenTree::get_actual_value(root);
        if (should_convert_to_const(expr)) {
          res = create_const_variable(expr, root->location);
        }
      }

      if (root->const_type == cnst_const_val) {
        if (should_convert_to_const(root)) {
          res = create_const_variable(root, root->location);
        }
      }

      in_param_list -= root->type() == op_func_param_list;

      return res;
    }
};

/*** Register variables ***/
// 1. Function parametres (with default values)
// 2. Global variables
// 3. Static local variables (with default values)
// 4. Local variables
// 5. Class static fields
class RegisterVariables : public FunctionPassBase {
  private:
    AUTO_PROF (register_variables);
    map <string, VarPtr> registred_vars;
    bool global_function_flag;
    int in_param_list;
  public:

    struct LocalT : public FunctionPassBase::LocalT {
      bool need_recursion_flag;
      LocalT() : need_recursion_flag (true) {}
    };

    RegisterVariables() :
      global_function_flag (false),
      in_param_list (0) {
    }

    string get_description() {
      return "Register variables";
    }

    VarPtr create_global_var (const string &name) {
      VarPtr var = G->get_global_var (name, VarData::var_global_t, VertexPtr());
      pair <map <string, VarPtr>::iterator, bool> it = registred_vars.insert (make_pair (name, var));
      if (it.second == false) {
        VarPtr old_var = it.first->second;
        kphp_error (
          old_var.ptr == var.ptr,
          dl_pstr ("conflict in variable names [%s]", old_var->name.c_str())
        );
      } else {
        if (in_param_list > 0) {
          current_function->header_global_var_ids.push_back (var);
        } else {
          current_function->global_var_ids.push_back (var);
        }
      }
      return var;
    }

    VarPtr create_local_var (const string &name, VarData::Type type, bool create_flag) {
      map <string, VarPtr>::iterator it = registred_vars.find (name);
      if (it != registred_vars.end()) {
        kphp_error (!create_flag, "Redeclaration of local variable");
        return it->second;
      }
      VarPtr var = G->create_local_var (current_function, name, type);
      kphp_error (registred_vars.insert (make_pair (name, var)).second == true, "Redeclaration of local variable");

      return var;
    }

    VarPtr get_global_var (const string &name) {
      map <string, VarPtr>::iterator it = registred_vars.find (name);
      if (it != registred_vars.end()) {
        return it->second;
      }
      return create_global_var (name);
    }
    VarPtr get_local_var (const string &name, VarData::Type type = VarData::var_local_t) {
      return create_local_var(name, type, false);
    }

    void register_global_var (VertexAdaptor <op_var> var_vertex) {
      string name = var_vertex->str_val;
      var_vertex->set_var_id (create_global_var (name));

      FunctionPtr function_where_global_keyword_occured = var_vertex->location.get_function();
      if (function_where_global_keyword_occured.not_null() && function_where_global_keyword_occured->type() == FunctionData::func_global) {
        var_vertex->get_var_id()->marked_as_global = true;
      }
    }

    bool is_const (VertexPtr v) {
      return v->const_type == cnst_const_val ||
        (v->type() == op_var && v->get_var_id()->type() == VarData::var_const_t) ||
        v->type() == op_define_val;
    }
    bool is_global_var (VertexPtr v) {
      return v->type() == op_var && v->get_var_id()->type() == VarData::var_global_t;
    }

    void register_static_var (VertexAdaptor <op_var> var_vertex, VertexPtr default_value, OperationExtra extra_type) {
      kphp_error_return (!global_function_flag || extra_type == op_ex_static_private || extra_type == op_ex_static_public
                         || extra_type == op_ex_static_protected,
                         "Keyword 'static' used in global function");

      VarPtr var;
      string name;
      if (global_function_flag) {
        kphp_assert (extra_type == op_ex_static_private || extra_type == op_ex_static_public || extra_type == op_ex_static_protected);
        name = replace_characters(current_function->namespace_name, '\\', '$') + "$" +
          current_function->class_name + "$$" + var_vertex->str_val;
        var = get_global_var(name);
        var->class_id = current_function->class_id;
      } else {
        kphp_assert (extra_type == op_ex_none);
        name = var_vertex->str_val;
        var = create_local_var(name, VarData::var_static_t, true);
        var->static_id = current_function;
      }
      if (default_value.not_null()) {
        if (!kphp_error (is_const (default_value), dl_pstr ("Default value of [%s] is not constant", name.c_str()))) {
          var->init_val = default_value;
        }
      }
      var_vertex->set_var_id (var);
      var->access_type =
          extra_type == op_ex_static_public ? access_static_public :
          extra_type == op_ex_static_private ? access_static_private :
          extra_type == op_ex_static_protected ? access_static_protected :
          access_nonmember;
    }

    void register_param_var (VertexAdaptor <op_var> var_vertex, VertexPtr default_value) {
      string name = var_vertex->str_val;
      VarPtr var = create_local_var (name, VarData::var_param_t, true);
      var->is_reference = var_vertex->ref_flag;
      kphp_assert (var.not_null());
      if (default_value.not_null()) {
        if (!kphp_error (
              is_const (default_value) || is_global_var (default_value),
              dl_pstr ("Default value of [%s] is not constant", name.c_str()))) {
          var->init_val = default_value;
        }
      }
      var_vertex->set_var_id (var);
    }

    void register_var (VertexAdaptor <op_var> var_vertex) {
      VarPtr var;
      string name = var_vertex->str_val;
      size_t pos$$ = name.find("$$");
      if (pos$$ != string::npos ||
          (vk::none_of_equal(var_vertex->extra_type, op_ex_var_superlocal, op_ex_var_superlocal_inplace) &&
           global_function_flag) ||
          var_vertex->extra_type == op_ex_var_superglobal) {
        if (pos$$ != string::npos) {
          string class_name = name.substr(0, pos$$);
          string var_name = name.substr(pos$$ + 2);
          ClassPtr klass = G->get_class(replace_characters(class_name, '$', '\\'));
          kphp_assert(klass.not_null());
          while (klass.not_null() && klass->static_fields.find(var_name) == klass->static_fields.end()) {
            klass = klass->parent_class;
          }
          if (kphp_error(klass.not_null(), dl_pstr("static field not found: %s", name.c_str()))) {
            return ;
          }
          name = replace_characters(klass->name, '\\', '$') + "$$" + var_name;
        }
        var = get_global_var (name);
      } else {
        VarData::Type var_type = var_vertex->extra_type == op_ex_var_superlocal_inplace
                                 ? VarData::var_local_inplace_t
                                 : VarData::var_local_t;
        var = get_local_var(name, var_type);
      }
      if (var_vertex->needs_const_iterator_flag) {
        var->needs_const_iterator_flag = true;
      }
      var_vertex->set_var_id (var);
      var->marked_as_global |= var_vertex->extra_type == op_ex_var_superglobal;
    }

    void visit_global_vertex (VertexAdaptor <op_global> global) {
      for (auto var : global->args()) {
        kphp_error_act (
          var->type() == op_var,
          "unexpected expression in 'global'",
          continue
        );
        register_global_var (var);
      }
    }

    void visit_static_vertex (VertexAdaptor <op_static> stat) {
      for (auto node : stat->args()) {
        VertexAdaptor <op_var> var;
        VertexPtr default_value;

        if (node->type() == op_var) {
          var = node;
        } else if (node->type() == op_set) {
          VertexAdaptor <op_set> set_expr = node;
          var = set_expr->lhs();
          kphp_error_act (
            var->type() == op_var,
            "unexpected expression in 'static'",
            continue
          );
          default_value = set_expr->rhs();
        } else {
          kphp_error_act (0, "unexpected expression in 'static'", continue);
        }

        register_static_var (var, default_value, stat->extra_type);
      }
    }
    template <class VisitT>
    void visit_func_param_list (VertexAdaptor <op_func_param_list> list, VisitT &visit) {
      for (auto i : list->params()) {
        kphp_assert (i.not_null());
        kphp_assert (i->type() == op_func_param);
        VertexAdaptor <op_func_param> param = i;
        VertexAdaptor <op_var> var = param->var();
        VertexPtr default_value;
        if (param->has_default()) {
          default_value = param->default_value();
          visit (param->default_value());
        }
        register_param_var (var, default_value);
      }
    }
    void visit_var (VertexAdaptor <op_var> var) {
      if (var->get_var_id().not_null()) {
        // автогенерённые через CREATE_VERTEX op_var типы, когда один VarData на несколько разных vertex'ов
        kphp_assert (var->get_var_id()->type() == VarData::var_const_t ||
                     var->get_var_id()->type() == VarData::var_local_inplace_t);
        return;
      }
      register_var (var);
    }

    bool check_function (FunctionPtr function) {
      return default_check_function (function) && function->type() != FunctionData::func_extern;
    }

    bool on_start (FunctionPtr function) {
      if (!FunctionPassBase::on_start (function)) {
        return false;
      }
      global_function_flag = function->type() == FunctionData::func_global ||
        function->type() == FunctionData::func_switch;
      return true;
    }

    VertexPtr on_enter_vertex (VertexPtr root, LocalT *local) {
      kphp_assert (root.not_null());
      if (root->type() == op_global) {
        visit_global_vertex (root);
        local->need_recursion_flag = false;
        CREATE_VERTEX (empty, op_empty);
        return empty;
      } else if (root->type() == op_static) {
        kphp_error(!stage::get_function()->root->inline_flag, "Inline functions don't support static variables");
        visit_static_vertex (root);
        local->need_recursion_flag = false;
        CREATE_VERTEX (empty, op_empty);
        return empty;
      } else if (root->type() == op_var) {
        visit_var (root);
        local->need_recursion_flag = false;
      }
      return root;
    }

    template <class VisitT>
    bool user_recursion (VertexPtr v, LocalT *local __attribute__((unused)), VisitT &visit) {
      if (v->type() == op_func_param_list) {
        in_param_list++;
        visit_func_param_list (v, visit);
        in_param_list--;
        return true;
      }
      return false;
    }


    bool need_recursion (VertexPtr root __attribute__((unused)), LocalT *local __attribute__((unused))) {
      return local->need_recursion_flag;
    }
};

class CheckAccessModifiers : public FunctionPassBase {
  private:
    AUTO_PROF (check_access_modifiers);
    string namespace_name;
    string class_name;
  public:
    CheckAccessModifiers() :
      namespace_name(""),
      class_name("") {
    }

    string get_description() {
      return "Check access modifiers";
    }

    bool check_function (FunctionPtr function) {
      return default_check_function (function) && function->type() != FunctionData::func_extern;
    }

    bool on_start (FunctionPtr function) {
      if (!FunctionPassBase::on_start (function)) {
        return false;
      }
      namespace_name = function->namespace_name;
      class_name = function->class_name;
      return true;
    }

    VertexPtr on_enter_vertex (VertexPtr root, LocalT *local __attribute__((unused))) {
      kphp_assert (root.not_null());
      if (root->type() == op_var) {
        VarPtr var_id = root.as<op_var>()->get_var_id();
        string real_name = root.as<op_var>()->str_val;
        string name = var_id->name;
        size_t pos = name.find("$$");
        if (pos != string::npos) {
          kphp_error(var_id->access_type != access_nonmember,
                     dl_pstr("Field wasn't declared: %s", real_name.c_str()));
          kphp_error(var_id->access_type != access_static_private ||
                     replace_characters(namespace_name, '\\', '$') + "$" + class_name == name.substr(0, pos),
                            dl_pstr("Can't access private field %s", real_name.c_str()));
          if (var_id->access_type == access_static_protected) {
            kphp_error_act(!namespace_name.empty() && !class_name.empty(), dl_pstr("Can't access protected field %s", real_name.c_str()), return root);
            ClassPtr var_class = var_id->class_id;
            ClassPtr klass = G->get_class(namespace_name + "\\" + class_name);
            kphp_assert(klass.not_null());
            while (klass.not_null() && var_class.ptr != klass.ptr) {
              klass = klass->parent_class;
            }
            kphp_error(klass.not_null(), dl_pstr("Can't access protected field %s", real_name.c_str()));
          }
        }
      } else if (root->type() == op_func_call) {
        FunctionPtr func_id = root.as<op_func_call>()->get_func_id();
        string name = func_id->name;
        size_t pos = name.find("$$");
        if (pos != string::npos) {
          kphp_assert(func_id->access_type != access_nonmember);
          kphp_error(func_id->access_type != access_static_private ||
                     replace_characters(namespace_name, '\\', '$') + "$" + class_name == name.substr(0, pos),
                     dl_pstr("Can't access private function %s", name.c_str()));
          // TODO: check protected
        }
      }
      return root;
    }
};

/**
 * 1. Паттерн list(...) = [...] или list(...) = f() : правую часть — во временную переменную $tmp_var
 * При этом $tmp_var это op_ex_var_superlocal_inplace: её нужно объявить по месту, а не выносить в начало функции в c++
 * Соответственно, по смыслу сущность $tmp_var это array или tuple
 * 2. list(...) = $var оборачиваем в op_seq_rval { list; $var }, для поддержки while(list()=f()) / if(... && list()=f())
 */
class ConvertListAssignmentsPass : public FunctionPassBase {
public:
  struct LocalT : public FunctionPassBase::LocalT {
    bool need_recursion_flag = true;
  };

  string get_description () {
    return "Process assignments to list";
  }

  bool check_function (FunctionPtr function) {
    return default_check_function(function) && function->type() != FunctionData::func_extern;
  }

  VertexPtr on_exit_vertex (VertexPtr root, LocalT *local) {
    if (root->type() == op_list) {
      local->need_recursion_flag = false;
      return process_list_assignment(root);
    }

    return root;
  }

  bool need_recursion (VertexPtr root __attribute__ ((unused)), LocalT *local) {
    return local->need_recursion_flag;
  }

  VertexPtr process_list_assignment (VertexAdaptor <op_list> list) {
    VertexPtr op_set_to_tmp_var;
    if (list->array()->type() != op_var) {        // list(...) = $var не трогаем, только list(...) = f()
      CREATE_VERTEX(tmp_var, op_var);
      tmp_var->set_string(gen_unique_name("tmp_var"));
      tmp_var->extra_type = op_ex_var_superlocal_inplace;        // отвечает требованиям: инициализируется 1 раз и внутри set
      CREATE_VERTEX(set_var, op_set, tmp_var, list->array());
      CLONE_VERTEX(tmp_var_rval, op_var, tmp_var);
      list->array() = tmp_var_rval;
      op_set_to_tmp_var = set_var;
    }

    CLONE_VERTEX(tmp_var_rval, op_var, list->array().as <op_var>());

    VertexPtr result_seq;
    if (op_set_to_tmp_var.is_null()) {
      CREATE_VERTEX(seq, op_seq_rval, list, tmp_var_rval);
      result_seq = seq;
    } else {
      CREATE_VERTEX(seq, op_seq_rval, op_set_to_tmp_var, list, tmp_var_rval);
      result_seq = seq;
    }
    set_location(result_seq, list->location);
    return result_seq;
  }
};
