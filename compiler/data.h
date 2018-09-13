#pragma once

#include "compiler/bicycle.h"
#include "compiler/data_ptr.h"
#include "compiler/type-inferer-core.h"
#include "compiler/utils.h"
#include "compiler/vertex.h"
#include "compiler/class-assumptions.h"

class Assumption;

enum AccessType {
  access_nonmember = 0,
  access_static_public,
  access_static_private,
  access_static_protected,   // static   functions/vars of a class
  access_public,
  access_private,
  access_protected,                        // instance functions/vars of a class
};

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
  AccessType access_type;
  int dependency_level;
  Token *phpdoc_token;

  void set_uninited_flag(bool f);
  bool get_uninited_flag();

  VarData(Type type = var_unknown_t);

  inline Type &type() { return type_; }
};

struct ClassInfo {
  string name;
  string namespace_name;
  VertexPtr root;
  string extends;
  vector<VertexPtr> members;
  map<string, VertexPtr> constants;
  // todo разобраться со static_members и static_methods и static_fields, members и будущим methods
  // (а ещё лучше — сначала таки вкостылить methods, потом написать тесты, а потом рефакторить)
  vector<VertexPtr> static_members;
  vector<VertexPtr> this_type_rules;
  vector<VertexPtr> vars;    // vector of op_class_var  todo потом избавиться от vars, их можно вычислить из members
  set<string> static_fields;
  map<string, FunctionPtr> static_methods;
  vector<VertexPtr> methods;
  FunctionPtr new_function;
  Token *phpdoc_token;

  inline bool has_instance_vars() const {
    return !vars.empty();
  }

  inline bool has_instance_methods() const {
    return !methods.empty();
  }
};

class ClassData {
public:
  int id;
  string name;
  string extends;
  ClassPtr parent_class;
  VertexPtr root;

  FunctionPtr init_function;
  FunctionPtr new_function;
  set<string> static_fields;
  set<string> constants;
  vector<VarPtr> vars;
  vector<FunctionPtr> methods;

  std::vector<Assumption> assumptions;
  int assumptions_inited_vars;

  SrcFilePtr file_id;
  string src_name, header_name;

  ClassData();

  inline VarPtr find_var(const std::string &var_name) const {
    for (std::vector<VarPtr>::const_iterator i = vars.begin(); i != vars.end(); ++i) {
      if ((*i)->name == var_name) {
        return *i;
      }
    }
    return VarPtr();
  }

  inline bool is_fully_static() const {
    return 0 == vars.size() && 0 == methods.size();
  }
};

template<>
struct DataTraits<FunctionSet> {
  typedef FunctionPtr value_type;
};

class FunctionSet : public Lockable {
public:
  int id;
  bool is_required;
  string name;
  FunctionPtr req_id;
  VertexPtr header;
private:
  DISALLOW_COPY_AND_ASSIGN (FunctionSet);

  //data:
  std::vector<FunctionPtr> functions;

  //functions:

public:
  FunctionSet();
  bool add_function(FunctionPtr new_function);
  int size();
  FunctionPtr operator[](int i);
};


class FunctionInfo {
public:
  VertexPtr root;
  string namespace_name;
  string class_name;
  string class_context;
  map<string, string> namespace_uses;
  string extends;
  bool kphp_required;
  AccessType access_type;

  FunctionInfo() :
    kphp_required(false),
    access_type(access_nonmember) {}

  FunctionInfo(VertexPtr root, const string &namespace_name, const string &class_name,
               const string &class_context, const map<string, string> namespace_uses,
               string extends, bool kphp_required, AccessType access_type) :
    root(root),
    namespace_name(namespace_name),
    class_name(class_name),
    class_context(class_context),
    namespace_uses(namespace_uses),
    extends(extends),
    kphp_required(kphp_required),
    access_type(access_type) {}

  void fill_namespace_and_class_name(const string &full_name) {
    size_t pos = full_name.rfind('\\');
    namespace_name = full_name.substr(0, pos);
    class_name = full_name.substr(pos + 1);
  }
};

class FunctionData {
public:
  int id;

  VertexPtr root;
  VertexPtr header;

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
  string src_full_name, header_full_name;
  SrcFilePtr file_id;
  FunctionPtr req_id;
  FunctionPtr fork_prev, wait_prev;
  ClassPtr class_id;
  bool varg_flag;

  int tinf_state;
  vector<tinf::VarNode> tinf_nodes;

  VertexPtr const_data;
  Token *phpdoc_token;

  int min_argn;
  bool is_extern;
  bool used_in_source;
  bool is_callback;
  bool should_be_sync;
  bool kphp_required;
  bool is_template;
  string namespace_name;
  string class_name;
  string class_context_name;
  string class_extends;
  AccessType access_type;
  map<string, string> namespace_uses;
  set<string> disabled_warnings;

  FunctionSetPtr function_set;

  FunctionData();
  explicit FunctionData(VertexPtr root);
  string name;

  map<long long, int> name_gen_map;

  inline func_type_t &type() { return type_; }

  bool is_static_init_empty_body() const;
  string get_resumable_path() const;

  inline bool is_instance_function() const {
    return access_type == access_public || access_type == access_protected || access_type == access_private;
  }

  inline bool is_constructor() const {
    return class_id.not_null() && class_id->new_function.ptr == this;
  }

private:
  DISALLOW_COPY_AND_ASSIGN (FunctionData);
};

inline bool operator<(FunctionPtr a, FunctionPtr b) {
  if (a->name == b->name) {
    return (unsigned long)a.ptr < (unsigned long)b.ptr;
  }
  return a->name < b->name;
}

inline bool operator==(const FunctionPtr &a, const FunctionPtr &b) {
  return (unsigned long)a.ptr == (unsigned long)b.ptr;
}

inline bool operator<(const VarPtr &a, const VarPtr &b) {
  int cmp_res = a->name.compare(b->name);

  if (cmp_res == 0) {
    bool af = a->static_id.not_null();
    bool bf = b->static_id.not_null();
    if (af || bf) {
      if (af && bf) {
        return a->static_id < b->static_id;
      } else {
        return af < bf;
      }
    }
    return (unsigned long)a.ptr < (unsigned long)b.ptr;
  }

  if (a->dependency_level != b->dependency_level) {
    return a->dependency_level < b->dependency_level;
  }

  return cmp_res < 0;
}

inline bool operator==(const VarPtr &a, const VarPtr &b) {
  return (unsigned long)a.ptr == (unsigned long)b.ptr;
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

