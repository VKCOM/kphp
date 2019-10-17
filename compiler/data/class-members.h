/*
 *  У классов есть статические / инстанс поля, методы; есть константы. См. struct ClassMember*
 *  У ClassData есть поле members, которое предоставляет compile-time reflection, например:
 * пробежаться по всем статическим полям, проверить существование константы, найти метод по условию и т.п.
 *  Методы, статические поля и константы превращаются в глобальные функции / переменные / дефайны, по именованию
 * Namespace$Class$Name$$local_name. Терминология `local_name` — это название внутри класса, `global_name` — вот такое.
 *  Есть ещё контекстные функции для статического наследования и реализации self/parent/static, их именование вида
 * Namespace$ClassName$$local_name$$Namespace$ClassContextName.
 */
#pragma once

#include <map>
#include <set>
#include <string>
#include <utility>

#include "compiler/data/function-modifiers.h"
#include "compiler/data/field-modifiers.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"
#include "common/type_traits/function_traits.h"

class TypeData;
class ClassMembersContainer;

struct ClassMemberStaticMethod {
  FunctionPtr function;

  explicit ClassMemberStaticMethod(FunctionPtr function) :
    function(std::move(function)) {}

  vk::string_view global_name() const &;
  vk::string_view local_name() const &;

  // не просто name из-за context-наследования, там коллизии
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
  vk::string_view phpdoc_str;

  ClassMemberStaticField(ClassPtr klass, VertexAdaptor<op_var> root, VertexPtr def_val, FieldModifiers modifiers, vk::string_view phpdoc_str);

  vk::string_view local_name() const &;
  static std::string hash_name(vk::string_view name);
  std::string get_hash_name() const;
  const TypeData *get_inferred_type() const;
};

struct ClassMemberInstanceField {
  FieldModifiers modifiers;
  VertexAdaptor<op_var> root;
  VarPtr var;
  vk::string_view phpdoc_str;

  ClassMemberInstanceField(ClassPtr klass, VertexAdaptor<op_var> root, VertexPtr def_val, FieldModifiers modifiers, vk::string_view phpdoc_str);

  vk::string_view local_name() const &;
  static std::string hash_name(vk::string_view name);
  std::string get_hash_name() const;
  const TypeData *get_inferred_type() const;
  void process_phpdoc();
};

struct ClassMemberConstant {
  std::string define_name;
  VertexPtr value;

  ClassMemberConstant(ClassPtr klass, const std::string &const_name, VertexPtr value);

  vk::string_view global_name() const &;
  vk::string_view local_name() const &;
  static vk::string_view hash_name(vk::string_view name);
  std::string get_hash_name() const;
};


/*
   —————————————————————————————————————

 *  У каждого php-класса (ClassData) есть members — объект ClassMembersContainer.
 *  Собственно, он хранит все члены php-класса и предоставляет осмысленные методы доступа.
 *  Он хранит вектора — т.к. нужен порядок сохранения и кодогенерации.
 * И параллельно с ними — set хешей, для быстрой проверки существования по имени (особенно актуально для констант).
 *  Несмотря на то, что хранятся 5 векторов — функции for_each и find_member одни, и пробегаются по нужному вектору:
 * он определяется исходя из типа, как первый аргумент callback'а, см. usages.
 */
class ClassMembersContainer {
  ClassPtr klass;

  std::vector<ClassMemberStaticMethod> static_methods;
  std::vector<ClassMemberInstanceMethod> instance_methods;
  std::vector<ClassMemberStaticField> static_fields;
  std::vector<ClassMemberInstanceField> instance_fields;
  std::vector<ClassMemberConstant> contants;
  std::set<uint64_t> names_hashes;

  template<class MemberT>
  void append_member(MemberT &&member);
  bool member_exists(vk::string_view hash_name) const;

  template<class CallbackT>
  struct arg_helper {
    using MemberT = vk::decay_function_arg_t<CallbackT, 0>;
  };

  // выбор нужного vector'а из static_methods/instance_methods/etc; реализации см. внизу
  template<class MemberT>
  std::vector<MemberT> &get_all_of() {
    static_assert(sizeof(MemberT) == -1u, "Invalid template MemberT parameter");
    return {};
  }

  template<class MemberT>
  const std::vector<MemberT> &get_all_of() const { return const_cast<ClassMembersContainer *>(this)->get_all_of<MemberT>(); }

public:
  explicit ClassMembersContainer(ClassData *klass) :
    klass(ClassPtr(klass)) {}

  template<class CallbackT>
  inline void for_each(CallbackT callback) const {
    const auto &container = get_all_of<typename arg_helper<CallbackT>::MemberT>();
    std::for_each(container.cbegin(), container.cend(), std::move(callback));
  }

  template<class CallbackT>
  inline void for_each(CallbackT callback) {
    auto &container = get_all_of<typename arg_helper<CallbackT>::MemberT>();
    std::for_each(container.begin(), container.end(), std::move(callback));
  }

  template<class CallbackT>
  inline void remove_if(CallbackT callbackReturningBool) {
    auto &container = get_all_of<typename arg_helper<CallbackT>::MemberT>();
    auto end = std::remove_if(container.begin(), container.end(), std::move(callbackReturningBool));
    container.erase(end, container.end());
  }

  template<class CallbackT>
  inline const typename arg_helper<CallbackT>::MemberT *find_member(CallbackT callbackReturningBool) const {
    const auto &container = get_all_of<typename arg_helper<CallbackT>::MemberT>();
    auto it = std::find_if(container.cbegin(), container.cend(), std::move(callbackReturningBool));
    return it == container.cend() ? nullptr : &(*it);
  }

  template<class MemberT>
  inline const MemberT *find_by_local_name(vk::string_view local_name) const {
    return find_member([&local_name](const MemberT &f) { return f.local_name() == local_name; });
  }

  void add_static_method(FunctionPtr function);
  void add_instance_method(FunctionPtr function);
  void add_static_field(VertexAdaptor<op_var> root, VertexPtr def_val, FieldModifiers modifiers, vk::string_view phpdoc_str);
  void add_instance_field(VertexAdaptor<op_var> root, VertexPtr def_val, FieldModifiers modifiers, vk::string_view phpdoc_str);
  void add_constant(const std::string &const_name, VertexPtr value);

  void safe_add_instance_method(FunctionPtr function);

  bool has_constant(vk::string_view local_name) const;
  bool has_field(vk::string_view local_name) const;
  bool has_instance_method(vk::string_view local_name) const;
  bool has_static_method(vk::string_view local_name) const;

  bool has_any_instance_var() const;
  bool has_any_instance_method() const;
  FunctionPtr get_constructor() const;

  const ClassMemberStaticMethod *get_static_method(vk::string_view local_name) const;
  const ClassMemberInstanceMethod *get_instance_method(vk::string_view local_name) const;
  const ClassMemberStaticField *get_static_field(vk::string_view local_name) const;
  const ClassMemberInstanceField *get_instance_field(vk::string_view local_name) const;
  const ClassMemberConstant *get_constant(vk::string_view local_name) const;

  ClassMemberStaticMethod *get_static_method(vk::string_view local_name);
  ClassMemberInstanceMethod *get_instance_method(vk::string_view local_name);
};

/*
 * У нас методы/статические поля класса превращаются в глобальные функции/дефайны, например:
 * "Classes$A$$method", "c#Classes$A$$constname"
 * Т.е. везде паттерн такой, что после $$ идёт именно локальное для класса имя, как оно объявлено разработчиком.
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
inline std::vector<ClassMemberStaticMethod> &ClassMembersContainer::get_all_of<ClassMemberStaticMethod>() { return static_methods; }
template<>
inline std::vector<ClassMemberInstanceMethod> &ClassMembersContainer::get_all_of<ClassMemberInstanceMethod>() { return instance_methods; }
template<>
inline std::vector<ClassMemberStaticField> &ClassMembersContainer::get_all_of<ClassMemberStaticField>() { return static_fields; }
template<>
inline std::vector<ClassMemberInstanceField> &ClassMembersContainer::get_all_of<ClassMemberInstanceField>() { return instance_fields; }
template<>
inline std::vector<ClassMemberConstant> &ClassMembersContainer::get_all_of<ClassMemberConstant>() { return contants; }
