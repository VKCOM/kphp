#pragma once

#include "compiler/class-assumptions.h"
#include "compiler/data/class-members.h"
#include "compiler/location.h"
#include "compiler/threading/data-stream.h"
#include "compiler/threading/locks.h"
#include "compiler/threading/data-stream.h"

enum ClassType {
  ctype_class,
  ctype_interface,
  ctype_trait
};

class ClassData : public Lockable {
public:
  // описание extends / implements / use trait в строковом виде (class_name)
  struct StrDependence {
    ClassType type;
    string class_name;

    StrDependence(ClassType type, string class_name) :
      type(type),
      class_name(std::move(class_name)) {}
  };

  int id;
  ClassType class_type;       // класс / интерфейс / трейт
  string name;                // название класса с полным namespace и слешами: "VK\Feed\A"
  VertexAdaptor<op_class> root;

  vector<StrDependence> str_dependents; // extends / implements / use trait на время парсинга, до связки ptr'ов
  ClassPtr parent_class;                // extends
  vector<ClassPtr> implements;          // на будущее
  vector<ClassPtr> traits_uses;         // на будущее

  FunctionPtr construct_function;
  Token *phpdoc_token;

  std::vector<Assumption> assumptions_for_vars;
  int assumptions_inited_vars;
  bool was_constructor_invoked;
  bool can_be_php_autoloaded;

  SrcFilePtr file_id;
  string src_name, header_name;

  ClassMembersContainer members;

  ClassData();

  static VertexAdaptor<op_var> gen_vertex_this(int location_line_num) ;
  VertexAdaptor<op_var> gen_vertex_this_with_type_rule(int location_line_num);

  // __construct(args) { body } => __construct(args) { $this ::: tp_Class; def vars init; body; return $this; }
  void patch_func_constructor(VertexAdaptor<op_function> func, int location_line_num);

  void create_default_constructor(int location_line_num, DataStream<FunctionPtr> &os);
  void create_constructor_with_args(int location_line_num, VertexAdaptor<op_func_param_list> params);
  void create_constructor_with_args(int location_line_num, VertexAdaptor<op_func_param_list> params, DataStream<FunctionPtr> &os, bool auto_required = true);

  // function fname(args) => function fname($this ::: class_instance, args)
  void patch_func_add_this(vector<VertexPtr> &params_next, int location_line_num);

  virtual bool is_lambda_class() const {
    return false;
  }

  void set_name_and_src_name(const string &name);

  void debugPrint();

  virtual std::string get_namespace() const;

  virtual std::string get_subdir() const {
    return "cl";
  }

  const std::string *get_parent_class_name() const {
    for (const auto &dep : str_dependents) {    // именно когда нужно строковое имя extends,
      if (dep.type == ctype_class) {            // до связки классов, т.е. parent_class ещё не определёнs
        return &dep.class_name;
      }
    }
    return nullptr;
  }
};
