// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt
#pragma once
#include "compiler/function-pass.h"

class AnalyzePerformance final : public FunctionPassBase {
public:
  std::string get_description() final {
    return "Analyze performance";
  }

  bool check_function(FunctionPtr func) const final;
  VertexPtr on_enter_vertex(VertexPtr vertex) final;
  void on_finish() final;

  struct Issue {
    int line;
    PerformanceInspections::Inspections inspection;
    std::string description;
  };
private:
  void analyze_func_call(VertexAdaptor<op_func_call> func_call) noexcept;
  void analyze_set(VertexAdaptor<op_set> op_set_vertex) noexcept;
  void analyze_set_array_value(VertexAdaptor<op_set_value> op_set_value_vertex) noexcept;
  template<Operation op>
  void analyze_array_insertion(VertexAdaptor<op> vertex) noexcept;
  void analyze_op_array(VertexAdaptor<op_array> op_array_vertex) noexcept;

  void check_implicit_array_conversion(VertexPtr expr, const TypeData *to) noexcept;

  static uint64_t get_function_inspections(FunctionPtr func) noexcept {
    return func->performance_inspections_for_analyse.inspections() | func->performance_inspections_for_warning.inspections();
  }

  template<PerformanceInspections::Inspections inspection>
  bool is_enabled() const noexcept {
    return get_function_inspections(current_function) & inspection;
  }

  void on_array_merge() noexcept;
  void check_and_fire_warning(PerformanceInspections::Inspections inspection, vk::string_view message) noexcept;
  void check_and_save_performance_issue_for_analyse(PerformanceInspections::Inspections inspection, std::string message) noexcept;

  bool warnings_enabled_{false};
  std::vector<Issue> performance_issues_for_analyse_;
};

class PerformanceIssuesReport : vk::not_copyable {
public:
  static PerformanceIssuesReport &get() noexcept {
    static PerformanceIssuesReport self;
    return self;
  }

  void add_issues(FunctionPtr function, std::vector<AnalyzePerformance::Issue> &&issues) noexcept;

  bool empty() noexcept;
  void flush_to(FILE *out) noexcept;

private:
  PerformanceIssuesReport() = default;

  struct FunctionPerformanceIssues {
    std::string file_name;
    std::string function_name;
    std::vector<AnalyzePerformance::Issue> issues;
  };

  std::mutex mutex_;
  std::vector<FunctionPerformanceIssues> report_;
};
