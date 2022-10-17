// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt
#include "compiler/pipes/analyze-performance.h"

#include "common/algorithms/sorting.h"
#include "common/algorithms/string-algorithms.h"
#include "common/termformat/termformat.h"
#include "common/wrappers/to_array.h"

#include "compiler/compiler-core.h"
#include "compiler/data/src-file.h"
#include "compiler/inferring/public.h"
#include "compiler/vertex-util.h"

namespace {

VertexPtr remove_conv_wrap(VertexPtr vertex) noexcept {
  if (OpInfo::type(vertex->type()) == conv_op) {
    return remove_conv_wrap(vertex.as<meta_op_unary>()->expr());
  }
  if (auto func_call = vertex.try_as<op_func_call>()) {
    if (func_call->str_val == "make_clone") {
      kphp_assert(func_call->args().size() == 1);
      return remove_conv_wrap(func_call->args().front());
    }
  }
  return vertex;
}

bool is_pure_nary_operation(VertexPtr vertex) noexcept {
  return vk::any_of_equal(vertex->type(), op_minus, op_plus, op_not,
                          op_add, op_mul, op_sub, op_div, op_mod, op_pow,
                          op_and, op_or, op_xor, op_shl, op_shr,
                          op_log_xor_let, op_log_and_let, op_log_or_let, op_log_and, op_log_or,
                          op_eq2, op_eq3, op_le, op_lt,
                          op_spaceship, op_null_coalesce, op_ternary, op_isset, op_string_build);
}

template<bool full>
std::string get_description_for_help_impl(VertexPtr expr);

std::string join_string_build_elements(VertexRange elements) noexcept {
  return vk::join(elements, ".", get_description_for_help_impl<false>);
}

template<bool full>
std::string get_description_for_help_impl(VertexPtr expr) {
  expr = remove_conv_wrap(expr);
  switch (expr->type()) {
    case op_var:
      if (const auto *constexpr_str = VertexUtil::get_constexpr_string(expr)) {
        std::string raw_str{constexpr_str->c_str(), std::min(constexpr_str->size(), size_t{36})};
        if (raw_str.size() < constexpr_str->size()) {
          raw_str.append("<...>");
        }
        std::replace_if(raw_str.begin(), raw_str.end(),
                        [](char c) {
                          return iscntrl(c) || vk::any_of_equal(c, '"', '\'', '\\');
                        }, '?');
        raw_str.erase(std::unique(raw_str.begin(), raw_str.end(),
                                  [](char lhs, char rhs) { return vk::any_of_equal(lhs, '?', ' ') && lhs == rhs; }),
                      raw_str.end());
        return "'" + vk::replace_all(raw_str, "?", "<...>") + "'";
      }
      return (full ? "variable " : "") + TermStringFormat::paint_green(expr.as<op_var>()->var_id->as_human_readable());
    case op_instance_prop:
      return (full ? "variable " : "") + TermStringFormat::paint_green(expr.as<op_instance_prop>()->var_id->as_human_readable());
    case op_func_call:
      return (full ? "function call " : "") + TermStringFormat::paint_green(expr.as<op_func_call>()->func_id->as_human_readable(false)) +
             "(" + vk::join(expr.as<op_func_call>()->args(), ", ", get_description_for_help_impl<false>) + ")";
    case op_index: {
      std::string key;
      auto op_index_vertex = expr.as<op_index>();
      if (op_index_vertex->has_key()) {
        key = get_description_for_help_impl<false>(op_index_vertex->key());
      }
      return (full ? "array element " : "") + get_description_for_help_impl<false>(op_index_vertex->array()) + "[" + key + "]";
    }
    case op_string_build:
      return join_string_build_elements(expr.as<op_string_build>()->args());
    case op_true:
      return "true";
    case op_false:
      return "false";
    case op_null:
      return "null";
    case op_int_const:
    case op_float_const:
      return expr.as<meta_op_num>()->str_val;
    default:
      // TODO add more cases
      return full ? "expresion <...>" : "<...>";
  }
}

std::string get_description_for_help(VertexPtr expr) noexcept {
  return get_description_for_help_impl<true>(expr);
}

bool is_same_var_expression(VertexPtr lhs, VertexPtr rhs) noexcept {
  if (!lhs || !rhs) {
    return false;
  }

  lhs = remove_conv_wrap(lhs);
  rhs = remove_conv_wrap(rhs);
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
    case op_float_const:
      if (lhs.as<meta_op_num>()->str_val != rhs.as<meta_op_num>()->str_val) {
        return false;
      }
      break;
    case op_index:
    case op_false:
    case op_true:
    case op_null:
      break;
    default:
      if (!is_pure_nary_operation(lhs)) {
        // unsupported compare
        return false;
      }
  }
  return std::equal(lhs->begin(), lhs->end(), rhs->begin(), rhs->end(), is_same_var_expression);
}

VertexPtr get_first_arg_from_builtin_call(VertexAdaptor<op_func_call> func_call, vk::string_view func_name) noexcept {
  return func_call && func_call->func_id->is_extern() && func_call->func_id->name == func_name && !func_call->args().empty()
         ? remove_conv_wrap(func_call->args().front())
         : VertexPtr{};
}

VertexPtr get_first_arg_from_array_merge_call(VertexAdaptor<op_func_call> func_call) noexcept {
  return get_first_arg_from_builtin_call(func_call, "array_merge");
}

constexpr auto get_reserve_function_names() noexcept {
  return vk::to_array(
    {
      "array_reserve",
      "array_reserve_vector",
      "array_reserve_map_int_keys",
      "array_reserve_map_string_keys",
      "array_reserve_from"
    });
}

VertexPtr get_first_arg_from_array_reserve_call(VertexAdaptor<op_func_call> func_call) noexcept {
  for (const auto *reserve_function : get_reserve_function_names()) {
    if (auto first_arg = get_first_arg_from_builtin_call(func_call, reserve_function)) {
      return first_arg;
    }
  }
  return {};
}

bool is_var_can_be_optimized_in_loop(VertexAdaptor<op_var> op_var_vertex) noexcept {
  return !op_var_vertex->var_id->is_in_global_scope() && !op_var_vertex->var_id->class_id;
}

bool is_op_index_sequence_with_non_int_keys(VertexPtr expr) noexcept {
  expr = remove_conv_wrap(expr);
  if (auto op_index_vertex = expr.try_as<op_index>()) {
    return !op_index_vertex->has_key() ||
           tinf::get_type(op_index_vertex->key())->get_real_ptype() != tp_int ||
           is_op_index_sequence_with_non_int_keys(op_index_vertex->array()) ||
           is_op_index_sequence_with_non_int_keys(op_index_vertex->key());
  }
  return vk::none_of_equal(expr->type(), op_var, op_int_const, op_float_const);
}

std::string prepare_for_json(std::string str) noexcept {
  return vk::replace_all(TermStringFormat::remove_special_symbols(std::move(str)), "\\", "\\\\");
}

} // namespace

void AnalyzePerformance::analyze_func_call(VertexAdaptor<op_func_call> func_call) noexcept {
  if (is_enabled<PerformanceInspections::implicit_array_cast>() && !func_call->func_id->is_extern()) {
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
  if (func_call->func_id->is_pure) {
    save_to_second_pass_analyze_on_loop_exit(func_call);
  }

  if (is_enabled<PerformanceInspections::array_reserve>()) {
    if (auto reserved_array_var = get_first_arg_from_array_reserve_call(func_call).try_as<op_var>()) {
      reserved_arrays_.emplace(reserved_array_var->var_id);
    }
  }
}

void AnalyzePerformance::analyze_set(VertexAdaptor<op_set> op_set_vertex) noexcept {
  if (is_enabled<PerformanceInspections::array_merge_into>()) {
    const auto first_arg = get_first_arg_from_array_merge_call(op_set_vertex->rhs().try_as<op_func_call>());
    if (is_same_var_expression(op_set_vertex->lhs(), first_arg)) {
      trigger_array_merge(first_arg);
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
      trigger_array_merge(first_arg);
    }
  }

  analyze_array_insertion(op_set_value_vertex);
}

template<Operation op>
void AnalyzePerformance::analyze_array_insertion(VertexAdaptor<op> vertex) noexcept {
  if (is_enabled<PerformanceInspections::implicit_array_cast>()) {
    const auto *to_type = tinf::get_type(vertex->array());
    if (to_type->get_real_ptype() == tp_array) {
      to_type = to_type->lookup_at_any_key();
    }
    check_implicit_array_conversion(vertex->value(), to_type);
  }

  save_to_second_pass_analyze_on_loop_exit(vertex);
}

void AnalyzePerformance::analyze_op_array(VertexAdaptor<op_array> op_array_vertex) noexcept {
  if (is_enabled<PerformanceInspections::implicit_array_cast>()) {
    const auto *to_type = tinf::get_type(op_array_vertex)->lookup_at_any_key();
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
  if (!loop_data_for_second_pass_.empty()) {
    loop_data_for_second_pass_.back().has_return = true;
  }
}

void AnalyzePerformance::analyze_op_var(VertexAdaptor<op_var> op_var_vertex) noexcept {
  if (!loop_data_for_second_pass_.empty() && is_var_can_be_optimized_in_loop(op_var_vertex) && op_var_vertex->rl_type != RLValueType::val_r) {
    loop_data_for_second_pass_.back().first_pass_modified_vars.emplace(op_var_vertex->var_id);
  }
}

void AnalyzePerformance::check_implicit_array_conversion(VertexPtr expr, const TypeData *to) noexcept {
  const auto *from = tinf::get_type(expr);
  if (is_implicit_array_conversion(from, to)) {
    trigger_inspection(PerformanceInspections::implicit_array_cast,
                       get_description_for_help(expr) + " is implicitly converted from " + from->as_human_readable() + " to " + to->as_human_readable());
  }
}

void AnalyzePerformance::save_to_second_pass_analyze_on_loop_exit(VertexPtr vertex) noexcept {
  if (!loop_data_for_second_pass_.empty() && !loop_data_for_second_pass_.back().condition_depth && !second_pass_saved_top_vertex_) {
    loop_data_for_second_pass_.back().second_pass_vertexes.emplace_back(vertex);
    second_pass_saved_top_vertex_ = vertex;
  }
}

void AnalyzePerformance::run_second_pass_on_loop_exit(VertexPtr vertex, uint64_t enabled_inspections, VertexPtr loop_vertex) noexcept {
  if (enabled_inspections & PerformanceInspections::constant_execution_in_loop) {
    if (vk::any_of_equal(vertex->type(), op_index, op_func_call)) {
      if (is_constant_expression_in_this_loop(vertex) && is_op_index_sequence_with_non_int_keys(vertex)) {
        auto message = get_description_for_help(vertex) + " can be saved in a separate variable out of loop";
        trigger_inspection_on_second_pass(vertex, PerformanceInspections::constant_execution_in_loop, std::move(message));
        enabled_inspections &= ~PerformanceInspections::constant_execution_in_loop;
      }
    }
    if (auto op_string_build_vertex = vertex.try_as<op_string_build>()) {
      auto first_it = std::adjacent_find(op_string_build_vertex->begin(), op_string_build_vertex->end(),
                                         [this](VertexPtr lhs, VertexPtr rhs) {
                                           return is_constant_expression_in_this_loop(lhs) && is_constant_expression_in_this_loop(rhs);
                                         });
      auto last_it = std::find_if_not(first_it, op_string_build_vertex->end(),
                                      [this](VertexPtr vertex) { return is_constant_expression_in_this_loop(vertex); });
      VertexRange constant_expressions_range{first_it, last_it};
      if (!constant_expressions_range.empty()) {
        auto message = "string building " + join_string_build_elements(constant_expressions_range) + " can be saved in a separate variable out of loop";
        trigger_inspection_on_second_pass(op_string_build_vertex, PerformanceInspections::constant_execution_in_loop, std::move(message));
        enabled_inspections &= ~PerformanceInspections::constant_execution_in_loop;
      }
    }
  }
  if (enabled_inspections & PerformanceInspections::array_reserve) {
    if (auto op_push_back_vertex = vertex.try_as<meta_op_push_back>()) {
      auto array_var = remove_conv_wrap(op_push_back_vertex->array()).try_as<op_var>();
      if (array_var && !array_var->var_id->is_in_global_scope() && !reserved_arrays_.count(array_var->var_id)) {
        auto message = get_description_for_help(array_var) + " can be reserved with array_reserve functions family out of loop";
        trigger_inspection_on_second_pass(op_push_back_vertex, PerformanceInspections::array_reserve, std::move(message));
        enabled_inspections &= ~PerformanceInspections::array_reserve;
      }
    }
    auto op_set_value_vertex = vertex.try_as<op_set_value>();
    auto op_foreach_loop_exit = loop_vertex.try_as<op_foreach>();
    if (op_set_value_vertex && op_foreach_loop_exit && op_foreach_loop_exit->params()->has_key()) {
      auto array_var = remove_conv_wrap(op_set_value_vertex->array()).try_as<op_var>();
      auto array_set_key = remove_conv_wrap(op_set_value_vertex->key()).try_as<op_var>();
      auto foreach_key = op_foreach_loop_exit->params()->key();
      if (array_var && !array_var->var_id->is_in_global_scope() && !reserved_arrays_.count(array_var->var_id) &&
          array_set_key && array_set_key->var_id == foreach_key->var_id) {
        auto message = get_description_for_help(array_var) + " can be reserved with array_reserve functions family out of loop";
        trigger_inspection_on_second_pass(op_set_value_vertex, PerformanceInspections::array_reserve, std::move(message));
        enabled_inspections &= ~PerformanceInspections::array_reserve;
      }
    }
  }

  if (enabled_inspections & (PerformanceInspections::constant_execution_in_loop | PerformanceInspections::array_reserve)) {
    for (auto child : *vertex) {
      run_second_pass_on_loop_exit(child, enabled_inspections, loop_vertex);
    }
  }
}

void AnalyzePerformance::enter_loop() noexcept {
  if (is_enabled<PerformanceInspections::constant_execution_in_loop>() || is_enabled<PerformanceInspections::array_reserve>()) {
    loop_data_for_second_pass_.emplace_back();
  }
}

void AnalyzePerformance::exit_loop(VertexPtr loop_vertex) noexcept {
  if (!is_enabled<PerformanceInspections::constant_execution_in_loop>() && !is_enabled<PerformanceInspections::array_reserve>()) {
    return;
  }
  const size_t loop_depth = loop_data_for_second_pass_.size();
  kphp_assert(loop_depth);
  const auto &exiting_loop = loop_data_for_second_pass_.back();
  if (loop_depth > 1) {
    const auto &loop_exit_modified_vars = exiting_loop.first_pass_modified_vars;
    loop_data_for_second_pass_[loop_depth - 2].first_pass_modified_vars.insert(loop_exit_modified_vars.begin(), loop_exit_modified_vars.end());
    loop_data_for_second_pass_[loop_depth - 2].has_return |= exiting_loop.has_return;
  }

  kphp_assert(!exiting_loop.condition_depth);
  if (!exiting_loop.has_return && !exiting_loop.has_break_or_continue) {
    for (auto second_pass_vertex : exiting_loop.second_pass_vertexes) {
      run_second_pass_on_loop_exit(second_pass_vertex, get_function_inspections(current_function), loop_vertex);
    }
  }
  loop_data_for_second_pass_.pop_back();
}

void AnalyzePerformance::enter_conditional() noexcept {
  if (!loop_data_for_second_pass_.empty()) {
    ++loop_data_for_second_pass_.back().condition_depth;
  }
}

void AnalyzePerformance::exit_conditional() noexcept {
  if (!loop_data_for_second_pass_.empty()) {
    kphp_assert(loop_data_for_second_pass_.back().condition_depth);
    --loop_data_for_second_pass_.back().condition_depth;
  }
}

bool AnalyzePerformance::is_constant_expression_in_this_loop(VertexPtr vertex) const noexcept {
  if (loop_data_for_second_pass_.empty()) {
    return false;
  }
  vertex = remove_conv_wrap(vertex);
  if (vk::any_of_equal(vertex->type(), op_int_const, op_float_const, op_null, op_true, op_false)) {
    return true;
  }
  if (auto op_var_vertex = vertex.try_as<op_var>()) {
    return is_var_can_be_optimized_in_loop(op_var_vertex) && !loop_data_for_second_pass_.back().first_pass_modified_vars.count(op_var_vertex->var_id);
  }
  if (auto op_index_vertex = vertex.try_as<op_index>()) {
    return op_index_vertex->has_key() &&
           is_constant_expression_in_this_loop(op_index_vertex->key()) &&
           is_constant_expression_in_this_loop(op_index_vertex->array());
  }
  if (auto op_func_call_vertex = vertex.try_as<op_func_call>()) {
    if (op_func_call_vertex->func_id->is_pure) {
      return vk::all_of(op_func_call_vertex->args(), [this](VertexPtr v) { return is_constant_expression_in_this_loop(v); });
    }
    return false;
  }
  if (is_pure_nary_operation(vertex)) {
    return vk::all_of(*vertex, [this](VertexPtr v) { return is_constant_expression_in_this_loop(v); });
  }
  return false;
}

void AnalyzePerformance::trigger_array_merge(VertexPtr first_arg) noexcept {
  auto first_arg_help = get_description_for_help_impl<false>(first_arg);
  auto message = "expression " + first_arg_help + " = array_merge(" + first_arg_help + ", <...>) " +
                 "can be replaced with array_merge_into(" + first_arg_help + ", <...>)";
  trigger_inspection(PerformanceInspections::array_merge_into, std::move(message));
}

void AnalyzePerformance::trigger_inspection(PerformanceInspections::Inspections inspection, std::string message) noexcept {
  if (current_function->performance_inspections_for_warning.inspections() & inspection) {
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

  if (current_function->performance_inspections_for_analysis.inspections() & inspection) {
    performance_issues_for_analysis_.emplace_back(Issue{stage::get_line(), inspection, prepare_for_json(std::move(message))});
  }
}

void AnalyzePerformance::trigger_inspection_on_second_pass(VertexPtr inspected_vertex,
                                                           PerformanceInspections::Inspections inspection,
                                                           std::string message) noexcept {
  const auto origin_location = stage::get_location();
  stage::set_location(inspected_vertex->get_location());
  trigger_inspection(inspection, std::move(message));
  stage::set_location(origin_location);
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
    case op_while:
    case op_do:
      enter_loop();
      break;
    case op_string_build:
    case op_index:
      save_to_second_pass_analyze_on_loop_exit(vertex);
      break;
    case op_var:
      analyze_op_var(vertex.as<op_var>());
      break;
    case op_continue:
      // TODO think about switch break
    case op_break:
      if (!loop_data_for_second_pass_.empty()) {
        loop_data_for_second_pass_.back().has_break_or_continue = true;
      }
      break;
    default:
      break;
  }
  return vertex;
}

bool AnalyzePerformance::user_recursion(VertexPtr vertex) {
  if (auto for_loop_vertex = vertex.try_as<op_for>()) {
    run_function_pass(for_loop_vertex->pre_cond_ref(), this);
    enter_loop();
    run_function_pass(for_loop_vertex->cond(), this);
    run_function_pass(for_loop_vertex->post_cond_ref(), this);
    run_function_pass(for_loop_vertex->cmd_ref(), this);
    exit_loop(vertex);
    return true;
  }
  if (auto foreach_loop_vertex = vertex.try_as<op_foreach>()) {
    auto params = foreach_loop_vertex->params();
    run_function_pass(params->xs(), this);
    run_function_pass(params->temp_var(), this);
    enter_loop();
    run_function_pass(params->x_ref(), this);
    if (params->has_key()) {
      run_function_pass(params->key_ref(), this);
    }
    run_function_pass(foreach_loop_vertex->cmd_ref(), this);
    exit_loop(vertex);
    return true;
  }
  if (auto if_vertex = vertex.try_as<op_if>()) {
    run_function_pass(if_vertex->cond(), this);
    enter_conditional();
    run_function_pass(if_vertex->true_cmd_ref(), this);
    if (if_vertex->has_false_cmd()) {
      run_function_pass(if_vertex->false_cmd_ref(), this);
    }
    exit_conditional();
    return true;
  }
  if (auto op_ternary_vertex = vertex.try_as<op_ternary>()) {
    run_function_pass(op_ternary_vertex->cond(), this);
    enter_conditional();
    run_function_pass(op_ternary_vertex->true_expr(), this);
    run_function_pass(op_ternary_vertex->false_expr(), this);
    exit_conditional();
    return true;
  }
  if (auto op_case_vertex = vertex.try_as<op_switch>()) {
    run_function_pass(op_case_vertex->condition(), this);
    enter_conditional();
    run_function_pass(op_case_vertex->condition_on_switch_ref(), this);
    run_function_pass(op_case_vertex->matched_with_one_case_ref(), this);
    for (auto &case_vertex : op_case_vertex->cases()) {
      run_function_pass(case_vertex, this);
    }
    exit_conditional();
    return true;
  }
  if (vk::any_of_equal(vertex->type(), op_null_coalesce, op_log_and, op_log_and_let, op_log_or, op_log_or_let)) {
    auto bin_op_vertex = vertex.as<meta_op_binary>();
    run_function_pass(bin_op_vertex->lhs(), this);
    enter_conditional();
    run_function_pass(bin_op_vertex->rhs(), this);
    exit_conditional();
    return true;
  }
  return false;
}

VertexPtr AnalyzePerformance::on_exit_vertex(VertexPtr vertex) {
  if (vertex == second_pass_saved_top_vertex_) {
    kphp_assert(!loop_data_for_second_pass_.empty());
    kphp_assert(loop_data_for_second_pass_.back().second_pass_vertexes.back() == vertex);
    second_pass_saved_top_vertex_ = {};
  }
  switch (vertex->type()) {
    case op_while:
    case op_do:
      exit_loop(vertex);
      break;
    default:
      break;
  }
  return vertex;
}

void AnalyzePerformance::on_finish() {
  if (current_function->performance_inspections_for_analysis.inspections()) {
    vk::singleton<PerformanceIssuesReport>::get().add_issues_and_require_flush(current_function, std::move(performance_issues_for_analysis_));
  }
}

void PerformanceIssuesReport::add_issues_and_require_flush(FunctionPtr function, std::vector<AnalyzePerformance::Issue> &&issues) noexcept {
  is_flush_required_ = true;
  if (issues.empty()) {
    return;
  }
  FunctionPerformanceIssues function_issues{
    function->file_id->relative_file_name,
    prepare_for_json(function->as_human_readable(false)),
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

bool PerformanceIssuesReport::is_flush_required() const noexcept {
  return is_flush_required_;
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

  vk::sort_and_unique(full_report);
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
