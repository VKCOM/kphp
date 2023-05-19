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
#include "compiler/data/performance-inspections.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/debug.h"
#include "compiler/function-colors.h"
#include "compiler/inferring/var-node.h"
#include "compiler/threading/data-stream.h"
#include "compiler/vertex-meta_op_base.h"

class GenericsDeclarationMixin;
class KphpTracingDeclarationMixin;

class FunctionData {
  DEBUG_STRING_METHOD { return as_human_readable(); }
  
  // code outside of the data/ should use FunctionData::create_function()
  FunctionData() = default;
  FunctionData& operator=(const FunctionData &other) = default;
  FunctionData(const FunctionData &other) = default;

public:
  int id = -1;

  std::string name;        // full function name; when it belongs to a class, it looks like VK$Namespace$$funcname
  VertexAdaptor<op_function> root;
  bool is_required = false;

  enum func_type_t {
    func_main,
    func_local,
    func_lambda,
    func_switch,
    func_extern,
    func_class_holder
  };
  func_type_t type = func_local;

  std::vector<VarPtr> local_var_ids, global_var_ids, static_var_ids, param_ids;
  std::unordered_set<VarPtr> *bad_vars = nullptr;     // for check ub and safe operations wrapper, see comments in check-ub.cpp
  std::set<VarPtr> implicit_const_var_ids, explicit_const_var_ids, explicit_header_const_var_ids;
  std::vector<FunctionPtr> dep;
  std::set<ClassPtr> class_dep;
  std::set<ClassPtr> exceptions_thrown; // exceptions that can be thrown by this function
  bool tl_common_h_dep = false;

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

  std::string src_name, header_name;
  std::string subdir;
  std::string header_full_name;

  SrcFilePtr file_id;
  ModulitePtr modulite;
  FunctionPtr fork_prev, wait_prev;
  FunctionPtr throws_reason;
  Location throws_location;
  std::set<std::string> disabled_warnings;
  std::map<size_t, int> name_gen_map;

  const TypeHint *return_typehint{nullptr};
  tinf::VarNode tinf_node;              // tinf node for return

  const PhpDocComment *phpdoc{nullptr};

  // TODO: pack most (all?) bool fields to std::bitset
  // can save a few bytes per every FunctionData object even if we add more flags later;
  // or use bit fields like we do in vertex base class
  bool has_variadic_param = false;
  bool should_be_sync = false;
  bool should_not_throw = false;
  bool kphp_lib_export = false;
  bool is_auto_inherited = false;
  bool is_inline = false;
  bool cpp_template_call = false;
  bool cpp_variadic_call = false;
  bool is_resumable = false;
  bool can_be_implicitly_interrupted_by_other_resumable = false;
  bool is_virtual_method = false;
  bool is_overridden_method = false;
  bool is_no_return = false;
  bool has_lambdas_inside = false;      // used for optimization after cloning (not to launch CloneNestedLambdasPass)
  bool has_var_tags_inside = false;     // used for optimization (not to traverse body if no @var inside)
  bool has_commentTs_inside = false;    // used for optimization (not to traverse body if no /*<...>*/ inside)
  bool warn_unused_result = false;
  bool is_flatten = false;
  bool is_pure = false;
  bool is_internal = false; // whether this function can be called only when inserted by the compiler
  bool is_result_indexing = false; // whether this function implicitly does array indexing inside
  bool is_result_array2tuple = false; // whether this function result is optimized from array to tuple, and we need to preserve the semantics

  int8_t readonly_param_index = -1; // until we need more than one, encode it as a byte-sized index

  function_palette::ColorContainer colors{};            // colors specified with @kphp-color
  std::vector<FunctionPtr> *next_with_colors{nullptr};  // next colored functions reachable via call graph

  enum class profiler_status : uint8_t {
    disable,
    // A function that is being profiled that starts and ends the profiling
    enable_as_root,
    // A function that will be profiled if it's reachable from the root (inline functions are not profiled)
    enable_as_child,
    // A function that will be profiled if it's reachable from the root even if it's an inline function
    enable_as_inline_child,
  } profiler_state = profiler_status::disable;

  PerformanceInspections performance_inspections_for_analysis;
  PerformanceInspections performance_inspections_for_warning;
  std::forward_list<FunctionPtr> performance_inspections_for_warning_parents;

  // non-null for generic functions, e.g. f<T> or g<T1, T2>
  // describes what's actually written in @kphp-generic: that f has T, probably with extends_hint
  GenericsDeclarationMixin *genericTs{nullptr};
  // non-null for instantiations of generic functions, e.g. f<User>
  // describes a mapping between generic types (T) and actual instantiation types (User)
  GenericsInstantiationMixin *instantiationTs{nullptr};

  // non-null for functions marked with @kphp-tracing, contains parsed tag contents
  KphpTracingDeclarationMixin *kphp_tracing{nullptr};

  ClassPtr class_id;
  ClassPtr context_class;
  FunctionModifiers modifiers = FunctionModifiers::nonmember();

  enum class body_value {
    empty,
    non_empty,
    unknown,
  } body_seq = body_value::unknown;

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

  vk::string_view get_tmp_string_specialization() const;

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
