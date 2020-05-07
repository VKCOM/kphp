#pragma once
#include <atomic>

#include "compiler/class-assumptions.h"
#include "compiler/data/class-members.h"
#include "compiler/data/class-modifiers.h"
#include "compiler/location.h"
#include "compiler/threading/data-stream.h"
#include "compiler/threading/locks.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"
#include "common/algorithms/hashes.h"

enum class ClassType {
  klass,
  interface,
  trait
};

class ClassData : public Lockable {
public:
  // описание extends / implements / use trait в строковом виде (class_name)
  struct StrDependence {
    ClassType type;
    std::string class_name;

    StrDependence(ClassType type, std::string class_name) :
      type(type),
      class_name(std::move(class_name)) {}
  };

private:
  std::vector<StrDependence> str_dependents;   // extends / implements / use trait на время парсинга, до связки ptr'ов

public:
  int id{0};
  ClassType class_type{ClassType::klass}; // класс / интерфейс / трейт
  std::string name;                            // название класса с полным namespace и слешами: "VK\Feed\A"

  ClassPtr parent_class;                       // extends
  std::vector<InterfacePtr> implements;
  std::vector<ClassPtr> derived_classes;

  FunctionPtr construct_function;
  vk::string_view phpdoc_str;

  std::vector<std::pair<std::string, vk::intrusive_ptr<Assumption>>> assumptions_for_vars;   // (var_name, assumption)[]
  std::atomic<AssumptionStatus> assumption_vars_status{AssumptionStatus::unknown};
  bool can_be_php_autoloaded{false};
  bool is_immutable{false};
  bool really_used{false};
  bool is_tl_class{false};
  bool has_custom_constructor{false};
  bool is_serializable{false};
  // flag forces to have php-docs above functions; we should add the same check for class fields
  bool has_kphp_infer{false};

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

  void create_default_constructor(Location location, DataStream<FunctionPtr> &os);
  void create_constructor(VertexAdaptor<op_function> func);

  static auto gen_param_this(Location location) {
    return VertexAdaptor<op_func_param>::create(gen_vertex_this(location));
  }

  // function fname(args) => function fname($this ::: class_instance, args)
  template<Operation Op>
  static void patch_func_add_this(std::vector<VertexAdaptor<Op>> &params_next, Location location) {
    static_assert(vk::any_of_equal(Op, meta_op_base, meta_op_func_param, op_func_param), "disallowed vector of Operation");
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

  ClassPtr get_parent_or_interface() const;
  bool is_parent_of(ClassPtr other) const;
  ClassPtr get_common_base_or_interface(ClassPtr other) const;
  const ClassMemberInstanceMethod *get_instance_method(vk::string_view local_name) const;
  const ClassMemberInstanceField *get_instance_field(vk::string_view local_name) const;
  const ClassMemberStaticField *get_static_field(vk::string_view local_name) const;
  const ClassMemberConstant *get_constant(vk::string_view local_name) const;
  void check_parent_constructor();

  ClassPtr get_self() const {
    return ClassPtr{const_cast<ClassData *>(this)};
  }

  virtual bool is_fully_static() const {
    return is_class() && !modifiers.is_abstract() && !construct_function;
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
    for (const auto &dep : str_dependents) {    // именно когда нужно строковое имя extends,
      if (dep.type == ClassType::klass) {       // до связки классов, т.е. parent_class ещё не определёнs
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
