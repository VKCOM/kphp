// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <map>
#include <set>
#include <thread>

#include "auto/compiler/vertex/vertex-op_function.h"
#include "common/mixin/not_copyable.h"
#include "common/wrappers/copyable-atomic.h"

#include "compiler/class-assumptions.h"
#include "compiler/data/class-members.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/function-modifiers.h"
#include "compiler/data/performance-inspections.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/inferring/var-node.h"
#include "compiler/threading/data-stream.h"
#include "compiler/vertex-meta_op_base.h"

class FunctionData {
  // code outside of the data/ should use FunctionData::create_function()
  FunctionData() = default;
  FunctionData& operator=(const FunctionData &other) = default;
  FunctionData(const FunctionData &other) = default;

private:
  int min_argn = -1;

public:
  struct InferHint {
    enum infer_mask {
      check         = 0b00001,
      hint          = 0b00010,
      hint_check    = 0b00100,
      cast          = 0b01000,
    };

    InferHint(infer_mask infer_type, int param_i, VertexPtr type_rule) :
      infer_type(infer_type),
      param_i(param_i),
      type_rule(type_rule) {
    }


    infer_mask infer_type;
    int param_i;            // 0..N — arguments, -1 return (just like in tinf)
    VertexPtr type_rule;    // op_lt_type_rule / op_common_type_rule / etc
  };

  int id = -1;

  string name;        // full function name; when it belongs to a class it looks like VK$Namespace$funcname
  VertexAdaptor<op_function> root;
  bool is_required = false;

  enum func_type_t {
    func_main,
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

  // TODO: find usages when we'll allow lambdas inside template functions.
  //std::vector<FunctionPtr> lambdas_inside;

  std::vector<std::pair<std::string, vk::intrusive_ptr<Assumption>>> assumptions_for_vars;   // (var_name, assumption)[]
  vk::intrusive_ptr<Assumption> assumption_for_return;

  vk::copyable_atomic<AssumptionStatus> assumption_args_status{AssumptionStatus::unknown};
  vk::copyable_atomic<AssumptionStatus> assumption_return_status{AssumptionStatus::unknown};
  vk::copyable_atomic<std::thread::id> assumption_return_processing_thread{std::thread::id{}};

  string src_name, header_name;
  string subdir;
  string header_full_name;

  SrcFilePtr file_id;
  FunctionPtr fork_prev, wait_prev;
  FunctionPtr throws_reason;
  Location throws_location;
  std::set<string> disabled_warnings;
  std::map<size_t, int> name_gen_map;

  int tinf_state = 0;
  vector<tinf::VarNode> tinf_nodes;
  vector<InferHint> infer_hints;        // kphp-infer hint/check for param/return
  std::string return_typehint;

  bool has_variadic_param = false;
  bool should_be_sync = false;
  bool should_not_throw = false;
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
  bool is_no_return = false;
  bool warn_unused_result = false;
  bool is_flatten = false;
  enum class profiler_status : uint8_t {
    disable,
    // A function that is being profiled that starts and ends the profiling
    enable_as_root,
    // A function that will be profiled if it's reachable from the root (inline functions are not profiled)
    enable_as_child,
    // A function that will be profiled if it's reachable from the root even if it's an inline function
    enable_as_inline_child,
  } profiler_state = profiler_status::disable;

  PerformanceInspections performance_inspections_for_analyse;
  PerformanceInspections performance_inspections_for_warning;
  std::forward_list<FunctionPtr> performance_inspections_for_warning_parents;

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
  string get_throws_call_chain() const;
  std::string get_performance_inspections_warning_chain(PerformanceInspections::Inspections inspection, bool search_disabled_inspection = false) const noexcept;
  static string get_human_readable_name(const std::string &name, bool add_details = true);
  string get_human_readable_name(bool add_details = true) const;
  void add_kphp_infer_hint(InferHint::infer_mask infer_mask, int param_i, VertexPtr type_rule);

  bool has_implicit_this_arg() const {
    return modifiers.is_instance();
  }

  bool is_extern() const {
    return type == func_extern;
  }

  bool is_constructor() const;
  bool is_main_function() const;

  void update_location_in_body();
  static std::string encode_template_arg_name(const vk::intrusive_ptr<Assumption> &assumption, int id);
  static FunctionPtr generate_instance_of_template_function(const std::map<int, vk::intrusive_ptr<Assumption>> &template_type_id_to_ClassPtr,
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
  vk::string_view local_name() const && = delete;

private:
  FunctionPtr get_self() const {
    return FunctionPtr{const_cast<FunctionData *>(this)};
  }
};
