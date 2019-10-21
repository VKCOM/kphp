#pragma once

#include <map>
#include <set>

#include "compiler/class-assumptions.h"
#include "compiler/data/class-members.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/function-modifiers.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/inferring/var-node.h"
#include "compiler/threading/data-stream.h"
#include "compiler/vertex-meta_op_base.h"
#include "auto/compiler/vertex/vertex-op_function.h"
#include "common/mixin/not_copyable.h"

class FunctionData {
  // внешний код должен использовать FunctionData::create_function()
  FunctionData() = default;
  FunctionData& operator=(const FunctionData &other) = default;
  FunctionData(const FunctionData &other) = default;

private:
  int min_argn = -1;

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
  std::set<VarPtr> implicit_const_var_ids, explicit_const_var_ids, explicit_header_const_var_ids;
  vector<VarPtr> param_ids;
  vector<FunctionPtr> dep;
  std::set<ClassPtr> class_dep;
  bool tl_common_h_dep = false;
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
  std::set<string> disabled_warnings;
  std::map<size_t, int> name_gen_map;

  int tinf_state = 0;
  vector<tinf::VarNode> tinf_nodes;
  vector<InferHint> infer_hints;        // kphp-infer hint/check для param/return
  std::string return_typehint;

  bool is_vararg = false;
  bool has_variadic_param = false;
  bool should_be_sync = false;
  bool kphp_lib_export = false;
  bool is_template = false;
  bool is_auto_inherited = false;
  bool is_inline = false;
  bool can_throw = false;
  bool cpp_template_call = false;
  bool cpp_variadic_call = false;
  bool is_resumable = false;
  bool is_virtual_method = false;
  bool is_overridden_method = false;

  ClassPtr class_id;
  ClassPtr context_class;
  FunctionModifiers modifiers = FunctionModifiers::nonmember();
  vk::string_view phpdoc_str;

  Location instantiation_of_template_function_location;

  enum class body_value {
    empty,
    non_empty,
    unknown,
  } body_seq = body_value::unknown;

  static FunctionPtr create_function(std::string name, VertexAdaptor<op_function> root, func_type_t type);
  static FunctionPtr clone_from(const std::string &new_name, FunctionPtr other, VertexAdaptor<op_function> new_root);

  string get_resumable_path() const;
  static string get_human_readable_name(const std::string &name);
  string get_human_readable_name() const;
  void add_kphp_infer_hint(InferHint::infer_mask infer_mask, int param_i, VertexPtr type_rule);

  inline bool has_implicit_this_arg() const {
    return modifiers.is_instance();
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
  void move_virtual_to_self_method(DataStream<FunctionPtr> &os);
  static std::string get_name_of_self_method(vk::string_view name);
  std::string get_name_of_self_method() const;

  int get_min_argn();

  bool is_lambda() const {
    return static_cast<bool>(function_in_which_lambda_was_created);
  }

  bool is_lambda_with_uses() const;

  bool is_imported_from_static_lib() const;

  const FunctionData *get_this_or_topmost_if_lambda() const {
    return is_lambda() ? function_in_which_lambda_was_created->get_this_or_topmost_if_lambda() : this;
  }

  VertexRange get_params() const;

  static bool check_cnt_params(int expected_cnt_params, FunctionPtr called_func);

  vk::string_view local_name() const & { return get_local_name_from_global_$$(name); }
};
