#pragma once

#include "compiler/class-assumptions.h"
#include "compiler/data/class-members.h"
#include "compiler/threading/locks.h"

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

  std::string get_name_of_invoke_function_for_extern(VertexAdaptor<op_func_call> extern_function_call,
                                                     FunctionPtr function_context,
                                                     std::map<int, std::pair<AssumType, ClassPtr>> *template_type_id_to_ClassPtr = nullptr,
                                                     FunctionPtr *template_of_invoke_method = nullptr) const;
  FunctionPtr get_template_of_invoke_function() const;

  bool is_lambda_class() const;
  void infer_uses_assumptions(FunctionPtr parent_function);

  void set_name_and_src_name(const string &name);

  void debugPrint();

  const std::string &get_subdir() const {
    static std::string lambda_subdir("cl_l");
    static std::string common_subdir("cl");

    return is_lambda_class() ? lambda_subdir : common_subdir;
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
