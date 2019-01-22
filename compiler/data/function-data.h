#pragma once

#include "auto/compiler/vertex/vertex-meta_op_function.h"
#include "common/mixin/not_copyable.h"

#include "compiler/class-assumptions.h"
#include "compiler/data/class-members.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/lambda-class-data.h"
#include "compiler/inferring/var-node.h"
#include "compiler/stage.h"
#include "compiler/threading/data-stream.h"
#include "compiler/vertex-meta_op_base.h"

class FunctionData : private vk::not_copyable {
public:
  // при @kphp-infer hint/check над функцией — все необходимые type rule хранятся в векторе infer_hints
  struct InferHint {
    enum infer_mask {
      check = 0b001,
      hint = 0b010,
      cast = 0b100
    };

    infer_mask infer_type;
    int param_i;            // 0..N — аргументы, -1 return (как и для tinf)
    VertexPtr type_rule;    // op_lt_type_rule / op_common_type_rule / etc
  };

  int id;

  string name;        // полное имя функции, в случае принадлежности классу это VK$Namespace$funcname
  VertexAdaptor<meta_op_function> root;
  VertexPtr header;   // это только для костыля extern_function, потом должно уйти
  bool is_required;

  enum func_type_t {
    func_global,
    func_local,
    func_switch,
    func_extern,
    func_class_holder
  };
  func_type_t type_;

  vector<VarPtr> local_var_ids, global_var_ids, static_var_ids, header_global_var_ids;
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

  int tinf_state;
  vector<tinf::VarNode> tinf_nodes;
  vector<InferHint> infer_hints;        // kphp-infer hint/check для param/return

  Token *phpdoc_token;

  int min_argn;
  bool used_in_source;    // это только для костыля extern_function, потом должно уйти
  bool is_vararg;
  bool should_be_sync;
  bool kphp_lib_export;
  bool is_template;
  bool is_auto_inherited;
  bool is_inline;
  bool can_throw;
  bool cpp_template_call;
  bool is_resumable;

  ClassPtr context_class;
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
  static FunctionPtr create_function(VertexAdaptor<meta_op_function> root, func_type_t type);

  inline func_type_t &type() { return type_; }

  bool is_static_init_empty_body() const;
  string get_resumable_path() const;
  static string get_human_readable_name(const std::string &name);
  string get_human_readable_name() const;
  void add_kphp_infer_hint(InferHint::infer_mask infer_mask, int param_i, VertexPtr type_rule);

  inline static bool is_instance_access_type(AccessType access_type) {
    return vk::any_of_equal(access_type, access_public, access_protected, access_private);
  }

  inline static bool is_static_access_type(AccessType access_type) {
    return vk::any_of_equal(access_type, access_static_public, access_static_protected, access_static_private);
  }

  inline bool is_instance_function() const {
    return is_instance_access_type(access_type);
  }

  inline bool is_static_function() const {
    return is_static_access_type(access_type);
  }

  inline bool has_implicit_this_arg() const {
    return is_instance_function() && !is_constructor();
  }

  inline bool is_extern() const {
    return type_ == func_extern;
  }

  bool is_constructor() const;

  void update_location_in_body();
  static FunctionPtr generate_instance_of_template_function(const std::map<int, std::pair<AssumType, ClassPtr>> &template_type_id_to_ClassPtr,
                                                            FunctionPtr func,
                                                            const std::string &name_of_function_instance);

  bool is_lambda() const {
    return static_cast<bool>(function_in_which_lambda_was_created);
  }

  bool is_lambda_with_uses() const;

  bool is_imported_from_static_lib() const;

  const FunctionData *get_this_or_topmost_if_lambda() const {
    return is_lambda() ? function_in_which_lambda_was_created->get_this_or_topmost_if_lambda() : this;
  }

  VertexRange get_params();

};
