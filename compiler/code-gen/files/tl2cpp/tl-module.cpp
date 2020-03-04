#include "compiler/code-gen/files/tl2cpp/tl-module.h"

#include "compiler/code-gen/files/tl2cpp/tl-constructor.h"
#include "compiler/code-gen/files/tl2cpp/tl-function.h"
#include "compiler/code-gen/files/tl2cpp/tl-type.h"

namespace tl2cpp {
std::unordered_map<std::string, Module> modules;
std::set<std::string> modules_with_functions;

void Module::compile_tl_h_file(CodeGenerator &W) const {
  W << OpenFile(name + ".h", "tl");
  W << "#pragma once" << NL;
  W << ExternInclude("runtime/tl/tl_builtins.h");
  W << ExternInclude("tl/tl_const_vars.h");
  W << h_includes;
  W << NL;
  for (const auto &t : target_types) {
    for (const auto &constructor : t->constructors) {
      W << TlConstructorDecl(constructor.get());
    }
  }
  for (const auto &t : target_types) {
    W << TlTypeDeclaration(t);
    if (is_type_dependent(t, tl)) {
      W << TlTypeDefinition(t);
    }
  }
  for (const auto &t : target_types) {
    if (is_type_dependent(t, tl)) {
      for (const auto &constructor : t->constructors) {
        W << TlConstructorDef(constructor.get());
      }
    }
  }
  for (const auto &f : target_functions) {
    W << TlFunctionDecl(f);
  }
  W << CloseFile();
}

void Module::compile_tl_cpp_file(CodeGenerator &W) const {
  W << OpenFile(name + ".cpp", "tl", false);
  W << ExternInclude("php_functions.h");
  W << Include("tl/" + name + ".h") << NL;
  W << cpp_includes;

  for (const auto &t : target_types) {
    if (!is_type_dependent(t, tl)) {
      W << TlTypeDefinition(t);
      for (const auto &constructor : t->constructors) {
        W << TlConstructorDef(constructor.get());
      }
    }
  }
  for (const auto &f: target_functions) {
    W << TlFunctionDef(f);
  }
  W << CloseFile();
}

void Module::add_obj(const std::unique_ptr<vk::tl::combinator> &f) {
  modules_with_functions.insert(name);
  target_functions.push_back(f.get());
  update_dependencies(f);

  if (TlFunctionDecl::does_tl_function_need_typed_fetch_store(f.get())) {
    ClassPtr klass = get_php_class_of_tl_function(f.get());
    kphp_assert(klass);
    h_includes.add_class_forward_declaration(klass);
    cpp_includes.add_class_include(klass);
    klass = get_php_class_of_tl_function_result(f.get());
    kphp_assert(klass);
    cpp_includes.add_class_include(klass);
  }
}

void Module::add_obj(const std::unique_ptr<vk::tl::type> &t) {
  target_types.push_back(t.get());
  update_dependencies(t);

  for (const auto &c : t->constructors) {
    if (TlConstructorDecl::does_tl_constructor_need_typed_fetch_store(c.get())) {
      auto php_classes = get_all_php_classes_of_tl_constructor(c.get());
      std::for_each(php_classes.begin(), php_classes.end(), [&](ClassPtr klass) { h_includes.add_class_include(klass); });
    }
  }
}

void Module::update_dependencies(const std::unique_ptr<vk::tl::combinator> &combinator) {
  for (const auto &arg : combinator->args) {
    collect_deps_from_type_tree(arg->type_expr.get());
  }
  if (combinator->is_function()) {
    collect_deps_from_type_tree(combinator->result.get());
  }
}

void Module::update_dependencies(const std::unique_ptr<vk::tl::type> &t) {
  std::string type_module = get_module_name(t->name);
  for (const auto &c : t->constructors) {
    update_dependencies(c);
    if (type_module != name) {
      ensure_existence(type_module).h_includes.add_raw_filename_include("tl/" + name + ".h");
    }
  }
}

void Module::collect_deps_from_type_tree(vk::tl::expr_base *expr) {
  if (auto as_type_expr = expr->as<vk::tl::type_expr>()) {
    std::string expr_module = get_module_name(tl->types[as_type_expr->type_id]->name);
    if (name != expr_module) {
      h_includes.add_raw_filename_include("tl/" + expr_module + ".h");
    }
    for (const auto &child : as_type_expr->children) {
      collect_deps_from_type_tree(child.get());
    }
  } else if (auto as_type_array = expr->as<vk::tl::type_array>()) {
    for (const auto &arg : as_type_array->args) {
      collect_deps_from_type_tree(arg->type_expr.get());
    }
  }
}

void check_type_expr(vk::tl::expr_base *expr_base) {
  if (auto array = expr_base->as<vk::tl::type_array>()) {
    for (const auto &arg : array->args) {
      check_type_expr(arg->type_expr.get());
    }
  } else if (auto type_expr = expr_base->as<vk::tl::type_expr>()) {
    vk::tl::type *type = type_of(type_expr);
    if (type->is_integer_variable() || type->name == T_TYPE) {
      return;
    }
    if (type->name == "Maybe") {
      assert(type_expr->children.size() == 1);
      const auto &child = type_expr->children.back();
      if (auto child_as_type_expr = child->as<vk::tl::type_expr>()) {
        kphp_error(type_of(child_as_type_expr)->id != TL_TRUE, "'Maybe True' usage is prohibited in TL scheme");
      }
    }
    kphp_error(!type->is_polymorphic() || !type_expr->is_bare(),
               fmt_format("Polymorphic TL type {} can't be used as bare in tl scheme.", type->name));
    for (const auto &child : type_expr->children) {
      check_type_expr(child.get());
    }
  }
}

void check_combinator(const std::unique_ptr<vk::tl::combinator> &c) {
  for (const auto &arg : c->args) {
    check_type_expr(arg->type_expr.get());
  }
}

void check_constructor(const std::unique_ptr<vk::tl::combinator> &c) {
  check_combinator(c);
  // Проверяем, что порядок неявных аргументов конструктора совпадает с их порядком в типе
  std::vector<int> var_nums;
  for (const auto &arg : c->args) {
    if (arg->is_optional() && arg->var_num != -1) {
      var_nums.push_back(arg->var_num);
    }
  }
  std::vector<int> params_order;
  auto as_type_expr = c->result->as<vk::tl::type_expr>();
  int idx = 0;
  for (const auto &child : as_type_expr->children) {
    if (auto as_nat_var = child->as<vk::tl::nat_var>()) {
      params_order.push_back(as_nat_var->var_num);
    } else if (auto as_type_var = child->as<vk::tl::type_var>()) {
      params_order.push_back(as_type_var->var_num);
    }
    idx += 1;
  }
  kphp_error(var_nums == params_order,
             fmt_format("Strange tl scheme here: {}", c->name));
  // Также проверяем, что индексы аргументов типовых переменных в конструкторе совпадают с порядковым (слева направо, начиная с 0)
  // номером соответстсвующей вершины-ребенка в типовом дереве
  std::vector<int> indices_in_constructor;
  for (int i = 0; i < c->args.size(); ++i) {
    if (c->args[i]->is_type(tl)) {
      indices_in_constructor.push_back(i);
    }
  }
  std::vector<int> indices_in_type_tree;
  int i = 0;
  for (const auto &child : as_type_expr->children) {
    if (dynamic_cast<vk::tl::type_var *>(child.get())) {
      indices_in_type_tree.push_back(i);
    }
    ++i;
  }
  kphp_error(indices_in_constructor == indices_in_type_tree,
             fmt_format("Strange tl scheme here: {}", c->name));
}
}
