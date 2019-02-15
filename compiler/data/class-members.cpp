#include "compiler/data/class-members.h"

#include "common/wrappers/string_view.h"

#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/var-data.h"
#include "compiler/debug.h"
#include "compiler/inferring/public.h"
#include "compiler/name-gen.h"
#include "compiler/vertex.h"

const string &ClassMemberStaticMethod::global_name() const {
  return function->name;
}

string ClassMemberStaticMethod::local_name() const {
  return get_local_name_from_global_$$(global_name());
}


const string &ClassMemberInstanceMethod::global_name() const {
  return function->name;
}

string ClassMemberInstanceMethod::local_name() const {
  return get_local_name_from_global_$$(global_name());
}


inline ClassMemberStaticField::ClassMemberStaticField(ClassPtr klass, VertexAdaptor<op_var> root, VertexPtr init_val, AccessType access_type, Token *phpdoc_token) :
  access_type(access_type),
  full_name(replace_backslashes(klass->name) + "$$" + root->get_string()),
  root(root),
  init_val(init_val),
  phpdoc_token(phpdoc_token) {}

const string &ClassMemberStaticField::global_name() const {
  return full_name;
}

const string &ClassMemberStaticField::local_name() const {
  return root->str_val;
}

const TypeData *ClassMemberStaticField::get_inferred_type() const {
  return tinf::get_type(root->get_var_id());
}


const string &ClassMemberInstanceField::local_name() const {
  return root->str_val;
}

inline ClassMemberInstanceField::ClassMemberInstanceField(ClassPtr klass, VertexAdaptor<op_class_var> root, AccessType access_type) :
  access_type(access_type),
  root(root),
  phpdoc_token(root->phpdoc_token) {

  var = VarPtr(new VarData(VarData::var_instance_t));
  var->class_id = klass;
  var->name = local_name();
}

const TypeData *ClassMemberInstanceField::get_inferred_type() const {
  return tinf::get_type(var);
}


inline ClassMemberConstant::ClassMemberConstant(ClassPtr klass, string const_name, VertexPtr value) :
  value(value) {
  define_name = "c#" + replace_backslashes(klass->name) + "$$" + const_name;
}

const string &ClassMemberConstant::global_name() const {
  return define_name;
}

string ClassMemberConstant::local_name() const {
  return get_local_name_from_global_$$(define_name);
}


template <class MemberT>
void ClassMembersContainer::append_member(const string &hash_name, const MemberT &member) {
  unsigned long long hash_num = hash_ll(hash_name);
  kphp_error(names_hashes.insert(hash_num).second,
             format("Redeclaration of %s::%s", klass->name.c_str(), hash_name.c_str()));
  get_all_of<MemberT>().push_back(member);
  //printf("append %s::%s\n", klass->name.c_str(), hash_name.c_str());
}

inline bool ClassMembersContainer::member_exists(const string &hash_name) const {
  return names_hashes.find(hash_ll(hash_name)) != names_hashes.end();
}


void ClassMembersContainer::add_static_method(FunctionPtr function, AccessType access_type) {
  string hash_name = function->name + "()";     // не local_name из-за context-наследования, там коллизии
  append_member(hash_name, ClassMemberStaticMethod(function, access_type));
  // стоит помнить, что сюда попадают все функции при парсинге, даже которые не required в итоге могут получиться

  function->access_type = access_type;
  function->class_id = klass;
  function->context_class = klass;
}

void ClassMembersContainer::add_instance_method(FunctionPtr function, AccessType access_type) {
  string hash_name = get_local_name_from_global_$$(function->name) + "()";
  append_member(hash_name, ClassMemberInstanceMethod(function, access_type));
  // стоит помнить, что сюда попадают все функции при парсинге, даже которые не required в итоге могут получиться

  function->access_type = access_type;
  function->class_id = klass;
  function->context_class = klass;

  if (vk::string_view(function->name).ends_with("__construct")) {
    klass->construct_function = function;
  }
}

void ClassMembersContainer::add_static_field(VertexAdaptor<op_var> root, VertexPtr init_val, AccessType access_type, Token *phpdoc_token) {
  string hash_name = "$" + root->str_val;
  append_member(hash_name, ClassMemberStaticField(klass, root, init_val, access_type, phpdoc_token));
}

void ClassMembersContainer::add_instance_field(VertexAdaptor<op_class_var> root, AccessType access_type) {
  string hash_name = "$" + root->str_val;
  append_member(hash_name, ClassMemberInstanceField(klass, root, access_type));
}

void ClassMembersContainer::add_constant(string const_name, VertexPtr value) {
  const string &hash_name = const_name;
  append_member(hash_name, ClassMemberConstant(klass, const_name, value));
}


bool ClassMembersContainer::has_constant(const string &local_name) const {
  const string &hash_name = local_name;
  return member_exists(hash_name);
}

bool ClassMembersContainer::has_field(const string &local_name) const {
  string hash_name = "$" + local_name;
  return member_exists(hash_name);          // либо static, либо instance field — оба не могут существовать с одним именем
}

bool ClassMembersContainer::has_instance_method(const string &local_name) const {
  string hash_name = local_name + "()";
  return member_exists(hash_name);          // либо static, либо instance method — не может быть двух с одним именем
}

bool ClassMembersContainer::has_static_method(const string &local_name) const {
  string hash_name = replace_backslashes(klass->name) + "$$" + local_name + "()";
  return member_exists(hash_name);
}

bool ClassMembersContainer::has_any_instance_var() const {
  return !instance_fields.empty();
}

bool ClassMembersContainer::has_any_instance_method() const {
  return !instance_methods.empty();
}

bool ClassMembersContainer::has_constructor() const {
  return has_instance_method("__construct");
}


const ClassMemberStaticMethod *ClassMembersContainer::get_static_method(const string &local_name) const {
  return find_by_local_name<ClassMemberStaticMethod>(local_name);
}

const ClassMemberInstanceMethod *ClassMembersContainer::get_instance_method(const string &local_name) const {
  return find_by_local_name<ClassMemberInstanceMethod>(local_name);
}

const ClassMemberStaticField *ClassMembersContainer::get_static_field(const string &local_name) const {
  return find_by_local_name<ClassMemberStaticField>(local_name);
}

const ClassMemberInstanceField *ClassMembersContainer::get_instance_field(const string &local_name) const {
  return find_by_local_name<ClassMemberInstanceField>(local_name);
}

const ClassMemberConstant *ClassMembersContainer::get_constant(const string &local_name) const {
  return find_by_local_name<ClassMemberConstant>(local_name);
}

