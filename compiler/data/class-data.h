#pragma once

#include "compiler/class-assumptions.h"
#include "compiler/data/class-members.h"
#include "compiler/location.h"
#include "compiler/threading/data-stream.h"
#include "compiler/threading/locks.h"

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

  int id{0};
  ClassType class_type{ClassType::klass}; // класс / интерфейс / трейт
  std::string name;                            // название класса с полным namespace и слешами: "VK\Feed\A"
  VertexAdaptor<op_class> root;

  std::vector<StrDependence> str_dependents;   // extends / implements / use trait на время парсинга, до связки ptr'ов
  ClassPtr parent_class;                  // extends
  std::vector<InterfacePtr> implements;        // на будущее
  std::vector<ClassPtr> derived_classes;
  std::vector<ClassPtr> traits_uses;           // на будущее

  FunctionPtr construct_function;
  vk::string_view phpdoc_str;

  std::vector<Assumption> assumptions_for_vars;
  int assumptions_inited_vars{0};
  bool can_be_php_autoloaded{false};
  bool is_immutable{false};

  SrcFilePtr file_id;
  std::string src_name, header_name;

  ClassMembersContainer members;

  const TypeData *const type_data;

  static const char * NAME_OF_VIRT_CLONE;

  ClassData();

  static VertexAdaptor<op_var> gen_vertex_this(int location_line_num);
  VertexAdaptor<op_var> gen_vertex_this_with_type_rule(int location_line_num);
  FunctionPtr gen_holder_function(const std::string &name);
  FunctionPtr add_virt_clone(DataStream<FunctionPtr> &os, bool with_body = true);

  // __construct(args) { body } => __construct(args) { $this ::: tp_Class; def vars init; body; return $this; }
  void patch_func_constructor(VertexAdaptor<op_function> func);

  void create_default_constructor(int location_line_num, DataStream<FunctionPtr> &os);
  void create_constructor_with_args(int location_line_num, VertexAdaptor<op_func_param_list> params);
  void create_constructor_with_args(int location_line_num, VertexAdaptor<op_func_param_list> params, DataStream<FunctionPtr> &os, bool auto_required = true);

  // function fname(args) => function fname($this ::: class_instance, args)
  template<Operation Op>
  void patch_func_add_this(std::vector<VertexAdaptor<Op>> &params_next, int location_line_num);

  bool is_not_empty_class() const {
    return members.has_any_instance_var() || is_builtin() || is_interface() || !implements.empty();
  }

  bool is_class() const { return class_type == ClassType::klass; }
  bool is_interface() const { return class_type == ClassType::interface; }
  bool is_trait() const { return class_type == ClassType::trait; }
  virtual bool is_lambda() const { return false; }

  bool is_parent_of(ClassPtr other);
  InterfacePtr get_common_interface(ClassPtr other) const;
  ClassPtr get_self() const {
    return ClassPtr{const_cast<ClassData *>(this)};
  }

  virtual bool is_fully_static() const {
    return is_class() && !construct_function;
  }

  void set_name_and_src_name(const std::string &name);

  void debugPrint();

  virtual const char *get_name() const {
    return name.c_str();
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

  bool need_generate_accept_method() const;
  bool is_builtin() const;
  bool is_interface_or_has_interface_member();
  static bool does_need_codegen(ClassPtr c);

private:
  bool has_interface_member_dfs(std::unordered_set<ClassPtr> &checked);
};

bool operator<(const ClassPtr &lhs, const ClassPtr &rhs);
