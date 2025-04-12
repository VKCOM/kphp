// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/smart_ptrs/singleton.h"

#include "compiler/function-pass.h"


class AnalyzePerformance final : public FunctionPassBase {
public:
  std::string get_description() final {
    return "Analyze performance";
  }

  bool check_function(FunctionPtr func) const final;
  VertexPtr on_enter_vertex(VertexPtr vertex) final;
  bool user_recursion(VertexPtr vertex) final;
  VertexPtr on_exit_vertex(VertexPtr vertex) final;
  void on_finish() final;

  struct Issue {
    int line;
    PerformanceInspections::Inspections inspection;
    std::string description;
  };
private:
  void analyze_func_call(VertexAdaptor<op_func_call> op_func_call_vertex) noexcept;
  void analyze_set(VertexAdaptor<op_set> op_set_vertex) noexcept;
  void analyze_set_array_value(VertexAdaptor<op_set_value> op_set_value_vertex) noexcept;
  template<Operation op>
  void analyze_array_insertion(VertexAdaptor<op> vertex) noexcept;
  void analyze_op_array(VertexAdaptor<op_array> op_array_vertex) noexcept;
  void analyze_op_return(VertexAdaptor<op_return> op_return_vertex) noexcept;
  void analyze_op_var(VertexAdaptor<op_var> op_var_vertex) noexcept;

  void check_implicit_array_conversion(VertexPtr expr, const TypeData *to) noexcept;

  void save_to_second_pass_analyze_on_loop_exit(VertexPtr vertex) noexcept;
  void run_second_pass_on_loop_exit(VertexPtr vertex, uint64_t enabled_inspections, VertexPtr loop_vertex) noexcept;

  void enter_loop() noexcept;
  void exit_loop(VertexPtr loop_vertex) noexcept;

  void enter_conditional() noexcept;
  void exit_conditional() noexcept;

  bool is_constant_expression_in_this_loop(VertexPtr vertex) const noexcept;

  static uint64_t get_function_inspections(FunctionPtr func) noexcept {
    return func->performance_inspections_for_analysis.inspections() | func->performance_inspections_for_warning.inspections();
  }

  template<PerformanceInspections::Inspections inspection>
  bool is_enabled() const noexcept {
    return get_function_inspections(current_function) & inspection;
  }

  void trigger_array_merge(VertexPtr first_arg) noexcept;
  void trigger_inspection(PerformanceInspections::Inspections inspection, std::string message) noexcept;
  void trigger_inspection_on_second_pass(VertexPtr inspected_vertex, PerformanceInspections::Inspections inspection, std::string message) noexcept;

  bool warnings_enabled_{false};
  std::vector<Issue> performance_issues_for_analysis_;
  struct LoopSecondPassTraits {
    std::unordered_set<VarPtr> first_pass_modified_vars;
    std::vector<VertexPtr> second_pass_vertexes;
    uint32_t condition_depth{0};
    bool has_break_or_continue{false};
    bool has_return{false};
  };
  std::vector<LoopSecondPassTraits> loop_data_for_second_pass_;
  VertexPtr second_pass_saved_top_vertex_;
  std::unordered_set<VarPtr> reserved_arrays_;
};

class PerformanceIssuesReport : vk::not_copyable {
public:
  friend class vk::singleton<PerformanceIssuesReport>;

  void add_issues_and_require_flush(FunctionPtr function, std::vector<AnalyzePerformance::Issue> &&issues) noexcept;

  bool is_flush_required() const noexcept;
  void flush_to(FILE *out) noexcept;

private:
  PerformanceIssuesReport() = default;

  struct FunctionPerformanceIssues {
    std::string file_name;
    std::string function_name;
    std::vector<AnalyzePerformance::Issue> issues;
  };

  std::atomic<bool> is_flush_required_{false};

  std::mutex mutex_;
  std::vector<FunctionPerformanceIssues> report_;
};
