// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt
#include "compiler/pipes/analyze-performance.h"

#include "common/algorithms/string-algorithms.h"
#include "common/termformat/termformat.h"
#include "compiler/compiler-core.h"
#include "compiler/data/src-file.h"
#include "compiler/inferring/public.h"

namespace {

VertexPtr remove_array_conv(VertexPtr vertex) noexcept {
  switch (vertex->type()) {
    case op_conv_array:
      return remove_array_conv(vertex.as<op_conv_array>()->expr());
    case op_conv_drop_false:
      return remove_array_conv(vertex.as<op_conv_drop_false>()->expr());
    case op_conv_drop_null:
      return remove_array_conv(vertex.as<op_conv_drop_null>()->expr());
    default:
      return vertex;
  }
}

std::string extract_expression_name(VertexPtr expr) {
  switch (expr->type()) {
    case op_var:
      return TermStringFormat::paint_green(expr.as<op_var>()->var_id->get_human_readable_name());
    case op_instance_prop:
      return TermStringFormat::paint_green(expr.as<op_instance_prop>()->var_id->get_human_readable_name());
    case op_func_call:
      return TermStringFormat::paint_green(expr.as<op_func_call>()->func_id->get_human_readable_name(false));
    default:
      return "";
  }
}

std::string get_description_for_help(VertexPtr expr) noexcept {
  expr = remove_array_conv(expr);
  switch (expr->type()) {
    case op_var:
      return "variable " + extract_expression_name(expr);
    case op_instance_prop:
      return "variable " + extract_expression_name(expr);
    case op_func_call:
      return "function " + extract_expression_name(expr) + " result";
    case op_array:
      return "array construction [...] result";
    case op_ternary:
      return "ternary operation result";
    case op_index: {
      return "array element " + extract_expression_name(remove_array_conv(expr.as<op_index>()->array())) + "[...]";
    }
    default:
      // TODO add more cases
      return "expresion";
  }
}

bool is_same_var_expression(VertexPtr lhs, VertexPtr rhs) noexcept {
  if (!lhs || !rhs) {
    return false;
  }

  lhs = remove_array_conv(lhs);
  rhs = remove_array_conv(rhs);
  if (lhs->type() != rhs->type()) {
    return false;
  }

  switch (lhs->type()) {
    case op_var:
      if (lhs.as<op_var>()->var_id != rhs.as<op_var>()->var_id) {
        return false;
      }
      break;
    case op_instance_prop:
      if (lhs.as<op_instance_prop>()->var_id != rhs.as<op_instance_prop>()->var_id) {
        return false;
      }
      break;
    case op_int_const:
      if (lhs.as<op_int_const>()->str_val != rhs.as<op_int_const>()->str_val) {
        return false;
      }
      break;
    case op_index:
    case op_false:
    case op_true:
    case op_null:
      break;
    default:
      // unsupported compare
      return false;
  }
  return std::equal(lhs->begin(), lhs->end(), rhs->begin(), rhs->end(), is_same_var_expression);
}

VertexPtr get_first_arg_from_array_merge_call(VertexAdaptor<op_func_call> func_call) noexcept {
  return func_call && func_call->func_id->is_extern() && func_call->func_id->name == "array_merge" && !func_call->args().empty()
         ? remove_array_conv(func_call->args().front())
         : VertexPtr{};
}

std::string prepare_for_json(std::string str) noexcept {
  return vk::replace_all(TermStringFormat::remove_special_symbols(std::move(str)), "\\", "\\\\");
}

} // namespace

void AnalyzePerformance::analyze_func_call(VertexAdaptor<op_func_call> func_call) noexcept {
  if (is_enabled<PerformanceInspections::implicit_array_cast>()) {
    const auto &func_params = func_call->func_id->param_ids;
    VertexRange call_params = func_call->args();
    const size_t params_count = std::min(func_params.size(), static_cast<size_t>(call_params.size()));
    for (size_t i = 0; i < params_count; ++i) {
      const auto var_param_id = func_params[i];
      if (var_param_id->marked_as_const || var_param_id->is_read_only) {
        check_implicit_array_conversion(call_params[i], tinf::get_type(var_param_id));
      }
    }
  }
}

void AnalyzePerformance::analyze_set(VertexAdaptor<op_set> op_set_vertex) noexcept {
  if (is_enabled<PerformanceInspections::array_merge_into>()) {
    const auto first_arg = get_first_arg_from_array_merge_call(op_set_vertex->rhs().try_as<op_func_call>());
    if (is_same_var_expression(op_set_vertex->lhs(), first_arg)) {
      on_array_merge();
      return;
    }
  }
  if (is_enabled<PerformanceInspections::implicit_array_cast>()) {
    check_implicit_array_conversion(op_set_vertex->rhs(), tinf::get_type(op_set_vertex->lhs()));
  }
}

void AnalyzePerformance::analyze_set_array_value(VertexAdaptor<op_set_value> op_set_value_vertex) noexcept {
  if (is_enabled<PerformanceInspections::array_merge_into>()) {
    const auto first_arg = get_first_arg_from_array_merge_call(op_set_value_vertex->value().try_as<op_func_call>());
    auto first_arg_array_index = first_arg.try_as<op_index>();
    if (first_arg_array_index && first_arg_array_index->has_key() &&
        is_same_var_expression(op_set_value_vertex->key(), first_arg_array_index->key()) &&
        is_same_var_expression(op_set_value_vertex->array(), first_arg_array_index->array())) {
      on_array_merge();
      return;
    }
  }

  analyze_array_insertion(op_set_value_vertex);
}

template<Operation op>
void AnalyzePerformance::analyze_array_insertion(VertexAdaptor<op> vertex) noexcept {
  if (is_enabled<PerformanceInspections::implicit_array_cast>()) {
    const auto *to_type = tinf::get_type(vertex->array());
    if (to_type->get_real_ptype() == tp_array) {
      to_type = to_type->lookup_at(Key::any_key());
    }
    check_implicit_array_conversion(vertex->value(), to_type);
  }
}

void AnalyzePerformance::analyze_op_array(VertexAdaptor<op_array> op_array_vertex) noexcept {
  if (is_enabled<PerformanceInspections::implicit_array_cast>()) {
    const auto *to_type = tinf::get_type(op_array_vertex)->lookup_at(Key::any_key());
    for (auto array_element : *op_array_vertex) {
      if (vk::any_of_equal(array_element->type(), op_var, op_array)) {
        check_implicit_array_conversion(array_element, to_type);
      } else if (auto key_value_element = array_element.try_as<op_double_arrow>()) {
        check_implicit_array_conversion(key_value_element->value(), to_type);
      }
    }
  }
}

void AnalyzePerformance::analyze_op_return(VertexAdaptor<op_return> op_return_vertex) noexcept {
  if (is_enabled<PerformanceInspections::implicit_array_cast>() && op_return_vertex->has_expr()) {
    check_implicit_array_conversion(op_return_vertex->expr(), tinf::get_type(current_function, -1));
  }
}

void AnalyzePerformance::check_implicit_array_conversion(VertexPtr expr, const TypeData *to) noexcept {
  const auto *from = tinf::get_type(expr);
  if (is_implicit_array_conversion(from, to)) {
    std::string message = get_description_for_help(expr) + " is implicitly converted from " + colored_type_out(from) + " to " + colored_type_out(to) + "";
    check_and_fire_warning(PerformanceInspections::implicit_array_cast, message);
    check_and_save_performance_issue_for_analyse(PerformanceInspections::implicit_array_cast, std::move(message));
  }
}

void AnalyzePerformance::on_array_merge() noexcept {
  const char *message = "function array_merge can be replaced with array_merge_into";
  check_and_fire_warning(PerformanceInspections::array_merge_into, message);
  check_and_save_performance_issue_for_analyse(PerformanceInspections::array_merge_into, message);
}

void AnalyzePerformance::check_and_fire_warning(PerformanceInspections::Inspections inspection, vk::string_view message) noexcept {
  if (!(current_function->performance_inspections_for_warning.inspections() & inspection)) {
    return;
  }
  if (!warnings_enabled_) {
    constexpr uint64_t warnings_threshold = 3;
    static std::atomic<uint64_t> warnings_fired{0};
    warnings_enabled_ = warnings_fired++ < warnings_threshold;
  }
  if (warnings_enabled_) {
    auto chain = current_function->get_performance_inspections_warning_chain(inspection);
    kphp_warning(fmt_format("{}\nPerformance inspection '{}' enabled by: {}", message, PerformanceInspections::inspection2string(inspection), chain));
  }
}

void AnalyzePerformance::check_and_save_performance_issue_for_analyse(PerformanceInspections::Inspections inspection, std::string message) noexcept {
  if (current_function->performance_inspections_for_analyse.inspections() & inspection) {
    performance_issues_for_analyse_.emplace_back(Issue{stage::get_line(), inspection, prepare_for_json(std::move(message))});
  }
}

bool AnalyzePerformance::check_function(FunctionPtr func) const {
  return get_function_inspections(func);
}

VertexPtr AnalyzePerformance::on_enter_vertex(VertexPtr vertex) {
  switch (vertex->type()) {
    case op_func_call:
      analyze_func_call(vertex.as<op_func_call>());
      break;
    case op_set:
      analyze_set(vertex.as<op_set>());
      break;
    case op_set_value:
      analyze_set_array_value(vertex.as<op_set_value>());
      break;
    case op_push_back:
    case op_push_back_return:
      analyze_array_insertion(vertex.as<meta_op_push_back>());
      break;
    case op_array:
      analyze_op_array(vertex.as<op_array>());
      break;
    case op_return:
      analyze_op_return(vertex.as<op_return>());
      break;
    default:
      break;
  }
  return vertex;
}

void AnalyzePerformance::on_finish() {
  PerformanceIssuesReport::get().add_issues(current_function, std::move(performance_issues_for_analyse_));
}

void PerformanceIssuesReport::add_issues(FunctionPtr function, std::vector<AnalyzePerformance::Issue> &&issues) noexcept {
  if (issues.empty()) {
    return;
  }
  FunctionPerformanceIssues function_issues{
    static_cast<std::string>(G->get_base_relative_filename(function->file_id)),
    prepare_for_json(function->get_human_readable_name(false)),
    std::move(issues)
  };
  std::sort(function_issues.issues.begin(), function_issues.issues.end(),
            [](const AnalyzePerformance::Issue &lhs, const AnalyzePerformance::Issue &rhs) {
              return std::tie(lhs.line, lhs.inspection, lhs.description) <
                     std::tie(rhs.line, rhs.inspection, rhs.description);
            });
  std::lock_guard<std::mutex> lock{mutex_};
  report_.emplace_back(std::move(function_issues));
}

bool PerformanceIssuesReport::empty() noexcept {
  std::lock_guard<std::mutex> lock{mutex_};
  return report_.empty();
}

void PerformanceIssuesReport::flush_to(FILE *out) noexcept {
  struct IssueDetail {
    std::string file_name;
    std::string function_name;
    AnalyzePerformance::Issue issue;

    auto to_tuple() const noexcept {
      return std::tie(file_name, issue.line, function_name, issue.inspection, issue.description);
    }

    bool operator<(const IssueDetail &rhs) const noexcept {
      return to_tuple() < rhs.to_tuple();
    }

    bool operator==(const IssueDetail &rhs) const noexcept {
      return to_tuple() == rhs.to_tuple();
    }
  };

  std::vector<IssueDetail> full_report;
  {
    std::lock_guard<std::mutex> lock{mutex_};
    for (FunctionPerformanceIssues &fun_issues : report_) {
      for (AnalyzePerformance::Issue &issue : fun_issues.issues) {
        full_report.emplace_back(IssueDetail{fun_issues.file_name, fun_issues.function_name, std::move(issue)});
      }
    }
    report_.clear();
  }

  std::sort(full_report.begin(), full_report.end());
  full_report.erase(std::unique(full_report.begin(), full_report.end()), full_report.end());

  fprintf(out, "[");
  const char *line_begin = "\n";
  for (const auto &record : full_report) {
    fmt_fprintf(out, R"json({}{{"location": "{}:{}", "function": "{}", "inspection": "{}", "description": "{}"}})json",
                line_begin, record.file_name, record.issue.line, record.function_name,
                PerformanceInspections::inspection2string(record.issue.inspection), record.issue.description);
    line_begin = ",\n";
  }
  fprintf(out, "\n]\n");
}
