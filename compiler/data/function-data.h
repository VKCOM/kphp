#pragma once

#include "auto/compiler/vertex/vertex-op_function.h"
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
  // внешний код должен использовать FunctionData::create_function()
  FunctionData() = default;

public:
  // при @kphp-infer hint/check над функцией — все необходимые type rule хранятся в векторе infer_hints
  struct InferHint {
    enum infer_mask {
      check = 0b001,
      hint = 0b010,
      cast = 0b100
    };

    InferHint() = delete;   // создать можно только определяя все 3 параметра

    infer_mask infer_type;
    int param_i;            // 0..N — аргументы, -1 return (как и для tinf)
    VertexPtr type_rule;    // op_lt_type_rule / op_common_type_rule / etc
  };

  int id = -1;

  string name;        // полное имя функции, в случае принадлежности классу это VK$Namespace$funcname
  VertexAdaptor<op_function> root;
  bool is_required = false;

  enum func_type_t {
    func_global,
    func_local,
    func_switch,
    func_extern,
    func_class_holder
  };
  func_type_t type = func_local;

  vector<VarPtr> local_var_ids, global_var_ids, static_var_ids;
  vector<VarPtr> *bad_vars = nullptr;
  set<VarPtr> implicit_const_var_ids, explicit_const_var_ids, explicit_header_const_var_ids;
  vector<VarPtr> param_ids;
  vector<FunctionPtr> dep;
  std::set<ClassPtr> class_dep;
  FunctionPtr function_in_which_lambda_was_created;
  //std::vector<FunctionPtr> lambdas_inside;    // todo когда будем разрешать лямбды в шаблонных функциях, find usages

  std::vector<Assumption> assumptions_for_vars;
  Assumption assumption_for_return;
  int assumptions_inited_args = 0;
  volatile int assumptions_inited_return = 0;

  string src_name, header_name;
  string subdir;
  string header_full_name;

  SrcFilePtr file_id;
  FunctionPtr fork_prev, wait_prev;
  set<string> disabled_warnings;
  map<size_t, int> name_gen_map;

  int tinf_state = 0;
  vector<tinf::VarNode> tinf_nodes;
  vector<InferHint> infer_hints;        // kphp-infer hint/check для param/return

  int min_argn = 0;
  bool is_vararg = false;
  bool has_variadic_param = false;
  bool should_be_sync = false;
  bool kphp_lib_export = false;
  bool is_template = false;
  bool is_auto_inherited = false;
  bool is_inline = false;
  bool can_throw = false;
  bool cpp_template_call = false;
  bool is_resumable = false;
  bool is_final = false;

  ClassPtr class_id;
  ClassPtr context_class;
  AccessType access_type = access_nonmember;
  Token *phpdoc_token = nullptr;


  enum class body_value {
    empty,
    non_empty,
    unknown,
  } body_seq = body_value::unknown;

  static FunctionPtr create_function(VertexAdaptor<op_function> root, func_type_t type);

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
    return type == func_extern;
  }

  bool is_constructor() const;

  void update_location_in_body();
  static std::string encode_template_arg_name(AssumType assum, int id, ClassPtr klass);
  static FunctionPtr generate_instance_of_template_function(const std::map<int, std::pair<AssumType, ClassPtr>> &template_type_id_to_ClassPtr,
                                                            FunctionPtr func,
                                                            const std::string &name_of_function_instance);
  std::vector<VertexAdaptor<op_var>> get_params_as_vector_of_vars(int shift = 0) const;

  void calc_min_argn();

  bool is_lambda() const {
    return static_cast<bool>(function_in_which_lambda_was_created);
  }

  bool is_lambda_with_uses() const;

  bool is_imported_from_static_lib() const;

  const FunctionData *get_this_or_topmost_if_lambda() const {
    return is_lambda() ? function_in_which_lambda_was_created->get_this_or_topmost_if_lambda() : this;
  }

  VertexRange get_params() const;

};
