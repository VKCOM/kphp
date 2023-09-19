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
#include "compiler/data/src-file.h"
#include "compiler/vertex-util.h"
#include "compiler/inferring/type-data.h"
#include "compiler/name-gen.h"
#include "compiler/type-hint.h"
#include "compiler/utils/string-utils.h"
#include "compiler/data/define-data.h"

const char *ClassData::NAME_OF_VIRT_CLONE = "__virt_clone$";
const char *ClassData::NAME_OF_CLONE = "__clone";
const char *ClassData::NAME_OF_CONSTRUCT = "__construct";
const char *ClassData::NAME_OF_TO_STRING = "__toString";
const char *ClassData::NAME_OF_WAKEUP = "__wakeup";

ClassData::ClassData(ClassType type) :
  class_type(type),
  members(this),
  type_data(TypeData::create_for_class(ClassPtr(this))) {
}

void ClassData::set_name_and_src_name(const std::string &full_name) {
  this->name = full_name;
  this->src_name = std::string("C$").append(replace_backslashes(full_name));
  this->cpp_filename = replace_characters(src_name + ".cpp", '$', '@');
  this->h_filename = replace_characters(src_name + ".h", '$', '@');
  this->type_hint = TypeHintInstance::create(full_name);

  size_t pos = full_name.find_last_of('\\');
  std::string namespace_name = pos == std::string::npos ? "" : full_name.substr(0, pos);
  std::string class_name = pos == std::string::npos ? full_name : full_name.substr(pos + 1);

  this->can_be_php_autoloaded = file_id && namespace_name == file_id->namespace_name && class_name == file_id->short_file_name;
  this->can_be_php_autoloaded |= this->is_builtin();

  this->is_lambda = vk::string_view{full_name}.starts_with("Lambda$") || vk::string_view{full_name}.starts_with("ITyped$");
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

std::string ClassData::as_human_readable() const {
  if (is_typed_callable_interface()) {
    const auto *m_invoke = get_instance_method("__invoke");
    return m_invoke ? m_invoke->function->as_human_readable() : name;
  }
  if (is_ffi_scope()) {
    return "ffi_scope<" + name.substr(6) + ">";
  }
  return name;
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
  clone_this->class_id = get_self();

  std::string virt_clone_f_name = replace_backslashes(name) + "$$" + NAME_OF_VIRT_CLONE;

  auto param_list = VertexAdaptor<op_func_param_list>::create(gen_param_this({}));
  auto body = !modifiers.is_abstract()
              ? VertexAdaptor<op_seq>::create(VertexAdaptor<op_return>::create(clone_this))
              : VertexAdaptor<op_seq>::create();
  auto v_op_function = VertexAdaptor<op_function>::create(param_list, body);

  FunctionPtr f_virt_clone = FunctionData::create_function(virt_clone_f_name, v_op_function, FunctionData::func_local);
  f_virt_clone->file_id = file_id;
  f_virt_clone->return_typehint = type_hint;
  f_virt_clone->is_inline = true;
  f_virt_clone->modifiers = FunctionModifiers::instance_public();
  f_virt_clone->root.set_location_recursively(Location(location_line_num));

  members.add_instance_method(f_virt_clone);    // don't need a lock here, it's invoked from a sync pipe
  return f_virt_clone;
}

void ClassData::add_class_constant() {
  members.add_constant("class", VertexUtil::create_string_const(get_self()->name), AccessModifiers::public_);
}

void ClassData::create_constructor_with_parent_call(DataStream<FunctionPtr> &os) {
  auto parent_constructor = parent_class->construct_function;
  auto list = parent_constructor->root->param_list().clone();
  // skip parent's this
  list = VertexAdaptor<op_func_param_list>::create(VertexRange{list->params().begin() + 1, list->params().end()});

  auto parent_call = VertexAdaptor<op_func_call>::create(parent_constructor->get_params_as_vector_of_vars(1));
  parent_call->set_string("parent::__construct");
  has_custom_constructor = true;

  create_constructor(list, VertexAdaptor<op_seq>::create(parent_call), parent_constructor->phpdoc, os);
  construct_function->is_auto_inherited = true;
}

void ClassData::create_default_constructor_if_required(DataStream<FunctionPtr> &os) {
  if (!is_class() || modifiers.is_abstract() || construct_function || name == "KphpConfiguration") {
    return;
  }

  if (parent_class && parent_class->has_custom_constructor) {
    create_constructor_with_parent_call(os);
  } else {
    create_constructor(VertexAdaptor<op_func_param_list>::create(), VertexAdaptor<op_seq>::create(), {}, os);
  }
}

void ClassData::create_constructor(VertexAdaptor<op_func_param_list> param_list, VertexAdaptor<op_seq> body, const PhpDocComment *phpdoc, DataStream<FunctionPtr> &os) {
  auto func = VertexAdaptor<op_function>::create(param_list, body);
  func.set_location_recursively(Location{location_line_num});
  create_constructor(func);
  construct_function->phpdoc = phpdoc;

  G->register_and_require_function(construct_function, os, true);
}

void ClassData::create_constructor(VertexAdaptor<op_function> func) {
  std::string func_name = replace_backslashes(name) + "$$" + NAME_OF_CONSTRUCT;
  auto this_param = gen_param_this(func->get_location());
  func->param_list_ref() = VertexAdaptor<op_func_param_list>::create(this_param, func->param_list()->params());

  VertexUtil::func_force_return(func, gen_vertex_this(func->location));

  auto ctor_function = FunctionData::create_function(func_name, func, FunctionData::func_local);
  ctor_function->update_location_in_body();
  ctor_function->is_inline = true;
  ctor_function->modifiers = FunctionModifiers::instance_public();
  members.add_instance_method(ctor_function);
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

static const ClassMemberInstanceMethod *find_method_in_interface(InterfacePtr interface, vk::string_view local_name) {
  // we don't lock the class here: it's assumed, that members are not inserted concurrently
  if (const auto *member = interface->members.find_by_local_name<ClassMemberInstanceMethod>(local_name)) {
    return member;
  }

  // interfaces can extend multiple other interfaces; these interfaces are stored in 'implements' member
  for (InterfacePtr base_interface : interface->implements) {
    if (const auto *member = find_method_in_interface(base_interface, local_name)) {
      return member;
    }
  }
  return nullptr;
}

const ClassMemberInstanceMethod *ClassData::find_instance_method_by_local_name(vk::string_view local_name) const {
  const ClassMemberInstanceMethod *result{nullptr};

  ClassPtr current = get_self();
  while (current) {
    // we don't lock the class here: it's assumed, that members are not inserted concurrently
    const auto *member = current->members.find_by_local_name<ClassMemberInstanceMethod>(local_name);

    if (member) {
      if (!member->function->modifiers.is_abstract()) {
        return member;
      }
      if (!result) {
        result = member;
      }
    }

    // if we already found some abstract implementation, don't bother with interfaces traversal
    if (!result) {
      for (InterfacePtr interface : current->implements) {
        if (const auto *member = find_method_in_interface(interface, local_name)) {
          result = member;
          break;
        }
      }
    }

    current = current->parent_class;
  }

  return result;
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

std::vector<ClassPtr> ClassData::get_all_derived_classes() const {
  std::vector<ClassPtr> inheritors{get_self()};
  for (int last_derived_indx = 0; last_derived_indx != inheritors.size(); ++last_derived_indx) {
    const auto &cur_derived = inheritors[last_derived_indx]->derived_classes;
    inheritors.insert(inheritors.end(), cur_derived.begin(), cur_derived.end());
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
  return is_ffi_cdata() || (file_id && file_id->is_builtin());
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

bool ClassData::does_need_codegen() const {
  return !is_builtin() && !is_trait() &&
         (really_used || is_tl_class);
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

  // todo why we are setting it to parents, not only to child classes?
  for (auto parent_interface : implements) {
    parent_interface->set_atomic_field_deeply<field_ptr>();
  }

  if (parent_class) {
    parent_class->set_atomic_field_deeply<field_ptr>();
  }
}

void ClassData::deeply_require_to_array_debug_visitor() {
  set_atomic_field_deeply<&ClassData::need_to_array_debug_visitor>();
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
  auto full_class_name = resolve_uses(cur_function, static_cast<std::string>(class_name));
  str_dependents.emplace_back(type, full_class_name);
}

void ClassData::register_defines() const {
  members.for_each([this](const ClassMemberConstant &c) {
    auto *data = new DefineData(std::string{c.global_name()}, c.value, DefineData::def_unknown);
    data->file_id = file_id;
    data->access = c.access;
    data->class_id = get_self();
    G->register_define(DefinePtr(data));
  });
}

std::vector<const ClassMemberInstanceField *> ClassData::get_job_shared_memory_pieces() const {
  auto shared_memory_piece_interface = G->get_class("KphpJobWorkerSharedMemoryPiece");
  kphp_assert(shared_memory_piece_interface);

  std::vector<const ClassMemberInstanceField *> res;
  for (ClassPtr ancestor : get_all_ancestors()) {
    ancestor->members.for_each([&](const ClassMemberInstanceField &field) {
      for (ClassPtr field_class : field.get_inferred_type()->class_types()) {
        if (shared_memory_piece_interface->is_parent_of(field_class)) {
          res.emplace_back(&field);
        }
      }
    });
  }
  return res;
}
