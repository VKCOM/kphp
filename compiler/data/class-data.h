// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <atomic>
#include <forward_list>

#include "common/algorithms/compare.h"
#include "common/algorithms/hashes.h"

#include "compiler/data/class-members.h"
#include "compiler/data/class-modifiers.h"
#include "compiler/debug.h"
#include "compiler/location.h"
#include "compiler/threading/data-stream.h"
#include "compiler/threading/locks.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"
#include "compiler/ffi/ffi_types.h"

struct FFIClassDataMixin;
struct FFIScopeDataMixin;
namespace kphp_json { class KphpJsonTagList; }

enum class ClassType {
  klass,
  interface,
  trait,
  ffi_scope,
  ffi_cdata,
};

class ClassData : public Lockable {
  DEBUG_STRING_METHOD { return as_human_readable(); }
  
public:
  // extends/implements/use trait description in a string form (class_name)
  struct StrDependence {
    ClassType type;
    std::string class_name;

    StrDependence(ClassType type, std::string class_name) :
      type(type),
      class_name(std::move(class_name)) {}
  };

  int id{0};
  ClassType class_type{ClassType::klass}; // class/interface/trait
  std::string name;                       // class name with a full namespace path and slashes: "VK\Feed\A"

  ClassPtr parent_class;                       // extends
  std::vector<InterfacePtr> implements;
  std::vector<ClassPtr> derived_classes;
  FunctionPtr construct_function;

  const PhpDocComment *phpdoc{nullptr};
  const kphp_json::KphpJsonTagList *kphp_json_tags{nullptr};

  bool can_be_php_autoloaded{false};
  bool is_immutable{false};
  bool really_used{false};
  bool is_tl_class{false};
  bool has_custom_constructor{false};
  bool is_serializable{false};
  bool has_job_shared_memory_piece{false};

  SrcFilePtr file_id;
  ModulitePtr modulite;
  int location_line_num{-1};
  std::string src_name;
  std::string cpp_filename;
  std::string h_filename;

  std::atomic<bool> need_to_array_debug_visitor{false};
  std::atomic<bool> need_instance_cache_visitors{false};
  std::atomic<bool> need_instance_memory_estimate_visitor{false};
  std::atomic<bool> need_virtual_builtin_functions{false};
  // need_json_visitors doesn't exist: instead, we use json_encoders with a list of classes

  ClassModifiers modifiers;
  ClassMembersContainer members;

  const TypeHint *type_hint{nullptr};
  const TypeData *const type_data;

  FFIClassDataMixin *ffi_class_mixin = nullptr; // non-null for ffi_cdata classes
  FFIScopeDataMixin *ffi_scope_mixin = nullptr; // non-null for ffi_scope classes

  // if JsonEncoder::encode() and MyJsonEncoder::decode() were called for this class,
  // this field will contain [ {JsonEncoder,true}, {MyJsonEncoder,false} ]
  std::forward_list<std::pair<ClassPtr, bool>> json_encoders;

  static const char *NAME_OF_VIRT_CLONE;
  static const char *NAME_OF_CLONE;
  static const char *NAME_OF_CONSTRUCT;
  static const char *NAME_OF_TO_STRING;
  static const char *NAME_OF_WAKEUP;

  explicit ClassData(ClassType type);
  std::string as_human_readable() const;

  static VertexAdaptor<op_var> gen_vertex_this(Location location);
  FunctionPtr gen_holder_function(const std::string &name);
  FunctionPtr add_virt_clone();
  void add_class_constant();

  void create_constructor_with_parent_call(DataStream<FunctionPtr> &os);
  void create_default_constructor_if_required(DataStream<FunctionPtr> &os);
  void create_constructor(VertexAdaptor<op_func_param_list> param_list, VertexAdaptor<op_seq> body, const PhpDocComment *phpdoc, DataStream<FunctionPtr> &os);
  void create_constructor(VertexAdaptor<op_function> func);

  static auto gen_param_this(Location location) {
    return VertexAdaptor<op_func_param>::create(gen_vertex_this(location));
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
  bool is_ffi_scope() const { return class_type == ClassType::ffi_scope; }
  bool is_ffi_cdata() const { return class_type == ClassType::ffi_cdata; }
  bool is_lambda_class() const { return class_type == ClassType::klass && is_lambda; }
  bool is_typed_callable_interface() const { return class_type == ClassType::interface && is_lambda; }

  bool is_parent_of(ClassPtr other) const;
  std::vector<ClassPtr> get_common_base_or_interface(ClassPtr other) const;

  const ClassMemberInstanceMethod *get_instance_method(vk::string_view local_name) const {
    return find_instance_method_by_local_name(local_name);
  }
  const ClassMemberInstanceField *get_instance_field(vk::string_view local_name) const {
    return find_by_local_name<ClassMemberInstanceField>(local_name);
  }
  const ClassMemberStaticField *get_static_field(vk::string_view local_name) const {
    return find_by_local_name<ClassMemberStaticField>(local_name);
  }
  const ClassMemberConstant *get_constant(vk::string_view local_name) const {
    return find_by_local_name<ClassMemberConstant>(local_name);
  }

  ClassPtr get_self() const {
    return ClassPtr{const_cast<ClassData *>(this)};
  }

  FunctionPtr get_holder_function() const;

  bool need_virtual_modifier() const {
    auto has_several_parents = [](ClassPtr c) { return (c->parent_class && !c->implements.empty()) || c->implements.size() > 1; };
    return vk::any_of(get_all_derived_classes(), has_several_parents);
  }

  void set_name_and_src_name(const std::string &full_name);

  void debugPrint();

  int get_hash() const {
    return static_cast<int>(vk::std_hash(name));
  }

  std::string get_subdir() const {
    return is_lambda ? "cl_l" : "cl";
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

  std::vector<ClassPtr> get_all_derived_classes() const;
  std::vector<ClassPtr> get_all_ancestors() const;

  bool is_builtin() const;
  bool is_polymorphic_or_has_polymorphic_member() const;
  bool does_need_codegen() const;
  void mark_as_used();

  void deeply_require_to_array_debug_visitor();
  void deeply_require_instance_cache_visitor();
  void deeply_require_instance_memory_estimate_visitor();
  void deeply_require_virtual_builtin_functions();

  void add_str_dependent(FunctionPtr cur_function, ClassType type, vk::string_view class_name);
  const std::vector<StrDependence> &get_str_dependents() const {
    return str_dependents;
  }

  void register_defines() const;

  std::vector<const ClassMemberInstanceField *> get_job_shared_memory_pieces() const;

private:
  bool has_polymorphic_member_dfs(std::unordered_set<ClassPtr> &checked) const;

  const ClassMemberInstanceMethod *find_instance_method_by_local_name(vk::string_view local_name) const;

  template<class MemberT>
  const MemberT *find_by_local_name(vk::string_view local_name) const {
    // we don't lock the class here: it's assumed, that members are not inserted concurrently
    if (const auto *member = members.find_by_local_name<MemberT>(local_name)) {
      return member;
    }
    for (ClassPtr ancestor : get_all_ancestors()) {
      if (const auto *member = ancestor->members.find_by_local_name<MemberT>(local_name)) {
        return member;
      }
    }

    return {};
  }

  template<std::atomic<bool> ClassData:: *field_ptr>
  void set_atomic_field_deeply();

  // extends/implements/use trait during the parsing (before ptr is assigned)
  std::vector<StrDependence> str_dependents;

  // true for lambda classes and typed callable interfaces, based on name starts_with
  bool is_lambda{false};
};

bool operator<(const ClassPtr &lhs, const ClassPtr &rhs);
