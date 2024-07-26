// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

/*
 * Classes have static/instance fields and methods; they can also have constants (see struct ClassMember*).
 *
 * ClassData has a members field that is used for the compile-time reflection.
 * Some examples:
 * - Walk all static fields;
 * - Check whether a class defines a specific constant;
 * - Find a method by some condition.
 *
 * Methods, static fields and constants are converted into global functions/variables/defines using the
 * Namespace$Class$Name$$local_name naming scheme. The 'local_name' is a name inside a class,
 * 'global_name' is a name that can be used from the outside.
 *
 * Contextual functions for the static inheritance (and self/parent/static implementation as well)
 * has a naming like Namespace$ClassName$$local_name$$Namespace$ClassContextName.
 */
#pragma once

#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "common/type_traits/function_traits.h"

#include "compiler/data/data_ptr.h"
#include "compiler/data/field-modifiers.h"
#include "compiler/data/function-modifiers.h"
#include "compiler/data/vertex-adaptor.h"

class TypeData;
class ClassMembersContainer;
namespace kphp_json { class KphpJsonTagList; }
namespace kphp_msgpack { class KphpMsgPackTagList; }

struct ClassMemberStaticMethod {
  FunctionPtr function;

  explicit ClassMemberStaticMethod(FunctionPtr function) :
    function(std::move(function)) {}

  vk::string_view global_name() const &;
  vk::string_view global_name() const && = delete;

  vk::string_view local_name() const &;
  vk::string_view local_name() const && = delete;

  // can't use name() due to the context-inheritance (it introduces collisions)
  static std::string hash_name(vk::string_view name);
  std::string get_hash_name() const;
};

struct ClassMemberInstanceMethod {
  FunctionPtr function;

  explicit ClassMemberInstanceMethod(FunctionPtr function) :
    function(std::move(function)) {}

  const std::string &global_name() const;
  vk::string_view local_name() const &;
  static std::string hash_name(vk::string_view name);
  std::string get_hash_name() const;
};

struct ClassMemberStaticField {
  FieldModifiers modifiers;
  VertexAdaptor<op_var> root;
  VarPtr var;
  const PhpDocComment *phpdoc{nullptr};
  const TypeHint *type_hint{nullptr};  // from @var / php 7.4 type hint / default value

  ClassMemberStaticField(ClassPtr klass, VertexAdaptor<op_var> root, VertexPtr def_val, FieldModifiers modifiers, const PhpDocComment *phpdoc, const TypeHint *type_hint);

  vk::string_view local_name() const &;
  static std::string hash_name(vk::string_view name);
  std::string get_hash_name() const;
  const TypeData *get_inferred_type() const;
};

struct ClassMemberInstanceField {
  FieldModifiers modifiers;
  VertexAdaptor<op_var> root;
  VarPtr var;
  const PhpDocComment *phpdoc{nullptr};
  const kphp_json::KphpJsonTagList *kphp_json_tags{nullptr};
  const kphp_msgpack::KphpMsgPackTagList *kphp_msgpack_tags{nullptr};
  const TypeHint *type_hint{nullptr};  // from @var / php 7.4 type hint / default value
  int8_t serialization_tag = -1;
  bool serialize_as_float32{false};

  ClassMemberInstanceField(ClassPtr klass, VertexAdaptor<op_var> root, VertexPtr def_val, FieldModifiers modifiers, const PhpDocComment *phpdoc, const TypeHint *type_hint);

  vk::string_view local_name() const &;
  vk::string_view local_name() const && = delete;

  static std::string hash_name(vk::string_view name);
  std::string get_hash_name() const;
  const TypeData *get_inferred_type() const;
};

struct ClassMemberConstant {
  std::string define_name;
  VertexPtr value;
  AccessModifiers access;

  ClassMemberConstant(ClassPtr klass, const std::string &const_name, VertexPtr value, AccessModifiers access);

  vk::string_view global_name() const &;
  vk::string_view global_name() const && = delete;

  vk::string_view local_name() const &;
  vk::string_view local_name() const && = delete;
  static vk::string_view hash_name(vk::string_view name);
  std::string get_hash_name() const;
};


/*
   —————————————————————————————————————

 * Every PHP-class (ClassData) has members (ClassMembersContainer).
 * ClassMembersContainer contains all PHP-class members and defines accessors to them.
 * All members are ordered like they appear in the source code.
 * It also provides a fast way to check symbol existence by its name (it's especially useful for constants).
 */
class ClassMembersContainer {
  ClassPtr klass;

  std::list<ClassMemberStaticMethod> static_methods;
  std::list<ClassMemberInstanceMethod> instance_methods;
  std::list<ClassMemberStaticField> static_fields;
  std::list<ClassMemberInstanceField> instance_fields;
  std::list<ClassMemberConstant> constants;
  std::set<uint64_t> names_hashes;

  template<class MemberT>
  void append_member(MemberT &&member);
  bool member_exists(vk::string_view hash_name) const;

  template<class CallbackT>
  using GetMemberT = vk::decay_function_arg_t<CallbackT, 0>;

  // appropriate list selection out of the static_methods/instance_methods/etc; see implementations below
  template<class MemberT>
  std::list<MemberT> &get_all_of() {
    static_assert(sizeof(MemberT) == -1u, "Invalid template MemberT parameter");
    return {};
  }

  template<class MemberT>
  const std::list<MemberT> &get_all_of() const { return const_cast<ClassMembersContainer *>(this)->get_all_of<MemberT>(); }

public:
  explicit ClassMembersContainer(ClassData *klass) :
    klass(ClassPtr(klass)) {}

  template<class CallbackT>
  inline void for_each(CallbackT callback) const {
    const auto &container = get_all_of<GetMemberT<CallbackT>>();
    std::for_each(container.cbegin(), container.cend(), std::move(callback));
  }

  template<class CallbackT>
  inline void for_each(CallbackT callback) {
    auto &container = get_all_of<GetMemberT<CallbackT>>();
    std::for_each(container.begin(), container.end(), std::move(callback));
  }

  template<class CallbackT>
  inline void remove_if(CallbackT callbackReturningBool) {
    auto &container = get_all_of<GetMemberT<CallbackT>>();
    container.remove_if(std::move(callbackReturningBool));
  }

  template<class CallbackT>
  inline const GetMemberT<CallbackT> *find_member(CallbackT callbackReturningBool) const {
    const auto &container = get_all_of<GetMemberT<CallbackT>>();
    auto it = std::find_if(container.cbegin(), container.cend(), std::move(callbackReturningBool));
    return it == container.cend() ? nullptr : &(*it);
  }

  template<class MemberT>
  inline const MemberT *find_by_local_name(vk::string_view local_name) const {
    return find_member([&local_name](const MemberT &f) { return f.local_name() == local_name; });
  }

  void add_static_method(FunctionPtr function);
  void add_instance_method(FunctionPtr function);
  void add_static_field(VertexAdaptor<op_var> root, VertexPtr def_val, FieldModifiers modifiers, const PhpDocComment *phpdoc, const TypeHint *type_hint);
  void add_instance_field(VertexAdaptor<op_var> root, VertexPtr def_val, FieldModifiers modifiers, const PhpDocComment *phpdoc, const TypeHint *type_hint);
  void add_constant(const std::string &const_name, VertexPtr value, AccessModifiers access);

  bool has_constant(vk::string_view local_name) const;
  bool has_field(vk::string_view local_name) const;
  bool has_instance_method(vk::string_view local_name) const;
  bool has_static_method(vk::string_view local_name) const;

  bool has_any_instance_var() const { return !instance_fields.empty(); }
  bool has_any_instance_method() const { return !instance_methods.empty(); }
  bool has_any_static_var() const { return !static_fields.empty(); }
  bool has_any_static_method() const { return !static_methods.empty(); }

  const ClassMemberStaticMethod *get_static_method(vk::string_view local_name) const;
  const ClassMemberInstanceMethod *get_instance_method(vk::string_view local_name) const;
  const ClassMemberStaticField *get_static_field(vk::string_view local_name) const;
  const ClassMemberInstanceField *get_instance_field(vk::string_view local_name) const;
  const ClassMemberConstant *get_constant(vk::string_view local_name) const;

  ClassMemberStaticMethod *get_static_method(vk::string_view local_name);
  ClassMemberInstanceMethod *get_instance_method(vk::string_view local_name);
  FunctionPtr get_constructor() const;
};

/*
 * All class methods/fields are turned into the global functions/defines, for example:
 * "Classes$A$$method", "c#Classes$A$$constname"
 * Text after the "$$" matches the class local name (like defied in the source code).
 */
inline vk::string_view get_local_name_from_global_$$(vk::string_view global_name) {
  auto pos$$ = global_name.find("$$");
  return pos$$ == std::string::npos ? global_name : global_name.substr(pos$$ + 2);
}

template<class T>
inline vk::string_view get_local_name_from_global_$$(T &&global_name) {
  static_assert(!std::is_rvalue_reference<decltype(global_name)>{}, "get_local_name should be called only from lvalues");
  return get_local_name_from_global_$$(vk::string_view{global_name});
}

template<>
inline std::list<ClassMemberStaticMethod> &ClassMembersContainer::get_all_of<ClassMemberStaticMethod>() { return static_methods; }
template<>
inline std::list<ClassMemberInstanceMethod> &ClassMembersContainer::get_all_of<ClassMemberInstanceMethod>() { return instance_methods; }
template<>
inline std::list<ClassMemberStaticField> &ClassMembersContainer::get_all_of<ClassMemberStaticField>() { return static_fields; }
template<>
inline std::list<ClassMemberInstanceField> &ClassMembersContainer::get_all_of<ClassMemberInstanceField>() { return instance_fields; }
template<>
inline std::list<ClassMemberConstant> &ClassMembersContainer::get_all_of<ClassMemberConstant>() { return constants; }
