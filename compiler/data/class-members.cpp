// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/class-members.h"

#include "common/algorithms/contains.h"
#include "common/algorithms/hashes.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"
#include "compiler/debug.h"
#include "compiler/gentree.h"
#include "compiler/inferring/public.h"
#include "compiler/name-gen.h"
#include "compiler/phpdoc.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"

vk::string_view ClassMemberStaticMethod::global_name() const & {
  return function->name;
}

vk::string_view ClassMemberStaticMethod::local_name() const & {
  return function->local_name();
}

std::string ClassMemberStaticMethod::hash_name(vk::string_view name) {
  return std::string{name} + "()";
}

std::string ClassMemberStaticMethod::get_hash_name() const {
  return hash_name(global_name());
}

const string &ClassMemberInstanceMethod::global_name() const {
  return function->name;
}

vk::string_view ClassMemberInstanceMethod::local_name() const & {
  return function->local_name();
}

std::string ClassMemberInstanceMethod::hash_name(vk::string_view name) {
  return ClassMemberStaticMethod::hash_name(name);
}

std::string ClassMemberInstanceMethod::get_hash_name() const {
  return hash_name(local_name());
}

inline ClassMemberStaticField::ClassMemberStaticField(ClassPtr klass, VertexAdaptor<op_var> root, VertexPtr def_val, FieldModifiers modifiers, vk::string_view phpdoc_str) :
  modifiers(modifiers),
  root(root),
  phpdoc_str(phpdoc_str) {

  std::string global_var_name = replace_backslashes(klass->name) + "$$" + root->get_string();
  var = G->get_global_var(global_var_name, VarData::var_global_t, def_val);
  root->var_id = var;
  var->init_val = def_val;
  var->class_id = klass;
}

vk::string_view ClassMemberStaticField::local_name() const & {
  return root->str_val;
}

std::string ClassMemberStaticField::hash_name(vk::string_view name) {
  return "$" + name;
}

std::string ClassMemberStaticField::get_hash_name() const {
  return hash_name(local_name());
}

const TypeData *ClassMemberStaticField::get_inferred_type() const {
  return tinf::get_type(var);
}

vk::string_view ClassMemberInstanceField::local_name() const & {
  return var->name;
}

std::string ClassMemberInstanceField::hash_name(vk::string_view name) {
  return ClassMemberStaticField::hash_name(name);
}

std::string ClassMemberInstanceField::get_hash_name() const {
  return hash_name(local_name());
}

ClassMemberInstanceField::ClassMemberInstanceField(ClassPtr klass, VertexAdaptor<op_var> root, VertexPtr def_val, FieldModifiers modifiers, vk::string_view phpdoc_str) :
  modifiers(modifiers),
  root(root),
  phpdoc_str(phpdoc_str) {

  std::string local_var_name = root->get_string();
  var = G->create_var(local_var_name, VarData::var_instance_t);
  root->var_id = var;
  var->init_val = def_val;
  var->class_id = klass;
  var->marked_as_const = klass->is_immutable || phpdoc_tag_exists(phpdoc_str, php_doc_tag::kphp_const);
}

const TypeData *ClassMemberInstanceField::get_inferred_type() const {
  return tinf::get_type(var);
}

inline ClassMemberConstant::ClassMemberConstant(ClassPtr klass, const string &const_name, VertexPtr value, AccessModifiers access) :
  value(value),
  access(access) {
  define_name = "c#" + replace_backslashes(klass->name) + "$$" + const_name;
}

vk::string_view ClassMemberConstant::global_name() const & {
  return define_name;
}

vk::string_view ClassMemberConstant::local_name() const & {
  return get_local_name_from_global_$$(define_name);
}

vk::string_view ClassMemberConstant::hash_name(vk::string_view name) {
  return name;
}

std::string ClassMemberConstant::get_hash_name() const {
  return std::string{hash_name(local_name())};
}

template <class MemberT>
void ClassMembersContainer::append_member(MemberT &&member) {
  auto hash_num = vk::std_hash(member.get_hash_name());
  kphp_error(names_hashes.insert(hash_num).second,
             fmt_format("Redeclaration of {}::{}", klass->name, member.get_hash_name()));
  get_all_of<MemberT>().push_back(std::forward<MemberT>(member));
  //printf("append %s::%s\n", klass->name.c_str(), hash_name.c_str());
}

inline bool ClassMembersContainer::member_exists(vk::string_view hash_name) const {
  return vk::contains(names_hashes, vk::std_hash(hash_name));
}


void ClassMembersContainer::add_static_method(FunctionPtr function) {
  append_member(ClassMemberStaticMethod{function});
  // note that this method gets all functions during the parsing; some of them may not be required

  function->class_id = klass;
  function->context_class = klass;
  function->is_virtual_method |= function->modifiers.is_abstract();
}

void ClassMembersContainer::add_instance_method(FunctionPtr function) {
  append_member(ClassMemberInstanceMethod{function});
  // note that this method gets all functions during the parsing; some of them may not be required

  function->class_id = klass;
  function->context_class = klass;

  auto rule_this_var = GenTree::create_type_help_class_vertex(klass->name);
  rule_this_var->class_ptr = klass;
  auto this_var = function->root->params()->args()[0].as<op_func_param>()->var();
  this_var->type_rule = VertexAdaptor<op_common_type_rule>::create(rule_this_var);

  if (klass->is_interface()) {
    function->modifiers.set_abstract();
  }
  function->is_virtual_method |= function->modifiers.is_abstract();

  if (vk::string_view(function->name).ends_with(ClassData::NAME_OF_CONSTRUCT)) {
    klass->construct_function = function;
  }
}

void ClassMembersContainer::add_static_field(VertexAdaptor<op_var> root, VertexPtr def_val, FieldModifiers modifiers, vk::string_view phpdoc_str) {
  append_member(ClassMemberStaticField{klass, root, def_val, modifiers, phpdoc_str});
}

void ClassMembersContainer::add_instance_field(VertexAdaptor<op_var> root, VertexPtr def_val, FieldModifiers modifiers, vk::string_view phpdoc_str) {
  append_member(ClassMemberInstanceField{klass, root, def_val, modifiers, phpdoc_str});
}

void ClassMembersContainer::add_constant(const std::string &const_name, VertexPtr value, AccessModifiers access) {
  append_member(ClassMemberConstant{klass, const_name, value, access});
}

void ClassMembersContainer::safe_add_instance_method(FunctionPtr function) {
  AutoLocker<Lockable *> locker(&(*klass));
  add_instance_method(function);
}

bool ClassMembersContainer::has_constant(vk::string_view local_name) const {
  return member_exists(ClassMemberConstant::hash_name(local_name));
}

bool ClassMembersContainer::has_field(vk::string_view local_name) const {
  return member_exists(ClassMemberStaticField::hash_name(local_name));          // either static or instance field — they can't share the same name
}

bool ClassMembersContainer::has_instance_method(vk::string_view local_name) const {
  return member_exists(ClassMemberInstanceMethod::hash_name(local_name));          // either static or instance method — they can't share the same name
}

bool ClassMembersContainer::has_static_method(vk::string_view local_name) const {
  return member_exists(ClassMemberStaticMethod::hash_name(replace_backslashes(klass->name) + "$$" + local_name));
}

bool ClassMembersContainer::has_any_instance_var() const {
  return !instance_fields.empty();
}

bool ClassMembersContainer::has_any_instance_method() const {
  return !instance_methods.empty();
}

bool ClassMembersContainer::has_any_static_var() const {
  return !static_fields.empty();
}

bool ClassMembersContainer::has_any_static_method() const {
  return !static_methods.empty();
}

FunctionPtr ClassMembersContainer::get_constructor() const {
  if (auto construct_method = get_instance_method(ClassData::NAME_OF_CONSTRUCT)) {
    return construct_method->function;
  }
  return {};
}

size_t ClassMembersContainer::count_of_instance_methods() const {
  return instance_methods.size();
}

const ClassMemberStaticMethod *ClassMembersContainer::get_static_method(vk::string_view local_name) const {
  return find_by_local_name<ClassMemberStaticMethod>(local_name);
}

const ClassMemberInstanceMethod *ClassMembersContainer::get_instance_method(vk::string_view local_name) const {
  return find_by_local_name<ClassMemberInstanceMethod>(local_name);
}

const ClassMemberStaticField *ClassMembersContainer::get_static_field(vk::string_view local_name) const {
  return find_by_local_name<ClassMemberStaticField>(local_name);
}

const ClassMemberInstanceField *ClassMembersContainer::get_instance_field(vk::string_view local_name) const {
  return find_by_local_name<ClassMemberInstanceField>(local_name);
}

const ClassMemberConstant *ClassMembersContainer::get_constant(vk::string_view local_name) const {
  return find_by_local_name<ClassMemberConstant>(local_name);
}

ClassMemberInstanceMethod *ClassMembersContainer::get_instance_method(vk::string_view local_name) {
  const auto *container = this;
  return const_cast<ClassMemberInstanceMethod *>(container->get_instance_method(local_name));
}

ClassMemberStaticMethod *ClassMembersContainer::get_static_method(vk::string_view local_name) {
  const auto *container = this;
  return const_cast<ClassMemberStaticMethod *>(container->get_static_method(local_name));
}

