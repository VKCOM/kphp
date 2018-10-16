#pragma once

#include "common/wrappers/string_view.h"

#include "compiler/bicycle.h"
#include "compiler/class-assumptions.h"
#include "compiler/data_ptr.h"
#include "compiler/type-inferer-core.h"
#include "compiler/utils.h"
#include "compiler/vertex.h"
#include "compiler/class-members.h"


class VarData {
public:
  enum Type {
    var_unknown_t = 0,
    var_local_t,
    var_local_inplace_t,
    var_global_t,
    var_param_t,
    var_const_t,
    var_static_t,
    var_instance_t
  };

  Type type_;
  int id;
  int param_i;
  string name;
  tinf::VarNode tinf_node;
  VertexPtr init_val;
  FunctionPtr holder_func;
  FunctionPtr static_id; // id of function if variable is static
  ClassPtr class_id; // id of class if variable is static fields
  vector<VarPtr> *bad_vars;
  bool is_constant;
  bool is_reference;
  bool uninited_flag;
  bool optimize_flag;
  bool tinf_flag;
  bool global_init_flag;
  bool needs_const_iterator_flag;
  bool marked_as_global;
  int dependency_level;

  void set_uninited_flag(bool f);
  bool get_uninited_flag();

  explicit VarData(Type type = var_unknown_t);

  inline Type &type() { return type_; }

  string get_human_readable_name() const;

  inline bool is_global_var() const {
    return type_ == var_global_t && !class_id;
  }

  inline bool is_function_static_var() const {
    return type_ == var_static_t;
  }

  inline bool is_class_static_var() const {
    return type_ == var_global_t && class_id;
  }

  inline bool is_class_instance_var() const {
    return type_ == var_instance_t;
  }

  const ClassMemberStaticField *as_class_static_field() const;
  const ClassMemberInstanceField *as_class_instance_field() const;
};

// todo мне не нравится, что это теперь Lockable, подумать, можно ли переделать (см. set_func_id())
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
  VertexPtr root;             // op_class

  vector<StrDependence> str_dependents; // extends / implements / use trait на время парсинга, до связки ptr'ов
  ClassPtr parent_class;                // extends
  vector<ClassPtr> implements;          // на будущее
  vector<ClassPtr> traits_uses;         // на будущее

  FunctionPtr init_function;
  FunctionPtr new_function;
  Token *phpdoc_token;

  std::vector<Assumption> assumptions;
  int assumptions_inited_vars;
  bool was_constructor_invoked;

  SrcFilePtr file_id;
  string src_name, header_name;

  ClassMembersContainer members;

  ClassData();

  std::string get_name_of_invoke_function_for_extern(FunctionPtr extern_function) const;
  FunctionPtr get_invoke_function_for_extern_function(FunctionPtr extern_function) const;
  FunctionPtr get_template_of_invoke_function() const;

  void set_name_and_src_name(const string &name);

  void debugPrint() const;
};


class FunctionInfo {
public:
  VertexPtr root;
  string namespace_name;
  string class_context;
  bool kphp_required;
  AccessType access_type;

  FunctionInfo() :
    kphp_required(false),
    access_type(access_nonmember) {}

  FunctionInfo(VertexPtr root, string namespace_name, string class_context,
               bool kphp_required, AccessType access_type) :
    root(root),
    namespace_name(std::move(namespace_name)),
    class_context(std::move(class_context)),
    kphp_required(kphp_required),
    access_type(access_type) {}
};

class FunctionData {
public:
  int id;

  string name;        // полное имя функции, в случае принадлежности классу это VK$Namespace$funcname
  VertexPtr root;     // op_function
  VertexPtr header;   // это только для костыля extern_function, потом должно уйти
  bool is_required;

  enum func_type_t {
    func_global,
    func_local,
    func_switch,
    func_extern
  };
  func_type_t type_;

  vector<VarPtr> local_var_ids, global_var_ids, static_var_ids, header_global_var_ids;
  vector<VarPtr> tmp_vars;
  vector<VarPtr> *bad_vars;
  vector<DefinePtr> define_ids;
  set<VarPtr> const_var_ids, header_const_var_ids;
  vector<VarPtr> param_ids;
  vector<FunctionPtr> dep;

  std::vector<Assumption> assumptions;
  int assumptions_inited_args;
  int assumptions_inited_return;

  string src_name, header_name;
  string subdir;
  string header_full_name;
  SrcFilePtr file_id;
  FunctionPtr fork_prev, wait_prev;
  ClassPtr class_id;
  bool varg_flag;

  int tinf_state;
  vector<tinf::VarNode> tinf_nodes;

  VertexPtr const_data;
  Token *phpdoc_token;

  int min_argn;
  bool is_extern;
  bool used_in_source;    // это только для костыля extern_function, потом должно уйти
  bool is_callback;
  bool should_be_sync;
  bool kphp_required;
  bool is_template;
  string namespace_name;
  string class_context_name;
  AccessType access_type;
  set<string> disabled_warnings;
  map<long long, int> name_gen_map;

  FunctionData();
  explicit FunctionData(VertexPtr root);
  static FunctionPtr create_function(const FunctionInfo &info);

  inline func_type_t &type() { return type_; }

  bool is_static_init_empty_body() const;
  string get_resumable_path() const;
  string get_human_readable_name() const;

  inline bool is_instance_function() const {
    return access_type == access_public || access_type == access_protected || access_type == access_private;
  }

  inline bool is_static_function() const {
    return access_type == access_static_public || access_type == access_static_protected || access_type == access_static_private;
  }

  inline bool is_constructor() const {
    return class_id && class_id->new_function && &*(class_id->new_function) == this;
  }

  static FunctionPtr generate_instance_of_template_function(const std::map<int, std::pair<AssumType, ClassPtr>> &template_type_id_to_ClassPtr,
                                                            FunctionPtr func,
                                                            const std::string &name_of_function_instance);

  static ClassPtr is_lambda(VertexPtr v);

  static const std::string &get_lambda_namespace() {
    static std::string lambda_namespace("LAMBDA$NAMESPACE");
    return lambda_namespace;
  }

  bool is_lambda() const {
    return !!function_in_which_lambda_was_created;
  }

  const std::string get_outer_namespace_name() const {
    return get_or_default_field(&FunctionData::namespace_name);
  }

  ClassPtr get_outer_class() const {
    return is_lambda() ? function_in_which_lambda_was_created->class_id : class_id;
  }

  const std::string &get_outer_class_context_name() const {
    return get_or_default_field(&FunctionData::class_context_name);
  }

  void set_function_in_which_lambda_was_created(FunctionPtr f) {
    function_in_which_lambda_was_created = f;
  }

private:
  const std::string &get_or_default_field(std::string (FunctionData::*field)) const {
    if (is_lambda()) {
      kphp_assert(function_in_which_lambda_was_created);
      return function_in_which_lambda_was_created->get_or_default_field(field);
    }

    return this->*field;
  }

  DISALLOW_COPY_AND_ASSIGN (FunctionData);

private:
  FunctionPtr function_in_which_lambda_was_created;
};

inline bool operator<(FunctionPtr a, FunctionPtr b) {
  return a->name < b->name;
}

inline bool operator<(const VarPtr &a, const VarPtr &b) {
  int cmp_res = a->name.compare(b->name);

  if (cmp_res == 0) {
    {
      bool af{a->static_id};
      bool bf{b->static_id};
      if (af || bf) {
        if (af && bf) {
          return a->static_id < b->static_id;
        } else {
          return af < bf;
        }
      }
    }
    {
      bool af{a->holder_func};
      bool bf{b->holder_func};
      if (af || bf) {
        if (af && bf) {
          return a->holder_func < b->holder_func;
        } else {
          return af < bf;
        }
      }
    }
    return false;
  }

  if (a->dependency_level != b->dependency_level) {
    return a->dependency_level < b->dependency_level;
  }

  return cmp_res < 0;
}

class DefineData {
public:
  int id;

  VertexPtr val;
  string name;
  SrcFilePtr file_id;
  enum DefineType {
    def_php,
    def_raw,
    def_var
  };

  DefineType type_;

  DefineData();
  DefineData(VertexPtr val, DefineType type_);

  inline DefineType &type() { return type_; }

private:
  DISALLOW_COPY_AND_ASSIGN (DefineData);
};

