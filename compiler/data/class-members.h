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

#include "compiler/data/function-modifiers.h"
#include "compiler/data/field-modifiers.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"
#include "common/type_traits/function_traits.h"

class TypeData;

struct ClassMemberStaticMethod {
  FunctionModifiers modifiers;
  FunctionPtr function;

  ClassMemberStaticMethod(FunctionPtr function, FunctionModifiers modifiers) :
    modifiers(modifiers),
    function(function) {}

  const std::string &global_name() const;
  std::string local_name() const;
};

struct ClassMemberInstanceMethod {
  FunctionModifiers modifiers;
  FunctionPtr function;

  ClassMemberInstanceMethod(FunctionPtr function, FunctionModifiers modifiers) :
    modifiers(modifiers),
    function(function) {}

  const std::string &global_name() const;
  std::string local_name() const;
};

struct ClassMemberStaticField {
  FieldModifiers modifiers;
  std::string full_name;
  VertexAdaptor<op_var> root;
  VertexPtr init_val;          // op_empty в случае отсутствия значения, или какое-то выражение
  vk::string_view phpdoc_str;

  ClassMemberStaticField(ClassPtr klass, VertexAdaptor<op_var> root, VertexPtr init_val, FieldModifiers modifiers, const vk::string_view &phpdoc_str);

  const std::string &global_name() const;
  const std::string &local_name() const;
  const TypeData *get_inferred_type() const;
};

struct ClassMemberInstanceField {
  FieldModifiers modifiers;
  VertexAdaptor<op_var> root;
  VarPtr var;
  vk::string_view phpdoc_str;

  ClassMemberInstanceField(ClassPtr klass, VertexAdaptor<op_var> root, VertexPtr def_val, FieldModifiers modifiers, const vk::string_view &phpdoc_str);

  const std::string &local_name() const;
  const TypeData *get_inferred_type() const;
  void process_phpdoc();
};

struct ClassMemberConstant {
  std::string define_name;
  VertexPtr value;

  ClassMemberConstant(ClassPtr klass, const std::string &const_name, VertexPtr value);

  const std::string &global_name() const;
  std::string local_name() const;
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
  void append_member(const std::string &hash_name, const MemberT &member);
  bool member_exists(const std::string &hash_name) const;

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
  inline const MemberT *find_by_local_name(const std::string &local_name) const {
    return find_member([&local_name](const MemberT &f) { return f.local_name() == local_name; });
  }

  void add_static_method(FunctionPtr function, FunctionModifiers modifiers);
  void add_instance_method(FunctionPtr function, FunctionModifiers modifiers);
  void add_static_field(VertexAdaptor<op_var> root, VertexPtr init_val, FieldModifiers modifiers, const vk::string_view &phpdoc_str);
  void add_instance_field(VertexAdaptor<op_var> root, VertexPtr def_val, FieldModifiers modifiers, const vk::string_view &phpdoc_str);
  void add_constant(std::string const_name, VertexPtr value);

  void safe_add_instance_method(FunctionPtr function, FunctionModifiers modifiers);

  bool has_constant(const std::string &local_name) const;
  bool has_field(const std::string &local_name) const;
  bool has_instance_method(const std::string &local_name) const;
  bool has_static_method(const std::string &local_name) const;

  bool has_any_instance_var() const;
  bool has_any_instance_method() const;
  FunctionPtr get_constructor() const;

  const ClassMemberStaticMethod *get_static_method(const std::string &local_name) const;
  const ClassMemberInstanceMethod *get_instance_method(const std::string &local_name) const;
  const ClassMemberStaticField *get_static_field(const std::string &local_name) const;
  const ClassMemberInstanceField *get_instance_field(const std::string &local_name) const;
  const ClassMemberConstant *get_constant(const std::string &local_name) const;
};

/*
 * У нас методы/статические поля класса превращаются в глобальные функции/дефайны, например:
 * "Classes$A$$method", "c#Classes$A$$constname"
 * Т.е. везде паттерн такой, что после $$ идёт именно локальное для класса имя, как оно объявлено разработчиком.
 */
inline std::string get_local_name_from_global_$$(const std::string &global_name) {
  auto pos$$ = global_name.find("$$");
  return pos$$ == std::string::npos ? global_name : global_name.substr(pos$$ + 2);
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
