/*
 *  У классов есть статические / инстанс поля, методы; есть константы.
 * Тип этого — enum MemberKind. Реализация этого — наследники ClassMemberBase.
 *  У ClassData есть поле members, которое предоставляет compile-time reflection, например:
 * пробежаться по всем статическим полям, проверить существование константы, найти метод по условию и т.п.
 *  Методы, статические поля и константы превращаются в глобальные функции / переменные / дефайны, по именованию
 * Namespace$Class$Name$$local_name. Терминология `local_name` — это название внутри класса, `global_name` — вот такое.
 *  Есть ещё контекстные функции для статического наследования и реализации self/parent/static, их именование вида
 * Namespace$ClassName$$local_name$$Namespace$ClassContextName.
 */
#pragma once

#include "compiler/data_ptr.h"
#include "common/type_traits/function_traits.h"


enum AccessType {
  access_nonmember = 0,
  access_static_public,
  access_static_private,
  access_static_protected,
  access_public,
  access_private,
  access_protected,
};

enum MemberKind {
  mem_static_method,
  mem_instance_method,
  mem_static_field,
  mem_instance_field,
  mem_constant,
};

enum ClassType {
  ctype_class,
  ctype_interface,
  ctype_trait
};


class ClassMemberBase {
protected:
  explicit ClassMemberBase(ClassPtr klass __attribute__ ((unused)), MemberKind member_kind) :
    member_kind(member_kind) {}

public:
  MemberKind member_kind;

  virtual ~ClassMemberBase() = default;
};

struct ClassMemberStaticMethod : public ClassMemberBase {
  static const MemberKind TRAITS_MEM_KIND = mem_static_method;

  AccessType access_type;
  FunctionPtr function;

  ClassMemberStaticMethod(ClassPtr klass, FunctionPtr function, AccessType access_type) :
    ClassMemberBase(klass, TRAITS_MEM_KIND),
    access_type(access_type),
    function(function) {}

  const string &global_name() const;
  const string local_name() const;
};

struct ClassMemberInstanceMethod : public ClassMemberBase {
  static const MemberKind TRAITS_MEM_KIND = mem_instance_method;

  AccessType access_type;
  FunctionPtr function;

  ClassMemberInstanceMethod(ClassPtr klass, FunctionPtr function, AccessType access_type) :
    ClassMemberBase(klass, TRAITS_MEM_KIND),
    access_type(access_type),
    function(function) {}

  const string &global_name() const;
  const string local_name() const;
};

struct ClassMemberStaticField : public ClassMemberBase {
  static const MemberKind TRAITS_MEM_KIND = mem_static_field;

  // тут мне не нравится, но пока что для совместимости с ClassInfo; потом name можно удалить, реализовать global_name/local_name
  AccessType access_type;
  VertexAdaptor<op_static> root;
  string full_name;

  ClassMemberStaticField(ClassPtr klass, VertexAdaptor<op_static> root, AccessType access_type, string full_name) :
    ClassMemberBase(klass, TRAITS_MEM_KIND),
    access_type(access_type),
    root(root),
    full_name(std::move(full_name)) {}

  const string &global_name() const;
  const string local_name() const;
};

struct ClassMemberInstanceField : public ClassMemberBase {
  static const MemberKind TRAITS_MEM_KIND = mem_instance_field;

  AccessType access_type;
  VertexAdaptor<op_class_var> root;
  VarPtr var;
  Token *phpdoc_token;

  ClassMemberInstanceField(ClassPtr klass, VertexAdaptor<op_class_var> root, AccessType access_type);

  const string &local_name() const;
};

struct ClassMemberConstant : public ClassMemberBase {
  static const MemberKind TRAITS_MEM_KIND = mem_constant;

  VertexAdaptor<op_define> root;

  ClassMemberConstant(ClassPtr klass, VertexAdaptor<op_define> root) :
    ClassMemberBase(klass, TRAITS_MEM_KIND),
    root(root) {}

  const string &global_name() const;
  const string local_name() const;
};


/*
   —————————————————————————————————————

 *  У каждого php-класса (ClassData) есть members — объект ClassMembersContainer.
 *  Собственно, он хранит все члены php-класса и предоставляет осмысленные методы доступа.
 *  Он хранит вектор ClassMemberBase — т.к. нужен порядок сохранения и кодогенерации.
 * И параллельно с ним — set хешей, для быстрой проверки существования по имени (особенно актуально для констант).
 *  Несмотря на то, что хранится ClassMemberBase — функции for_each и find_member имеют пробегаться
 * по конкретным типам member'ов, а не Base: тип определяется как первый аргумент callback'а, см. usages.
 */
class ClassMembersContainer {
  ClassPtr klass;

  vector<ClassMemberBase *> all_members;
  set<unsigned long long> names_hashes;

  void append_member(const string &hash_name, ClassMemberBase *member);
  bool member_exists(const string &hash_name) const;

  template<class CallbackT>
  struct arg_helper {
    using MemberT = typename std::remove_pointer<
      typename vk::function_traits<CallbackT>::template Argument<0>
    >::type;
  };

public:
  explicit ClassMembersContainer(ClassData *klass) :
    klass(ClassPtr(klass)) {}

  template<class CallbackT>
  inline void for_each(CallbackT callback) const {
    using MemberT = typename arg_helper<CallbackT>::MemberT;

    for (auto &it : all_members) {    // TRAITS_MEM_KIND — чтоб не делать dynamic_cast != null всему подряд
      if (it->member_kind == MemberT::TRAITS_MEM_KIND) {
        callback(dynamic_cast<MemberT *>(it));
      }
    }
  }

  template<class CallbackT>
  inline const typename arg_helper<CallbackT>::MemberT *find_member(CallbackT callbackReturningBool) const {
    using MemberT = typename arg_helper<CallbackT>::MemberT;

    for (auto &it : all_members) {    // TRAITS_MEM_KIND — чтоб не делать dynamic_cast != null всему подряд
      if (it->member_kind == MemberT::TRAITS_MEM_KIND && callbackReturningBool(dynamic_cast<MemberT *>(it))) {
        return dynamic_cast<MemberT *>(it);
      }
    }

    return nullptr;
  }

  template<class MemberT>
  inline const MemberT* find_by_local_name(const string &local_name) const {
    return find_member([&](MemberT *f) { return f->local_name() == local_name; });
  }

  void add_static_method(FunctionPtr function, AccessType access_type);
  void add_instance_method(FunctionPtr function, AccessType access_type);
  void add_constructor(FunctionPtr function, AccessType access_type);
  void add_static_field(VertexAdaptor<op_static> root, const string &name, AccessType access_type);
  void add_instance_field(VertexAdaptor<op_class_var> root, AccessType access_type);
  void add_constant(VertexAdaptor<op_define> root);

  bool has_constant(const string &local_name) const;
  bool has_field(const string &local_name) const;
  bool has_instance_method(const string &local_name) const;

  bool has_any_instance_var() const;
  bool has_any_instance_method() const;
  bool has_constructor() const;

  const ClassMemberStaticMethod *get_static_method(const string &local_name) const;
  const ClassMemberInstanceMethod *get_instance_method(const string &local_name) const;
  const ClassMemberStaticField *get_static_field(const string &local_name) const;
  const ClassMemberInstanceField *get_instance_field(const string &local_name) const;
  const ClassMemberConstant *get_constant(const string &local_name) const;

};

/*
 * У нас методы/статические поля класса превращаются в глобальные функции/дефайны, например:
 * "Classes$A$$method", "c#Classes$A$$constname"
 * Т.е. везде паттерн такой, что после $$ идёт именно локальное для класса имя, как оно объявлено разработчиком.
 */
inline string get_local_name_from_global_$$(const string &global_name) {
  string::size_type pos$$ = global_name.find("$$");
  return pos$$ == string::npos ? global_name : global_name.substr(pos$$ + 2);
}

