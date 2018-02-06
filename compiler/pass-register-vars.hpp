#pragma once
#include "compiler-core.h"
#include "analyzer.h"
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

    /*** Serialize big consts if possible ***/
    static bool check_const (VertexPtr v, int *nodes_cnt) {
      (*nodes_cnt)++;

      if (!(v->type() == op_string || v->type() == op_int_const || v->type() == op_array ||
            v->type() == op_float_const || v->type() == op_true || v->type() == op_false || v->type() == op_null)) {
        return false;
      }
      if (v->type() == op_array) {
        int has_key = false, no_key = false;

        for (VertexRange i = all (*v); !i.empty(); i.next()) {
          VertexPtr cur = *i;
          if (cur->type() == op_double_arrow) {
            VertexAdaptor <op_double_arrow> arrow = cur;
            VertexPtr key = arrow->key();
            VertexPtr value = arrow->value();

            has_key = true;

            if (key->type() != op_int_const && key->type() != op_string) {
              return false;
            }

            if (!check_const (key, nodes_cnt) || !check_const (value, nodes_cnt)) {
              return false;
            }
          } else {
            no_key = true;
            if (!check_const (cur, nodes_cnt)) {
              return false;
            }
          }
        }
        if (has_key && no_key) {
          return false;
        }
      }
      return true;
    }


    static void serialize_int (string int_val, string *s) {
      int val;
      sscanf (int_val.c_str(), "%i", &val);
      int_val = int_to_str (val);
      (*s) += "i:";
      (*s) += int_val;
      (*s) += ";";
    }

    static void serialize_float (string float_val, string *s) {
      (*s) += "d:";
      (*s) += float_val;
      (*s) += ";";
    }

    static void serialize_bool (bool f, string *s) {
      (*s) += "b:";
      (*s) += f ? "1" : "0";
      (*s) += ";";
    }

    static void serialize_string (string str_val, string *s) {
      (*s) += "s:";
      (*s) += int_to_str ((int)str_val.size());
      (*s) += ":";
      (*s) += "\"";
      (*s) += str_val;
      (*s) += "\"";
      (*s) += ";";
    }

    static void serialize_array (VertexAdaptor <op_array> arr, string *s) {
      VertexRange args = arr->args();
      int ni = (int)args.size();
      (*s) += "a:";
      (*s) += int_to_str (ni);
      (*s) += ":{";

      for (int i = 0; i < ni; i++) {
        VertexPtr cur = args[i];
        if (cur->type() == op_double_arrow) {
          VertexAdaptor <op_double_arrow> arrow = cur;
          serialize_const (arrow->key(), s);
          serialize_const (arrow->value(), s);
        } else {
          serialize_int (int_to_str (i), s);
          serialize_const (cur, s);
        }
      }

      (*s) += "}";
    }

    static void serialize_const (VertexPtr v, string *s) {
      VertexPtr actual_v = GenTree::get_actual_value(v);
      switch (actual_v->type()) {
        case op_int_const:
          serialize_int (actual_v.as <op_int_const>()->str_val, s);
          break;
        case op_true:
          serialize_bool (true, s);
          break;
        case op_false:
          serialize_bool (false, s);
          break;
        case op_float_const:
          serialize_float (actual_v.as <op_float_const>()->str_val, s);
          break;
        case op_string:
          serialize_string (actual_v.as <op_string>()->str_val, s);
          break;
        case op_array:
          serialize_array (actual_v, s);
          break;
        case op_conv_int:
          serialize_const(v.as<meta_op_unary_>()->expr(), s);
          break;
        case op_add: {
          VertexAdaptor<meta_op_binary_op> binary = v.as<meta_op_binary_op>();
          serialize_const(binary->lhs(), s);
          (*s) += OpInfo::str(actual_v->type());
          serialize_const(binary->rhs(), s);
          break;
        }
        case op_null:
          *s = "op_null";
          break;

        default: {
          string msg = "unexpected type: " + OpInfo::str(actual_v->type());
          kphp_assert_msg(false, msg.c_str());
          break;
        }
      }
    }

    VertexPtr create_const_variable (VertexPtr root, Location loc) {
      string name;
      bool global_init_flag = true;

      if (root->type() == op_string) {
        name = gen_const_string_name(root.as<op_string>()->str_val);
      } else if (root->type() == op_conv_regexp && root.as<op_conv_regexp>()->expr()->type() == op_string) {
        name = gen_const_regexp_name(root.as<op_conv_regexp>()->expr().as<op_string>()->str_val);
      } else if (is_array_suitable_for_hashing(root)) {
          name = gen_const_array_name(root.as<op_array>());
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
        FOREACH(root.as<op_array>()->args(), it) {
          max_dep_level = std::max(max_dep_level, get_dependency_level(*it) + 1);
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

      if (root->const_type == cnst_const_val && root->type() == op_conv_regexp) {
        VertexPtr expr = GenTree::get_actual_value(root.as <op_conv_regexp>()->expr());

        bool conv_to_const =
          expr->type() == op_string ||
          expr->type() == op_concat ||
          expr->type() == op_string_build ||
          should_convert_to_const(root);

        if (conv_to_const) {
          return create_const_variable (root, root->location);
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
    VarPtr get_var (const string &name) {
      return create_local_var (name, VarData::var_local_t, false);
    }

    void register_global_var (VertexAdaptor <op_var> var_vertex) {
      string name = var_vertex->str_val;
      var_vertex->set_var_id (create_global_var (name));
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
      switch (extra_type) {
        case op_ex_static_private:
          var->access_type = access_private;
          break;
        case op_ex_static_public:
          var->access_type = access_public;
          break;
        case op_ex_static_protected:
          var->access_type = access_protected;
          break;
        case op_ex_none:
          var->access_type = access_nonmember;
          break;
        default:
          kphp_assert(false);
      }
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
      if (pos$$ != string::npos || (var_vertex->extra_type != op_ex_var_superlocal && global_function_flag) ||
          var_vertex->extra_type == op_ex_var_superglobal) {
        if (pos$$ != string::npos) {
          string class_name = name.substr(0, pos$$);
          string var_name = name.substr(pos$$ + 2);
          ClassPtr klass = G->get_class(replace_characters(class_name, '$', '\\'));
          kphp_assert(klass.not_null());
          while (klass.not_null() && klass->static_fields.find(var_name) == klass->static_fields.end()) {
            klass = klass->parent_class;
          }
          kphp_error(klass.not_null(), dl_pstr("static field not found: %s", name.c_str()));
          name = replace_characters(klass->name, '\\', '$') + "$$" + var_name;
        }
        var = get_global_var (name);
      } else {
        var = get_var (name);
      }
      if (var_vertex->needs_const_iterator_flag) {
        var->needs_const_iterator_flag = true;
      }
      var_vertex->set_var_id (var);
    }

    void visit_global_vertex (VertexAdaptor <op_global> global) {
      for (VertexRange i = global->args(); !i.empty(); i.next()) {
        VertexPtr var = *i;
        kphp_error_act (
          var->type() == op_var,
          "unexpected expression in 'global'",
          continue
        );
        register_global_var (var);
      }
    }

    void visit_static_vertex (VertexAdaptor <op_static> stat) {
      for (VertexRange i = stat->args(); !i.empty(); i.next()) {
        VertexAdaptor <op_var> var;
        VertexPtr default_value;

        VertexPtr node = *i;
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
      for (VertexRange i = list->params(); !i.empty(); i.next()) {
        kphp_assert ((*i).not_null());
        kphp_assert ((*i)->type() == op_func_param);
        VertexAdaptor <op_func_param> param = *i;
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
        kphp_assert (var->get_var_id()->type() == VarData::var_const_t);
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
          kphp_error(var_id->access_type == access_private || var_id->access_type == access_public ||
                       var_id->access_type == access_protected,
                     dl_pstr("Field wasn't declared: %s", real_name.c_str()));
          kphp_error(var_id->access_type != access_private ||
                     replace_characters(namespace_name, '\\', '$') + "$" + class_name == name.substr(0, pos),
                            dl_pstr("Can't access private field %s", real_name.c_str()));
          if (var_id->access_type == access_protected) {
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
        string real_name = root.as<op_func_call>()->str_val;
        size_t pos = name.find("$$");
        if (pos != string::npos) {
          kphp_assert(func_id->access_type == access_private || func_id->access_type == access_public ||
                        func_id->access_type == access_protected);
          kphp_error(func_id->access_type != access_private ||
                     replace_characters(namespace_name, '\\', '$') + "$" + class_name == name.substr(0, pos),
                     dl_pstr("Can't access private function %s", name.c_str()));
          // TODO: check protected
        }
      }
      return root;
    }
};
