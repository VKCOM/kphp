// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/class-data.h"

#include <string>
#include <queue>

#include "common/termformat/termformat.h"
#include "common/wrappers/string_view.h"

#include "compiler/compiler-core.h"
#include "compiler/data/function-data.h"
#include "compiler/data/lambda-class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/gentree.h"
#include "compiler/inferring/type-data.h"
#include "compiler/name-gen.h"
#include "compiler/phpdoc.h"
#include "compiler/utils/string-utils.h"
#include "compiler/data/define-data.h"

const char *ClassData::NAME_OF_VIRT_CLONE = "__virt_clone$";
const char *ClassData::NAME_OF_CLONE = "__clone";
const char *ClassData::NAME_OF_CONSTRUCT = "__construct";
const char *ClassData::NAME_OF_INVOKE_METHOD = "__invoke";

ClassData::ClassData() :
  members(this),
  type_data(TypeData::create_for_class(ClassPtr(this))) {
}

void ClassData::set_name_and_src_name(const string &name, vk::string_view phpdoc_str) {
  this->name = name;
  this->src_name = std::string("C$").append(replace_backslashes(name));
  this->header_name = replace_characters(src_name + ".h", '$', '@');
  this->phpdoc_str = phpdoc_str;

  size_t pos = name.find_last_of('\\');
  std::string namespace_name = pos == std::string::npos ? "" : name.substr(0, pos);
  std::string class_name = pos == std::string::npos ? name : name.substr(pos + 1);

  this->can_be_php_autoloaded = file_id && namespace_name == file_id->namespace_name && class_name == file_id->short_file_name;
  this->can_be_php_autoloaded |= this->is_builtin();
}

void ClassData::debugPrint() {
  const char *str_class_type =
    is_interface() ? "interface" :
    is_trait() ? "trait" : "class";
  fmt_print("=== {} {}\n", str_class_type, name);

  members.for_each([](ClassMemberConstant &m) {
    fmt_print("const {}\n", m.local_name());
  });
  members.for_each([](ClassMemberStaticField &m) {
    fmt_print("static ${}\n", m.local_name());
  });
  members.for_each([](ClassMemberStaticMethod &m) {
    fmt_print("static {}()\n", m.local_name());
  });
  members.for_each([](ClassMemberInstanceField &m) {
    fmt_print("var ${}\n", m.local_name());
  });
  members.for_each([](ClassMemberInstanceMethod &m) {
    fmt_print("method {}()\n", m.local_name());
  });
}

std::string ClassData::get_namespace() const {
  return file_id->namespace_name;
}

FunctionPtr ClassData::gen_holder_function(const std::string &name) {
  std::string func_name = "$" + replace_backslashes(name);  // function-wrapper for class
  auto func_params = VertexAdaptor<op_func_param_list>::create();
  auto func_body = VertexAdaptor<op_seq>::create();
  auto func_root = VertexAdaptor<op_function>::create(func_params, func_body);

  auto holder_function = FunctionData::create_function(func_name, func_root, FunctionData::func_class_holder);
  holder_function->class_id = ClassPtr{this};
  holder_function->context_class = ClassPtr{this};
  return holder_function;
}

FunctionPtr ClassData::add_magic_method(const char *magic_name, VertexPtr return_value) {
  std::string magic_func_name = replace_backslashes(name) + "$$" + magic_name;

  auto param_list = VertexAdaptor<op_func_param_list>::create(gen_param_this({}));

  VertexAdaptor<op_seq> body;
  if (!modifiers.is_abstract()) {
    auto wrapped_return_value = VertexAdaptor<op_return>::create(return_value);
    body = VertexAdaptor<op_seq>::create(wrapped_return_value);
  } else {
    body = VertexAdaptor<op_seq>::create();
  }

  auto virt_magic_func = VertexAdaptor<op_function>::create(param_list, body);
  auto virt_magic_func_ptr = FunctionData::create_function(magic_func_name, virt_magic_func, FunctionData::func_local);
  virt_magic_func_ptr->file_id = file_id;
  virt_magic_func_ptr->update_location_in_body();
  virt_magic_func_ptr->assumption_return_status = AssumptionStatus::initialized;
  virt_magic_func_ptr->assumption_for_return = Assumption(ClassPtr{this});
  virt_magic_func_ptr->is_inline = true;
  virt_magic_func_ptr->modifiers = FunctionModifiers::instance_public();
  virt_magic_func.set_location_recursively(Location(location_line_num));

  members.add_instance_method(virt_magic_func_ptr);

  return virt_magic_func_ptr;
}

/**
 * Generate and add `__virt_clone` method to class (or interface); method is dedicated for cloning interfaces
 *
 * For classes:
 * function __virt_clone() { return clone $this; }
 *
 * For interfaces:
 * / ** @return InterfaceName * /
 * function __virt_clone();
 *
 * @return generated method
 */
FunctionPtr ClassData::add_virt_clone() {
  auto clone_this = VertexAdaptor<op_clone>::create(gen_vertex_this(Location{}));
  auto virt_clone = add_magic_method(NAME_OF_VIRT_CLONE, clone_this);
  return virt_clone;
}

void ClassData::add_class_constant() {
  members.add_constant("class", GenTree::generate_constant_field_class_value(get_self()), AccessModifiers::public_);
}

void ClassData::create_constructor_with_parent_call(DataStream<FunctionPtr> &os) {
  auto parent_constructor = parent_class->construct_function;
  auto list = parent_constructor->root->param_list().clone();
  // skip parent's this
  list = VertexAdaptor<op_func_param_list>::create(VertexRange{list->params().begin() + 1, list->params().end()});

  auto parent_call = VertexAdaptor<op_func_call>::create(parent_constructor->get_params_as_vector_of_vars(1));
  parent_call->set_string("parent::__construct");
  has_custom_constructor = true;

  create_constructor(list, VertexAdaptor<op_seq>::create(parent_call), parent_constructor->phpdoc_str, os);
  construct_function->is_auto_inherited = true;
}

void ClassData::create_default_constructor_if_required(DataStream<FunctionPtr> &os) {
  if (!is_class() || modifiers.is_abstract() || construct_function) {
    return;
  }

  if (parent_class && parent_class->has_custom_constructor) {
    create_constructor_with_parent_call(os);
  } else {
    create_constructor(VertexAdaptor<op_func_param_list>::create(), VertexAdaptor<op_seq>::create(), {}, os);
  }
}

void ClassData::create_constructor(VertexAdaptor<op_func_param_list> param_list, VertexAdaptor<op_seq> body, vk::string_view phpdoc, DataStream<FunctionPtr> &os) {
  auto func = VertexAdaptor<op_function>::create(param_list, body);
  func.set_location_recursively(Location{location_line_num});
  create_constructor(func);
  construct_function->phpdoc_str = phpdoc;

  G->require_function(construct_function, os);
}

void ClassData::create_constructor(VertexAdaptor<op_function> func) {
  std::string func_name = replace_backslashes(name) + "$$" + NAME_OF_CONSTRUCT;
  auto this_param = gen_param_this(func->get_location());
  func->param_list_ref() = VertexAdaptor<op_func_param_list>::create(this_param, func->param_list()->params());

  GenTree::func_force_return(func, gen_vertex_this(func->location));

  auto ctor_function = FunctionData::create_function(func_name, func, FunctionData::func_local);
  ctor_function->update_location_in_body();
  ctor_function->is_inline = true;
  ctor_function->modifiers = FunctionModifiers::instance_public();
  members.add_instance_method(ctor_function);
  G->register_function(ctor_function);
}

bool ClassData::is_parent_of(ClassPtr other) const {
  if (!other) {
    return false;
  } else if (get_self() == other) {
    return true;
  }

  for (const auto &interface : other->implements) {
    if (is_parent_of(interface)) {
      return true;
    }
  }

  return is_parent_of(other->parent_class);
}

/**
 * If we have more than one lca in different hierarchy branches we will return all of them
 * interface A{}    ; interface B{}    ;
 * class C impl A, B; class D impl A, B;
 * $cd = true ? new C() : new D();
 */
std::vector<ClassPtr> ClassData::get_common_base_or_interface(ClassPtr other) const {
  if (!other) {
    return {};
  }

  auto self = get_self();
  if (self == other) {
    return {self};
  }

  if (is_lambda() && other->is_interface() && self.as<LambdaClassData>()->can_implement_interface(other)) {
    return {other};
  } else if (other->is_lambda() && is_interface() && other.as<LambdaClassData>()->can_implement_interface(self)) {
    return {self};
  }

  std::queue<ClassPtr> active_ancestors;
  active_ancestors.push(self);

  std::vector<ClassPtr> lcas;
  while (!active_ancestors.empty()) {
    auto cur_ancestor = active_ancestors.front();
    active_ancestors.pop();

    if (cur_ancestor->is_parent_of(other)) {
      lcas.emplace_back(cur_ancestor);
      continue;
    }

    if (cur_ancestor->parent_class) {
      active_ancestors.push(cur_ancestor->parent_class);
    }
    for (const auto &c : cur_ancestor->implements) {
      active_ancestors.push(c);
    }
  }

  return lcas;
}

const ClassMemberInstanceMethod *ClassData::get_instance_method(vk::string_view local_name) const {
  return find_by_local_name<ClassMemberInstanceMethod>(local_name);
}

const ClassMemberInstanceField *ClassData::get_instance_field(vk::string_view local_name) const {
  return find_by_local_name<ClassMemberInstanceField>(local_name);
}

const ClassMemberStaticField *ClassData::get_static_field(vk::string_view local_name) const {
  return find_by_local_name<ClassMemberStaticField>(local_name);
}

const ClassMemberConstant *ClassData::get_constant(vk::string_view local_name) const {
  return find_by_local_name<ClassMemberConstant>(local_name);
}

FunctionPtr ClassData::get_holder_function() const {
  return G->get_function("$" + replace_backslashes(name));
}

VertexAdaptor<op_var> ClassData::gen_vertex_this(Location location) {
  auto this_var = VertexAdaptor<op_var>::create();
  this_var->str_val = "this";
  this_var->extra_type = op_ex_var_this;
  this_var->const_type = cnst_const_val;
  this_var->location = location;
  this_var->is_const = true;

  return this_var;
}

std::vector<ClassPtr> ClassData::get_all_inheritors() const {
  std::vector<ClassPtr> inheritors{get_self()};
  for (int last_derived_indx = 0; last_derived_indx != inheritors.size(); ++last_derived_indx) {
    auto &new_derived = inheritors[last_derived_indx]->derived_classes;
    inheritors.insert(inheritors.end(), new_derived.begin(), new_derived.end());
  }
  return inheritors;
}

std::vector<ClassPtr> ClassData::get_all_ancestors() const {
  std::vector<ClassPtr> result{get_self()};

  for (size_t cur_idx = 0; cur_idx < result.size(); ++cur_idx) {
    auto cur_class = result[cur_idx];
    if (cur_class->parent_class) {
      result.emplace_back(cur_class->parent_class);
    }
    result.insert(result.end(), cur_class->implements.cbegin(), cur_class->implements.cend());
  }

  return result;
}

bool ClassData::is_builtin() const {
  return file_id && file_id->is_builtin();
}

bool ClassData::is_polymorphic_or_has_polymorphic_member() const {
  if (is_polymorphic_class()) {
    return true;
  }
  std::unordered_set<ClassPtr> checked{get_self()};
  return has_polymorphic_member_dfs(checked);
}

bool ClassData::has_polymorphic_member_dfs(std::unordered_set<ClassPtr> &checked) const {
  return nullptr != members.find_member(
    [&checked](const ClassMemberInstanceField &field) {
      std::unordered_set<ClassPtr> sub_classes;
      field.var->tinf_node.get_type()->get_all_class_types_inside(sub_classes);
      for (auto klass : sub_classes) {
        if (checked.insert(klass).second) {
          if (klass->is_polymorphic_class() || klass->has_polymorphic_member_dfs(checked)) {
            return true;
          }
        }
      }
      return false;
    });
}

bool ClassData::does_need_codegen(ClassPtr c) {
  return c && !c->is_builtin() && !c->is_trait() &&
         (c->really_used || c->is_tl_class);
}

bool operator<(const ClassPtr &lhs, const ClassPtr &rhs) {
  return lhs->name < rhs->name;
}

void ClassData::mark_as_used() {
  if (really_used) {
    return;
  }
  really_used = true;
  if (parent_class) {
    parent_class->mark_as_used();
  }
  for (const auto &interface : implements) {
    interface->mark_as_used();
  }
}

bool ClassData::has_no_derived_classes() const {
  return (!implements.empty() || does_need_codegen(parent_class)) && derived_classes.empty();
}

template<std::atomic<bool> ClassData::*field_ptr>
void ClassData::set_atomic_field_deeply() {
  if (this->*field_ptr) {
    return;
  }

  this->*field_ptr = true;
  members.for_each([](ClassMemberInstanceField &field) {
    std::unordered_set<ClassPtr> sub_classes;
    field.var->tinf_node.get_type()->get_all_class_types_inside(sub_classes);
    for (auto klass : sub_classes) {
      klass->set_atomic_field_deeply<field_ptr>();
    }
  });

  for (auto child : derived_classes) {
    child->set_atomic_field_deeply<field_ptr>();
  }

  for (auto parent_interface : implements) {
    parent_interface->set_atomic_field_deeply<field_ptr>();
  }

  if (parent_class) {
    parent_class->set_atomic_field_deeply<field_ptr>();
  }
}

void ClassData::deeply_require_instance_to_array_visitor() {
  set_atomic_field_deeply<&ClassData::need_instance_to_array_visitor>();
}

void ClassData::deeply_require_instance_cache_visitor() {
  set_atomic_field_deeply<&ClassData::need_instance_cache_visitors>();
}

void ClassData::deeply_require_instance_memory_estimate_visitor() {
  set_atomic_field_deeply<&ClassData::need_instance_memory_estimate_visitor>();
}

void ClassData::deeply_require_virtual_builtin_functions() {
  set_atomic_field_deeply<&ClassData::need_virtual_builtin_functions>();
}

void ClassData::add_str_dependent(FunctionPtr cur_function, ClassType type, vk::string_view class_name) {
  auto full_class_name = resolve_uses(cur_function, static_cast<std::string>(class_name), '\\');
  str_dependents.emplace_back(type, full_class_name);
}

void ClassData::register_defines() const {
  members.for_each([this](const ClassMemberConstant &c) {
    auto data = new DefineData(std::string{c.global_name()}, c.value, DefineData::def_unknown);
    data->file_id = file_id;
    data->access = c.access;
    data->class_id = get_self();
    G->register_define(DefinePtr(data));
  });
}
