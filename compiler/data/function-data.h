// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <map>
#include <set>
#include <string>
#include <thread>

#include "auto/compiler/vertex/vertex-op_function.h"
#include "common/mixin/not_copyable.h"
#include "common/wrappers/copyable-atomic.h"

#include "compiler/class-assumptions.h"
#include "compiler/data/class-members.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/function-modifiers.h"
#include "compiler/data/generics-mixins.h"
#include "compiler/data/performance-inspections.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/debug.h"
#include "compiler/function-colors.h"
#include "compiler/inferring/var-node.h"
#include "compiler/threading/data-stream.h"
#include "compiler/vertex-meta_op_base.h"

class FunctionData {
  DEBUG_STRING_METHOD { return as_human_readable(); }
  
  // code outside of the data/ should use FunctionData::create_function()
  FunctionData()
    : tl_common_h_dep{false}
    , is_required{false}
    , has_variadic_param{false}
    , should_be_sync{false}
    , should_not_throw{false}
    , kphp_lib_export{false}
    , is_auto_inherited{false}
    , is_inline{false}
    , cpp_template_call{false}
    , cpp_variadic_call{false}
    , is_resumable{false}
    , can_be_implicitly_interrupted_by_other_resumable{false}
    , is_virtual_method{false}
    , is_overridden_method{false}
    , is_no_return{false}
    , has_lambdas_inside{false}
    , has_var_tags_inside{false}
    , has_commentTs_inside{false}
    , warn_unused_result{false}
    , is_flatten{false}
    , is_pure{false}
    , is_internal{false}
    , ignore_return_warning{false}
  {}

  FunctionData& operator=(const FunctionData &other) = default;
  FunctionData(const FunctionData &other) = default;

public:
  int id = -1;

  bool tl_common_h_dep : 1;
  bool is_required : 1;
  bool has_variadic_param : 1;
  bool should_be_sync : 1;
  bool should_not_throw : 1;
  bool kphp_lib_export : 1;
  bool is_auto_inherited : 1;
  bool is_inline : 1;
  bool cpp_template_call : 1;
  bool cpp_variadic_call : 1;
  bool is_resumable : 1;
  bool can_be_implicitly_interrupted_by_other_resumable : 1;
  bool is_virtual_method : 1;
  bool is_overridden_method : 1;
  bool is_no_return : 1;
  bool has_lambdas_inside : 1;      // used for optimization after cloning (not to launch CloneNestedLambdasPass)
  bool has_var_tags_inside : 1;     // used for optimization (not to traverse body if no @var inside)
  bool has_commentTs_inside : 1;    // used for optimization (not to traverse body if no /*<...>*/ inside)
  bool warn_unused_result : 1;
  bool is_flatten : 1;
  bool is_pure : 1;
  bool is_internal : 1; // whether this function can be called only when inserted by the compiler
  bool ignore_return_warning : 1;

  FunctionModifiers modifiers = FunctionModifiers::nonmember();

  enum class profiler_status : uint8_t {
    disable,
    // A function that is being profiled that starts and ends the profiling
    enable_as_root,
    // A function that will be profiled if it's reachable from the root (inline functions are not profiled)
    enable_as_child,
    // A function that will be profiled if it's reachable from the root even if it's an inline function
    enable_as_inline_child,
  } profiler_state = profiler_status::disable;

  enum class body_value: uint8_t {
    empty,
    non_empty,
    unknown,
  } body_seq = body_value::unknown;

  enum func_type_t: uint8_t {
    func_main,
    func_local,
    func_lambda,
    func_switch,
    func_extern,
    func_class_holder
  };
  func_type_t type = func_local;

  VertexAdaptor<op_function> root;

  std::unordered_set<VarPtr> *bad_vars = nullptr;     // for check ub and safe operations wrapper, see comments in check-ub.cpp
  std::vector<VarPtr> local_var_ids, global_var_ids, static_var_ids, param_ids;
  std::set<VarPtr> implicit_const_var_ids, explicit_const_var_ids, explicit_header_const_var_ids;
  std::vector<FunctionPtr> dep;
  std::set<ClassPtr> class_dep;
  std::set<ClassPtr> exceptions_thrown; // exceptions that can be thrown by this function

  // for lambdas: a function that contains this lambda ($this is captured from outer_function, it can also be a lambda on nesting)
  // for generic instantiations: refs to an original (a generic) function
  // for __invoke method of a lambda: refs to a lambda function that's called from this __invoke
  FunctionPtr outer_function;

  // use($var1, &$var2) for lambdas, implicit vars for arrow lambdas; auto-captured $this is also here, the first one
  std::forward_list<VertexAdaptor<op_var>> uses_list;

  // @kphp-throws checks that a function throws only specified exceptions;
  // empty vector means "nothing to check", it's not "throws nothing",
  // to check that a function does not throw, should_not_throw flag is used
  std::forward_list<std::string> check_throws;

  std::forward_list<std::pair<std::string, Assumption>> assumptions_for_vars;   // (var_name, assumption)[]
  Assumption assumption_for_return;

  enum class AssumptionStatus {
    uninitialized,
    processing_deduce_pass,
    done_deduce_pass,
    processing_returns_in_body,
    done_returns_in_body
  };
  vk::copyable_atomic<AssumptionStatus> assumption_pass_status{AssumptionStatus::uninitialized};
  vk::copyable_atomic<std::thread::id> assumption_processing_thread{std::thread::id{}};

  std::string name;        // full function name; when it belongs to a class, it looks like VK$Namespace$$funcname
  std::string src_name, header_name;
  std::string subdir;
  std::string header_full_name;

  const TypeHint *return_typehint{nullptr};
  const PhpDocComment *phpdoc{nullptr};

  SrcFilePtr file_id;
  ModulitePtr modulite;
  FunctionPtr fork_prev, wait_prev;
  FunctionPtr throws_reason;
  Location throws_location;
  std::map<size_t, int> name_gen_map;

  tinf::VarNode tinf_node;              // tinf node for return

  function_palette::ColorContainer colors{};            // colors specified with @kphp-color
  std::vector<FunctionPtr> *next_with_colors{nullptr};  // next colored functions reachable via call graph

  std::forward_list<FunctionPtr> performance_inspections_for_warning_parents;
  PerformanceInspections performance_inspections_for_analysis;
  PerformanceInspections performance_inspections_for_warning;

  // non-null for generic functions, e.g. f<T> or g<T1, T2>
  // describes what's actually written in @kphp-generic: that f has T, probably with extends_hint
  GenericsDeclarationMixin *genericTs{nullptr};
  // non-null for instantiations of generic functions, e.g. f<User>
  // describes a mapping between generic types (T) and actual instantiation types (User)
  GenericsInstantiationMixin *instantiationTs{nullptr};

  ClassPtr class_id;
  ClassPtr context_class;

  static FunctionPtr create_function(std::string name, VertexAdaptor<op_function> root, func_type_t type);
  static FunctionPtr clone_from(FunctionPtr other, const std::string &new_name, VertexAdaptor<op_function> root_instead_of_cloning = {});

  std::string get_resumable_path() const;
  std::string get_throws_call_chain() const;
  std::string get_performance_inspections_warning_chain(PerformanceInspections::Inspections inspection, bool search_disabled_inspection = false) const noexcept;
  std::string as_human_readable(bool add_details = true) const;

  bool can_throw() const noexcept  {
    return !exceptions_thrown.empty();
  }

  bool has_implicit_this_arg() const {
    return modifiers.is_instance();
  }

  bool is_extern() const {
    return type == func_extern;
  }

  bool is_main_function() const {
    return type == func_type_t::func_main;
  }

  bool is_lambda() const {
    return type == func_lambda;
  }

  bool is_generic() const {
    return genericTs != nullptr;
  }

  bool is_instantiation_of_generic_function() const {
    return instantiationTs != nullptr;
  }

  bool is_constructor() const;
  bool is_invoke_method() const;
  bool is_imported_from_static_lib() const;

  const FunctionData *get_this_or_topmost_if_lambda() const {
    return type == FunctionData::func_lambda ? outer_function->get_this_or_topmost_if_lambda() : this;
  }

  void update_location_in_body();
  std::vector<VertexAdaptor<op_var>> get_params_as_vector_of_vars(int shift = 0) const;
  void move_virtual_to_self_method(DataStream<FunctionPtr> &os);
  static std::string get_name_of_self_method(vk::string_view name);
  std::string get_name_of_self_method() const;

  // a range of op_func_param, but it can't be expressed with VertexRange and needs .as<op_func_param>() when using
  VertexRange get_params() const { return root->param_list()->params(); }

  Assumption get_assumption_for_var(const std::string &var_name) {
    for (const auto &name_and_a : assumptions_for_vars) {
      if (name_and_a.first == var_name) {
        return name_and_a.second;
      }
    }
    return {};
  }

  VertexAdaptor<op_func_param> find_param_by_name(vk::string_view var_name);
  VertexAdaptor<op_var> find_use_by_name(const std::string &var_name);
  VarPtr find_var_by_name(const std::string &var_name);
  int get_min_argn() const;

  vk::string_view local_name() const & { return get_local_name_from_global_$$(name); }
  vk::string_view local_name() const && = delete;

  bool does_need_codegen() const;

private:
  FunctionPtr get_self() const {
    return FunctionPtr{const_cast<FunctionData *>(this)};
  }
};
