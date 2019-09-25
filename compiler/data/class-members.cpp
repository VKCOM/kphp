#include "compiler/data/class-members.h"

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
#include "common/algorithms/hashes.h"

const string &ClassMemberStaticMethod::global_name() const {
  return function->name;
}

string ClassMemberStaticMethod::local_name() const {
  return function->local_name();
}


const string &ClassMemberInstanceMethod::global_name() const {
  return function->name;
}

string ClassMemberInstanceMethod::local_name() const {
  return function->local_name();
}


inline ClassMemberStaticField::ClassMemberStaticField(ClassPtr klass, VertexAdaptor<op_var> root, VertexPtr init_val, FieldModifiers modifiers, const vk::string_view &phpdoc_str) :
  modifiers(modifiers),
  full_name(replace_backslashes(klass->name) + "$$" + root->get_string()),
  root(root),
  init_val(init_val),
  phpdoc_str(phpdoc_str) {}

const string &ClassMemberStaticField::global_name() const {
  return full_name;
}

const string &ClassMemberStaticField::local_name() const {
  return root->str_val;
}

const TypeData *ClassMemberStaticField::get_inferred_type() const {
  return tinf::get_type(root->var_id);
}


const string &ClassMemberInstanceField::local_name() const {
  return root->str_val;
}

/**
 * Если при объявлении поля класса написано / ** @var int|false * / к примеру, делаем type_rule из phpdoc.
 * Это заставит type inferring принимать это во внимание, и если где-то выведется по-другому, будет ошибка.
 * С инстансами это тоже работает, т.е. / ** @var \AnotherClass * / будет тоже проверяться при выводе типов.
 */
void ClassMemberInstanceField::process_phpdoc() {
  if (auto tag_phpdoc = phpdoc_find_tag_as_string(phpdoc_str, php_doc_tag::var)) {
    auto klass = var->class_id;
    auto parsed = phpdoc_parse_type_and_var_name(*tag_phpdoc, klass->file_id->main_function);
    if (!kphp_error(parsed, format("Failed to parse phpdoc of %s::$%s", klass->name.c_str(), local_name().c_str()))) {
      parsed.type_expr->location = root->location;
      root->type_rule = VertexAdaptor<op_set_check_type_rule>::create(parsed.type_expr);
    }
  }
}

ClassMemberInstanceField::ClassMemberInstanceField(ClassPtr klass, VertexAdaptor<op_var> root, VertexPtr def_val, FieldModifiers modifiers, const vk::string_view &phpdoc_str) :
  modifiers(modifiers),
  root(root),
  phpdoc_str(phpdoc_str) {

  var = G->create_var(local_name(), VarData::var_instance_t);
  root->var_id = var;
  var->init_val = def_val;
  var->class_id = klass;
  var->marked_as_const = klass->is_immutable || phpdoc_tag_exists(phpdoc_str, php_doc_tag::kphp_const);
}

const TypeData *ClassMemberInstanceField::get_inferred_type() const {
  return tinf::get_type(var);
}

inline ClassMemberConstant::ClassMemberConstant(ClassPtr klass, const string &const_name, VertexPtr value) :
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
  auto hash_num = vk::std_hash(hash_name);
  kphp_error(names_hashes.insert(hash_num).second,
             format("Redeclaration of %s::%s", klass->name.c_str(), hash_name.c_str()));
  get_all_of<MemberT>().push_back(member);
  //printf("append %s::%s\n", klass->name.c_str(), hash_name.c_str());
}

inline bool ClassMembersContainer::member_exists(const string &hash_name) const {
  return names_hashes.find(vk::std_hash(hash_name)) != names_hashes.end();
}


void ClassMembersContainer::add_static_method(FunctionPtr function) {
  string hash_name = function->name + "()";     // не local_name из-за context-наследования, там коллизии
  append_member(hash_name, ClassMemberStaticMethod(function));
  // стоит помнить, что сюда попадают все функции при парсинге, даже которые не required в итоге могут получиться

  function->class_id = klass;
  function->context_class = klass;
  function->is_virtual_method |= function->modifiers.is_abstract();
}

void ClassMembersContainer::add_instance_method(FunctionPtr function) {
  string hash_name = function->local_name() + "()";
  append_member(hash_name, ClassMemberInstanceMethod(function));
  // стоит помнить, что сюда попадают все функции при парсинге, даже которые не required в итоге могут получиться

  function->class_id = klass;
  function->context_class = klass;

  auto rule_this_var = GenTree::create_type_help_class_vertex(klass);
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

void ClassMembersContainer::add_static_field(VertexAdaptor<op_var> root, VertexPtr init_val, FieldModifiers modifiers, const vk::string_view &phpdoc_str) {
  string hash_name = "$" + root->str_val;
  append_member(hash_name, ClassMemberStaticField(klass, root, init_val, modifiers, phpdoc_str));
}

void ClassMembersContainer::add_instance_field(VertexAdaptor<op_var> root, VertexPtr def_val, FieldModifiers modifiers, const vk::string_view &phpdoc_str) {
  string hash_name = "$" + root->str_val;
  append_member(hash_name, ClassMemberInstanceField(klass, root, def_val, modifiers, phpdoc_str));
}

void ClassMembersContainer::add_constant(string const_name, VertexPtr value) {
  const string &hash_name = const_name;
  append_member(hash_name, ClassMemberConstant(klass, const_name, value));
}

void ClassMembersContainer::safe_add_instance_method(FunctionPtr function) {
  AutoLocker<Lockable *> locker(&(*klass));
  add_instance_method(function);
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

FunctionPtr ClassMembersContainer::get_constructor() const {
  if (auto construct_method = get_instance_method(ClassData::NAME_OF_CONSTRUCT)) {
    return construct_method->function;
  }
  return {};
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

ClassMemberInstanceMethod *ClassMembersContainer::get_instance_method(const string &local_name) {
  const auto *container = this;
  return const_cast<ClassMemberInstanceMethod *>(container->get_instance_method(local_name));
}

ClassMemberStaticMethod *ClassMembersContainer::get_static_method(const string &local_name) {
  const auto *container = this;
  return const_cast<ClassMemberStaticMethod *>(container->get_static_method(local_name));
}

