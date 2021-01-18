// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <atomic>

#include "common/algorithms/compare.h"
#include "common/algorithms/hashes.h"

#include "compiler/class-assumptions.h"
#include "compiler/data/class-members.h"
#include "compiler/data/class-modifiers.h"
#include "compiler/debug.h"
#include "compiler/location.h"
#include "compiler/threading/data-stream.h"
#include "compiler/threading/locks.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"

enum class ClassType {
  klass,
  interface,
  trait
};

class ClassData : public Lockable {
  DEBUG_STRING_METHOD { return name; }
  
public:
  // extends/implements/use trait description in a string form (class_name)
  struct StrDependence {
    ClassType type;
    std::string class_name;

    StrDependence(ClassType type, std::string class_name) :
      type(type),
      class_name(std::move(class_name)) {}
  };

private:
  std::vector<StrDependence> str_dependents;   // extends/implements/use trait during the parsing (before ptr is assigned)

public:
  int id{0};
  ClassType class_type{ClassType::klass}; // class/interface/trait
  std::string name;                       // class name with a full namespace path and slashes: "VK\Feed\A"

  ClassPtr parent_class;                       // extends
  std::vector<InterfacePtr> implements;
  std::vector<ClassPtr> derived_classes;

  FunctionPtr construct_function;
  vk::string_view phpdoc_str;

  bool can_be_php_autoloaded{false};
  bool is_immutable{false};
  bool really_used{false};
  bool is_tl_class{false};
  bool has_custom_constructor{false};
  bool is_serializable{false};

  SrcFilePtr file_id;
  int location_line_num{-1};
  std::string src_name, header_name;

  std::atomic<bool> need_instance_to_array_visitor{false};
  std::atomic<bool> need_instance_cache_visitors{false};
  std::atomic<bool> need_instance_memory_estimate_visitor{false};

  ClassModifiers modifiers;
  ClassMembersContainer members;

  const TypeData *const type_data;

  static const char *NAME_OF_VIRT_CLONE;
  static const char *NAME_OF_CLONE;
  static const char *NAME_OF_CONSTRUCT;
  static const char *NAME_OF_INVOKE_METHOD;

  ClassData();

  static VertexAdaptor<op_var> gen_vertex_this(Location location);
  FunctionPtr gen_holder_function(const std::string &name);
  FunctionPtr add_magic_method(const char *magic_name, VertexPtr return_value);
  FunctionPtr add_virt_clone();
  void add_class_constant();

  void create_constructor_with_parent_call(DataStream<FunctionPtr> &os);
  void create_default_constructor_if_required(DataStream<FunctionPtr> &os);
  void create_constructor(VertexAdaptor<op_func_param_list> param_list, VertexAdaptor<op_seq> body, vk::string_view phpdoc, DataStream<FunctionPtr> &os);
  void create_constructor(VertexAdaptor<op_function> func);

  static auto gen_param_this(Location location) {
    return VertexAdaptor<op_func_param>::create(gen_vertex_this(location));
  }

  // function fname(args) => function fname($this ::: class_instance, args)
  static void patch_func_add_this(std::vector<VertexAdaptor<op_func_param>> &params_next, Location location) {
    params_next.emplace(params_next.begin(), gen_param_this(location));
  }


  bool is_polymorphic_class() const {
    return !derived_classes.empty() || !implements.empty() || parent_class;
  }

  bool is_empty_class() const {
    return !members.has_any_instance_var() && !is_builtin() && !is_tl_class && !is_polymorphic_class();
  }

  bool is_class() const { return class_type == ClassType::klass; }
  bool is_interface() const { return class_type == ClassType::interface; }
  bool is_trait() const { return class_type == ClassType::trait; }
  virtual bool is_lambda() const { return false; }

  bool is_parent_of(ClassPtr other) const;
  std::vector<ClassPtr> get_common_base_or_interface(ClassPtr other) const;
  const ClassMemberInstanceMethod *get_instance_method(vk::string_view local_name) const;
  const ClassMemberInstanceField *get_instance_field(vk::string_view local_name) const;
  const ClassMemberStaticField *get_static_field(vk::string_view local_name) const;
  const ClassMemberConstant *get_constant(vk::string_view local_name) const;

  ClassPtr get_self() const {
    return ClassPtr{const_cast<ClassData *>(this)};
  }

  FunctionPtr get_holder_function() const;

  bool need_virtual_modifier() const {
    auto has_several_parents = [](ClassPtr c) { return (c->parent_class && !c->implements.empty()) || c->implements.size() > 1; };
    return vk::any_of(get_all_inheritors(), has_several_parents);
  }

  void set_name_and_src_name(const std::string &name, vk::string_view phpdoc_str);

  void debugPrint();

  virtual const char *get_name() const {
    return name.c_str();
  }

  int get_hash() const {
    return static_cast<int>(vk::std_hash(name));
  }

  virtual std::string get_namespace() const;

  virtual std::string get_subdir() const {
    return "cl";
  }

  const std::string *get_parent_class_name() const {
    // this method returns string name from the extends;
    // it can work before the classes are bound (i.e. parent_class is not set yet)
    for (const auto &dep : str_dependents) {
      if (dep.type == ClassType::klass) {
        return &dep.class_name;
      }
    }
    return nullptr;
  }

  std::vector<ClassPtr> get_all_inheritors() const;

  std::vector<ClassPtr> get_all_ancestors() const;

  bool is_builtin() const;
  bool is_polymorphic_or_has_polymorphic_member() const;
  static bool does_need_codegen(ClassPtr c);
  void mark_as_used();
  bool has_no_derived_classes() const;

  void deeply_require_instance_to_array_visitor();
  void deeply_require_instance_cache_visitor();
  void deeply_require_instance_memory_estimate_visitor();

  void add_str_dependent(FunctionPtr cur_function, ClassType type, vk::string_view class_name);
  const std::vector<StrDependence> &get_str_dependents() const {
    return str_dependents;
  }

  void register_defines() const;

private:
  bool has_polymorphic_member_dfs(std::unordered_set<ClassPtr> &checked) const;

  template<class MemberT>
  const MemberT *find_by_local_name(vk::string_view local_name) const {
    for (const auto &ancestor : get_all_ancestors()) {
      AutoLocker<Lockable *> locker(&(*ancestor));
      if (auto member = ancestor->members.find_by_local_name<MemberT>(local_name)) {
        return member;
      }
    }

    return {};
  }

  template<std::atomic<bool> ClassData:: *field_ptr>
  void set_atomic_field_deeply();
};

bool operator<(const ClassPtr &lhs, const ClassPtr &rhs);
