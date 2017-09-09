#pragma once
#include "compiler/bicycle.h"
#include "compiler/data_ptr.h"
#include "compiler/type-inferer-core.h"
#include "compiler/utils.h"
#include "compiler/vertex.h"

extern IdGen <VertexPtr> tree_id_gen;
#define reg_vertex(V) ({                  \
    VertexPtr reg_vertex_tmp_res__ = reg_vertex_impl (V);  \
    reg_vertex_tmp_res__;                                  \
})
VertexPtr reg_vertex_impl (VertexPtr v);

//hack :(
template <>
struct DataTraits<Vertex> {
  typedef VertexPtr &value_type;
};

typedef enum {access_nonmember = 0, access_default, access_public, access_private, access_protected} AccessType;

class VarData {
public:
  typedef enum {var_unknown_t = 0, var_local_t, var_global_t, var_param_t, var_const_t, var_static_t} Type;
  Type type_;
  int id;
  int param_i;
  string name;
  TypeInfPtr tinf;
  tinf::VarNode tinf_node;
  VertexPtr init_val;
  FunctionPtr holder_func;
  FunctionPtr static_id; // id of function if variable is static
  ClassPtr class_id; // id of class if variable is static fields
  vector <VarPtr> *bad_vars;
  bool is_constant;
  bool is_reference;
  bool uninited_flag;
  bool optimize_flag;
  bool tinf_flag;
  bool global_init_flag;
  bool needs_const_iterator_flag;
  AccessType access_type;
  void set_uninited_flag (bool f);
  bool get_uninited_flag();

  VarData (Type type = var_unknown_t);
  inline Type &type() {return type_;}
};

struct ClassInfo {
  string name;
  VertexPtr root;
  vector <VertexPtr> members;
  vector <VertexPtr> constants;
  vector <VertexPtr> static_members;
};

class ClassData {
public:
  int id;
  bool is_inited;
  bool is_required;
  FunctionPtr req_func;
  string name;
  VertexPtr root;

  FunctionPtr init_function;
  FunctionPtr new_function;
  vector <FunctionPtr> member_functions;

  string header_name;
  string subdir;
  string header_full_name;
  SrcFilePtr file_id;

  ClassData();
};

template <>
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
    std::vector <FunctionPtr> functions;

    //functions:

  public:
    FunctionSet();
    bool add_function (FunctionPtr new_function);
    int size();
    FunctionPtr operator[] (int i);
};

class FunctionMatcher {
  private:
    DISALLOW_COPY_AND_ASSIGN (FunctionMatcher);

    //data:
    FunctionSetPtr function_set;
    FunctionPtr match;

  public:
    FunctionMatcher();
    void set_function_set (FunctionSetPtr new_function_set);
    void try_match (VertexPtr call);
    FunctionPtr get_function();
    //equivalent to this->get_function().not_null()
    bool is_ready();
};

class FunctionInfo {
public:
  VertexPtr root;
  string namespace_name;
  string class_name;
  map<string, string> namespace_uses;
  set<string> disabled_warnings;

  FunctionInfo(VertexPtr root, const string &namespace_name,
               const string &class_name, const map<string, string> namespace_uses,
               const set<string> disabled_warnings)                                : root(root),
                                                                                     namespace_name(namespace_name),
                                                                                     class_name(class_name),
                                                                                     namespace_uses(namespace_uses),
                                                                                     disabled_warnings(disabled_warnings) {
  }
};

class FunctionData {
public:
  int id;

  VertexPtr root;
  VertexPtr header;

  bool is_required;
  typedef enum {func_global, func_local, func_switch, func_extern} func_type_t;
  func_type_t type_;

  bool lazy_flag;

  vector <VarPtr> local_var_ids, global_var_ids, static_var_ids, header_global_var_ids;
  vector <VarPtr> tmp_vars;
  vector <VarPtr> *bad_vars;
  vector <DefinePtr> define_ids;
  set <VarPtr> const_var_ids, header_const_var_ids;
  vector <VarPtr> param_ids;
  vector <FunctionPtr> dep, rdep;
  queue <pair <VertexPtr , FunctionPtr> > calls_to_process;

  string src_name, header_name;
  string subdir;
  string src_full_name, header_full_name;
  SrcFilePtr file_id;
  FunctionPtr req_id;
  FunctionPtr fork_prev, wait_prev;
  ClassPtr class_id;
  bool varg_flag;

  vector <TypeInfPtr > tinf;
  int tinf_state;
  vector <tinf::VarNode> tinf_nodes;

  VertexPtr const_data;

  int min_argn;
  bool is_extern;
  bool used_in_source;
  bool is_callback;
  string namespace_name;
  string class_name;
  AccessType access_type;
  map<string, string> namespace_uses;
  set<string> disabled_warnings;

  FunctionSetPtr function_set;

  FunctionData();
  explicit FunctionData (VertexPtr root);
  string name;

  map <long long, int> name_gen_map;

  inline func_type_t &type() {return type_;}

  bool is_static_init_empty_body() const;
  string get_resumable_path() const;
private:
  DISALLOW_COPY_AND_ASSIGN (FunctionData);
};
inline bool operator < (FunctionPtr a, FunctionPtr b) {
  if (a->name == b->name) {
    return (unsigned long)a.ptr < (unsigned long)b.ptr;
  }
  return a->name < b->name;
}
inline bool operator == (const FunctionPtr &a, const FunctionPtr &b) {
  return (unsigned long)a.ptr == (unsigned long)b.ptr;
}

inline bool operator < (const VarPtr &a, const VarPtr &b) {
  if (a->name == b->name) {
    bool af = a->static_id.not_null(), bf = b->static_id.not_null();
    if (af || bf) {
      if (af && bf) {
        return a->static_id < b->static_id;
      } else {
        return af < bf;
      }
    }
    return (unsigned long)a.ptr < (unsigned long)b.ptr;
  }
  return a->name < b->name;
}

inline bool operator == (const VarPtr &a, const VarPtr &b) {
  return (unsigned long)a.ptr == (unsigned long)b.ptr;
}


class DefineData {
public:
  int id;

  VertexPtr val;
  string name;
  int pos_begin, pos_end;
  SrcFilePtr file_id;
  typedef enum {def_php, def_raw, def_var} DefineType;

  DefineType type_;

  DefineData();
  DefineData (VertexPtr val, DefineType type_);

  inline DefineType &type() {return type_;}

private:
  DISALLOW_COPY_AND_ASSIGN (DefineData);
};

typedef Range <vector <FunctionPtr>::iterator > FunctionRange;
typedef Range <vector <SrcFilePtr>::iterator> SrcFileRange;

extern int tree_node_dfs_cnt;


