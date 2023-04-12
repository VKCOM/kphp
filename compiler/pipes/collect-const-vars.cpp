// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/collect-const-vars.h"

#include "compiler/data/src-file.h"
#include "compiler/vertex-util.h"
#include "compiler/data/var-data.h"
#include "compiler/compiler-core.h"
#include "compiler/name-gen.h"

namespace {

template<class Derived, class ResultType>
struct VertexVisitor {
  static ResultType visit(VertexPtr v) {
    switch (v->type()) {
      case op_array:
        return Derived::on_array(v.as<op_array>());
      case op_string:
        return Derived::on_string(v.as<op_string>());
      case op_concat:
        return Derived::on_concat(v.as<op_concat>());
      case op_conv_regexp:
        return Derived::on_conv_regexp(v.as<op_conv_regexp>());
      case op_func_call:
        return Derived::on_func_call(v.as<op_func_call>());
      case op_string_build:
        return Derived::on_string_build(v.as<op_string_build>());
      default:
        return Derived::fallback(v);
    }
  }

  static ResultType on_array(VertexAdaptor<op_array> v) {
    return Derived::fallback(v);
  }

  static ResultType on_string(VertexAdaptor<op_string> v) {
    return Derived::fallback(v);
  }

  static ResultType on_concat(VertexAdaptor<op_concat> v) {
    return Derived::fallback(v);
  }

  static ResultType on_conv_regexp(VertexAdaptor<op_conv_regexp> v) {
    return Derived::fallback(v);
  }

  static ResultType on_func_call(VertexAdaptor<op_func_call> v) {
    return Derived::fallback(v);
  }

  static ResultType on_string_build(VertexAdaptor<op_string_build> v) {
    return Derived::fallback(v);
  }

  static ResultType fallback(VertexPtr v [[maybe_unused]]) {
    kphp_assert_msg(false, "Internal error: invalid visitor in CollectConstVars pass!");
  }
};

struct IsComposite : public VertexVisitor<IsComposite, bool> {
  static bool on_array(VertexAdaptor<op_array> v [[maybe_unused]]) {
    return true;
  }

  static bool fallback(VertexPtr v [[maybe_unused]]) {
    return false;
  }
};

struct ShouldStoreOnTopDown : public VertexVisitor<ShouldStoreOnTopDown, bool> {
  static bool on_conv_regexp(VertexAdaptor<op_conv_regexp> v) {
    const VertexPtr expr = VertexUtil::get_actual_value(v->expr());
    return vk::any_of_equal(expr->type(), op_string, op_concat, op_string_build);
  }

  static bool fallback(VertexPtr v) {
    return vk::any_of_equal(v->type(), op_concat, op_string_build, op_func_call);
  }
};

struct ShouldStoreOnBottomUp : public VertexVisitor<ShouldStoreOnBottomUp, bool> {
  static bool fallback(VertexPtr v) {
    return vk::any_of_equal(v->type(), op_string, op_array);
  }
};

struct NameGenerator : public VertexVisitor<NameGenerator, std::string> {
  static constexpr auto prefix = "const_var";

  static std::string fallback(VertexPtr v[[maybe_unused]]) {
    return gen_unique_name(prefix);
  }

  static std::string on_string(VertexAdaptor<op_string> v) {
    return gen_const_string_name(v->str_val);
  }

  static std::string on_array(VertexAdaptor<op_array> v) {
    if (is_array_suitable_for_hashing(v)) {
      return gen_const_array_name(v);
    }
    return fallback(v);
  }

  static std::string on_conv_regexp(VertexAdaptor<op_conv_regexp> v) {
    if (v->expr()->type() == op_string) {
      return gen_const_regexp_name(v->expr().as<op_string>()->str_val);
    }
    return fallback(v);
  }
};

struct ProcessBeforeReplace : public VertexVisitor<ProcessBeforeReplace, VertexPtr> {
    static VertexPtr fallback(VertexPtr v) {
      return v;
    }

    static VertexPtr on_func_call(VertexAdaptor<op_func_call> v) {
      return remove_op_define_val(v);
    }

    static VertexPtr on_conv_regexp(VertexAdaptor<op_conv_regexp> regexp) {
      return remove_op_define_val(regexp);
    }

    static VertexPtr on_concat(VertexAdaptor<op_concat> concat) {
      return remove_op_define_val(concat);
    }

    static VertexPtr on_string_build(VertexAdaptor<op_string_build> str_build) {
      return remove_op_define_val(str_build);
    }

  private:
    static VertexPtr remove_op_define_val(VertexPtr v) {
      for (VertexPtr& sub_vertex: *v) {
        sub_vertex = remove_op_define_val(sub_vertex);
      }
      if (auto as_op_define_val = v.try_as<op_define_val>()) {
        return as_op_define_val->value();
      }
      return v;
    }
};

int get_expr_dep_level(VertexPtr vertex) {
  if (auto var = vertex.try_as<op_var>()) {
    return var->var_id->dependency_level;
  }
  int max_dependency_level = 0;
  for (const auto &child: *vertex) {
    max_dependency_level = std::max(max_dependency_level, get_expr_dep_level(child));
  }
  return max_dependency_level;
}

void set_var_dep_level(VarPtr var_id) {
  if (!IsComposite::visit(var_id->init_val)) {
    var_id->dependency_level = 0;
  } else {
    var_id->dependency_level = 1 + get_expr_dep_level(var_id->init_val);
  }
}


} // namespace


VertexPtr CollectConstVarsPass::on_exit_vertex(VertexPtr root) {
  if (root->const_type == cnst_const_val) {
    composite_const_depth_ -= static_cast<int>(IsComposite::visit(root));
    if (ShouldStoreOnBottomUp::visit(root)) {
      root = ProcessBeforeReplace::visit(root);
      root = create_const_variable(root, root->location);
    }
  }

  in_param_list_ -= static_cast<int>(root->type() == op_func_param_list);
  if (auto as_define_val = root.try_as<op_define_val>()) {
    return as_define_val->value();
  }
  return root;
}

VertexPtr CollectConstVarsPass::on_enter_vertex(VertexPtr root) {
  in_param_list_ += static_cast<int>(root->type() == op_func_param_list);

  if (vk::any_of_equal(root->type(), op_defined)) {
    return root;
  }

  if (root->const_type == cnst_const_val) {
    if (ShouldStoreOnTopDown::visit(root)) {
      root = ProcessBeforeReplace::visit(root);
      return create_const_variable(root, root->location);
    }
    composite_const_depth_ += static_cast<int>(IsComposite::visit(root));
  }
  return root;
}


VertexPtr CollectConstVarsPass::create_const_variable(VertexPtr root, Location loc) {
  std::string name = NameGenerator::visit(root);

  auto var = VertexAdaptor<op_var>::create();
  var->str_val = name;
  var->extra_type = op_ex_var_const;
  var->location = loc;

  VarPtr var_id = G->get_global_var(name, VarData::var_const_t, root);
  set_var_dep_level(var_id);

  if (composite_const_depth_ > 0) {
    current_function->implicit_const_var_ids.insert(var_id);
  } else {
    if (in_param_list_ > 0) {
      current_function->explicit_header_const_var_ids.insert(var_id);
    } else {
      current_function->explicit_const_var_ids.insert(var_id);
    }
  }

  var->var_id = var_id;
  return var;
}

bool CollectConstVarsPass::user_recursion(VertexPtr v) {
  if (v->type() == op_function) {
    if (current_function->type == FunctionData::func_class_holder) {
      ClassPtr c = current_function->class_id;
      c->members.for_each([&](ClassMemberInstanceField &field) {
        if (field.var->init_val) {
          run_function_pass(field.var->init_val, this);
        }
      });
      c->members.for_each([&](ClassMemberStaticField &field) {
        if (field.var->init_val) {
          run_function_pass(field.var->init_val, this);
        }
      });
    }
  }
  return v->type() == op_defined;
}
