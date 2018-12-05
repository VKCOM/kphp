#pragma once

#include "compiler/class-assumptions.h"
#include "compiler/data/class-members.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/function-info.h"
#include "compiler/inferring/var-node.h"
#include "compiler/stage.h"
#include "compiler/threading/data-stream.h"
#include "compiler/vertex-meta_op_base.h"

class FunctionData {
public:
  int id;

  string name;        // полное имя функции, в случае принадлежности классу это VK$Namespace$funcname
  VertexAdaptor<meta_op_function> root;
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
  set<VarPtr> const_var_ids, header_const_var_ids;
  vector<VarPtr> param_ids;
  vector<FunctionPtr> dep;

  std::vector<Assumption> assumptions_for_vars;
  Assumption assumption_for_return;
  int assumptions_inited_args;
  volatile int assumptions_inited_return;

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
  FunctionPtr function_in_which_lambda_was_created;
  std::vector<FunctionPtr> lambdas_inside;

  enum class body_value {
    empty,
    non_empty,
    unknown,
  } body_seq;

  FunctionData();
  explicit FunctionData(VertexPtr root);
  static FunctionPtr create_function(const FunctionInfo &info);

  inline func_type_t &type() { return type_; }

  bool is_static_init_empty_body() const;
  string get_resumable_path() const;
  string get_human_readable_name() const;

  inline static bool is_instance_function(AccessType access_type) {
    return vk::any_of_equal(access_type, access_public, access_protected, access_private);
  }

  inline bool is_instance_function() const {
    return is_instance_function(access_type);
  }

  inline bool is_static_function() const {
    return vk::any_of_equal(access_type, access_static_public, access_static_protected, access_static_private);
  }

  inline bool has_implicit_this_arg() const {
    return is_instance_function() && !is_constructor();
  }

  bool is_constructor() const;

  static FunctionPtr generate_instance_of_template_function(const std::map<int, std::pair<AssumType, ClassPtr>> &template_type_id_to_ClassPtr,
                                                            FunctionPtr func,
                                                            const std::string &name_of_function_instance);

  void require_all_lambdas_inside(DataStream<FunctionPtr> &os) const;

  static ClassPtr is_lambda(VertexPtr v);

  static const std::string &get_lambda_namespace() {
    static std::string lambda_namespace("$L");
    return lambda_namespace;
  }

  bool is_lambda() const {
    return namespace_name == get_lambda_namespace();
  }

  bool is_lambda_with_uses() const;

  const std::string get_outer_namespace_name() const {
    return get_or_default_field(&FunctionData::namespace_name);
  }

  ClassPtr get_outer_class() const {
    return function_in_which_lambda_was_created ? function_in_which_lambda_was_created->class_id : class_id;
  }

  const std::string &get_outer_class_context_name() const {
    return get_or_default_field(&FunctionData::class_context_name);
  }

  VertexRange get_params();

private:
  const std::string &get_or_default_field(std::string (FunctionData::*field)) const {
    if (is_lambda()) {
      kphp_assert(function_in_which_lambda_was_created);
      return function_in_which_lambda_was_created->get_or_default_field(field);
    }

    return this->*field;
  }

  DISALLOW_COPY_AND_ASSIGN (FunctionData);
};
