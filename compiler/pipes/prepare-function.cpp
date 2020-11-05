// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/prepare-function.h"

#include <sstream>
#include <unordered_map>

#include "common/algorithms/contains.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/lambda-class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/gentree.h"
#include "compiler/phpdoc.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"

// Inspects @kphp-infer, @kphp-inline and other @kphp-* inside the function phpdoc.
// Note that @kphp-required is analyzed inside gentree.
class ParseAndApplyPHPDoc {
private:
  using infer_mask = FunctionData::InferHint::infer_mask;

public:
  ParseAndApplyPHPDoc(FunctionPtr f) :
    f_(f),
    func_params_(f->get_params()) {
    std::tie(phpdoc_str_, phpdoc_from_fun_) = get_inherited_phpdoc();
  }

  void apply() {
    stage::set_location(f_->root->get_location());
    const auto &phpdoc_tags = parse_php_doc(phpdoc_str_);

    generate_name_to_param_id();
    apply_arguments_type_hints();   // type hints are applied always, even if @param/@return are not
    apply_return_type_hint();

    // first, search for @kphp-... tags
    // @param/@return will be scanned later, as @kphp- annotations can affect their behavior
    bool analyze_kphp_tags = vk::contains(phpdoc_str_, "@kphp");

    if (analyze_kphp_tags) {
      for (const auto &tag : phpdoc_tags) {
        stage::set_line(tag.line_num);
        parse_kphp_doc_tag(tag);
      }
    }

    // do we analyze @param / @return? typically, yes, but...
    bool analyze_param_return =
      f_->type == FunctionData::func_local &&   // don't parse @param for extern functions (@tl classes might not exist, it's ok)
      !f_->is_template;                         // template functions can have anything in these tags, don't take them into account

    if (analyze_param_return) {
      for (const auto &tag : phpdoc_tags) {
        stage::set_line(tag.line_num);

        if (tag.type == php_doc_tag::returns) {
          parse_return_doc_tag(tag);
        } else if (tag.type == php_doc_tag::param) {
          parse_param_doc_tag(tag);
        }
      }
    }

    // does the user need to specify @param for all and @return unless void?
    bool needs_to_be_fully_typed =
      // if KPHP_REQUIRE_FUNCTIONS_TYPING = 0, only check types if given, typing everything is not mandatory
      G->settings().require_functions_typing.get() &&
      // if 1, typing is mandatory, except these conditions
      analyze_param_return &&                     // if we don't take @param into account at all — don't require typing of course
      !f_->is_lambda() &&                         // lambda functions don't require strict typing (it's inconvenient)
      !f_->disabled_warnings.count("return");     // "@kphp-disable-warnings return" in function phpdoc

    if (needs_to_be_fully_typed) {
      check_all_arguments_have_param_or_type_hint();
      check_function_has_return_or_type_hint();
    }
  }

private:
  std::pair<vk::string_view, FunctionPtr> get_inherited_phpdoc() {
    bool is_phpdoc_inherited = f_->is_overridden_method || (f_->modifiers.is_static() && f_->class_id->parent_class);
    // inherit phpDoc only if it's not overridden in derived class
    is_phpdoc_inherited &= f_->phpdoc_str.empty() ||
                           (!vk::contains(f_->phpdoc_str, "@param") && !vk::contains(f_->phpdoc_str, "@return"));

    if (!is_phpdoc_inherited) {
      return {f_->phpdoc_str, f_};
    }
    for (const auto &ancestor : f_->class_id->get_all_ancestors()) {
      if (ancestor == f_->class_id) {
        continue;
      }

      if (f_->modifiers.is_instance()) {
        if (auto method = ancestor->members.get_instance_method(f_->local_name())) {
          return {method->function->phpdoc_str, method->function};
        }
      } else if (auto method = ancestor->members.get_static_method(f_->local_name())) {
        return {method->function->phpdoc_str, method->function};
      }
    }

    return {f_->phpdoc_str, f_};
  }

  void generate_name_to_param_id() {
    for (int param_i = f_->has_implicit_this_arg(); param_i < func_params_.size(); ++param_i) {
      auto op_func_param = func_params_[param_i].as<meta_op_func_param>();
      name_to_function_param_[op_func_param->var()->str_val] = param_i;
    }
  }

  void apply_arguments_type_hints() {
    for (int param_i = 0; param_i < func_params_.size(); ++param_i) {
      auto op_func_param = func_params_[param_i].as<meta_op_func_param>();

      if (op_func_param->type_declaration == "callable") {
        op_func_param->is_callable = true;
        op_func_param->template_type_id = id_of_kphp_template_++;
        op_func_param->type_declaration.clear();
        f_->is_template = true;
      } else if (!op_func_param->type_declaration.empty()) {
        auto parsed = phpdoc_parse_type_and_var_name(op_func_param->type_declaration, f_);
        f_->add_kphp_infer_hint(infer_mask::hint_check, param_i, parsed.type_expr);
      }
    }
  }

  void apply_return_type_hint() {
    if (!f_->return_typehint.empty()) {
      auto parsed = phpdoc_parse_type_and_var_name(f_->return_typehint, f_);
      f_->add_kphp_infer_hint(infer_mask::hint_check, -1, parsed.type_expr);
    }
  }

  void parse_kphp_doc_tag(const php_doc_tag &tag) {
    switch (tag.type) {
      case php_doc_tag::kphp_inline: {
        f_->is_inline = true;
        break;
      }

      case php_doc_tag::kphp_profile: {
        f_->profiler_state = FunctionData::profiler_status::enable_as_root;
        break;
      }
      case php_doc_tag::kphp_profile_allow_inline: {
        if (f_->profiler_state == FunctionData::profiler_status::disable) {
          f_->profiler_state = FunctionData::profiler_status::enable_as_inline_child;
        } else {
          kphp_assert(f_->profiler_state == FunctionData::profiler_status::enable_as_root);
        }
        break;
      }

      case php_doc_tag::kphp_sync: {
        f_->should_be_sync = true;
        break;
      }

      case php_doc_tag::kphp_should_not_throw: {
        f_->should_not_throw = true;
        break;
      }

      case php_doc_tag::kphp_infer: {
        if (tag.value.find("cast") != std::string::npos) {
          infer_cast_ = true;
        }
        break;
      }

      case php_doc_tag::kphp_warn_unused_result: {
        f_->warn_unused_result = true;
        break;
      }

        case php_doc_tag::kphp_disable_warnings: {
          std::istringstream is(tag.value);
          std::string token;
          while (is >> token) {
            if (!f_->disabled_warnings.insert(token).second) {
              kphp_warning(fmt_format("Warning '{}' has been disabled twice", token));
            }
          }
          break;
        }

      case php_doc_tag::kphp_extern_func_info: {
        kphp_error(f_->is_extern(), "@kphp-extern-func-info used for regular function");
        std::istringstream is(tag.value);
        std::string token;
        while (is >> token) {
          if (token == "can_throw") {
            f_->can_throw = true;
          } else if (token == "resumable") {
            f_->is_resumable = true;
          } else if (token == "cpp_template_call") {
            f_->cpp_template_call = true;
          } else if (token == "cpp_variadic_call") {
            f_->cpp_variadic_call = true;
          } else if (token == "tl_common_h_dep") {
            f_->tl_common_h_dep = true;
          } else {
            kphp_error(0, fmt_format("Unknown @kphp-extern-func-info {}", token));
          }
        }
        break;
      }

      case php_doc_tag::kphp_pure_function: {
        kphp_error(f_->is_extern(), "@kphp-pure-function is supported only for built-in functions");
        if (f_->root->type_rule) {
          f_->root->type_rule->rule()->extra_type = op_ex_rule_const;
        }
        break;
      }

      case php_doc_tag::kphp_noreturn: {
        f_->is_no_return = true;
        break;
      }

      case php_doc_tag::kphp_lib_export: {
        f_->kphp_lib_export = true;
        break;
      }

      case php_doc_tag::kphp_template: {
        f_->is_template = true;
        bool is_first_time = true;
        for (const auto &var_name : split_skipping_delimeters(tag.value, ", ")) {
          if (var_name[0] != '$') {
            if (is_first_time) {
              is_first_time = false;
              continue;
            }
            break;
          }
          is_first_time = false;

          auto func_param_it = name_to_function_param_.find(var_name.substr(1));
          kphp_error_return(func_param_it != name_to_function_param_.end(), fmt_format("@kphp-template tag var name mismatch. found {}.", var_name));

          auto cur_func_param = func_params_[func_param_it->second].as<op_func_param>();
          name_to_function_param_.erase(func_param_it);

          cur_func_param->template_type_id = id_of_kphp_template_;
        }
        id_of_kphp_template_++;

        break;
      }

      case php_doc_tag::kphp_const: {
        for (const auto &var_name : split_skipping_delimeters(tag.value, ", ")) {
          auto func_param_it = name_to_function_param_.find(var_name.substr(1));
          kphp_error_return(func_param_it != name_to_function_param_.end(), fmt_format("@kphp-const tag var name mismatch. found {}.", var_name));

          auto cur_func_param = func_params_[func_param_it->second].as<op_func_param>();
          cur_func_param->var()->is_const = true;
        }

        break;
      }

      case php_doc_tag::kphp_flatten: {
        f_->is_flatten = true;
        break;
      }

      case php_doc_tag::kphp_warn_performance: {
        try {
          f_->performance_inspections_for_warning.add_from_php_doc(tag.value);
        } catch (const std::exception &ex) {
          kphp_error(false, fmt_format("@kphp-warn-performance bad tag: {}", ex.what()));
        }
        break;
      }
      case php_doc_tag::kphp_analyze_performance: {
        try {
          f_->performance_inspections_for_analyse.add_from_php_doc(tag.value);
        } catch (const std::exception &ex) {
          kphp_error(false, fmt_format("@kphp-analyze-performance bad tag: {}", ex.what()));
        }
        break;
      }

      default:
        break;
    }
  }

  void parse_return_doc_tag(const php_doc_tag &tag) {
    PhpDocTagParseResult doc_parsed = phpdoc_parse_type_and_var_name(tag.value, phpdoc_from_fun_);
    if (!doc_parsed) {    // an error has already been printed
      return;
    }

    // todo better to add hint_check here, but vk.com has lots of errors, maybe will be fixed later
    f_->add_kphp_infer_hint(infer_mask::check, -1, doc_parsed.type_expr);
    has_return_php_doc_ = true;
  }

  void parse_param_doc_tag(const php_doc_tag &tag) {
    PhpDocTagParseResult doc_parsed = phpdoc_parse_type_and_var_name(tag.value, phpdoc_from_fun_);
    if (!doc_parsed) {    // an error has already been printed
      return;
    }

    auto func_param_it = name_to_function_param_.find(doc_parsed.var_name);
    kphp_error_return(func_param_it != name_to_function_param_.end(),
                      fmt_format("@param for unexisting argument ${}", doc_parsed.var_name));
    int param_i = func_param_it->second;

    auto cur_func_param = func_params_[func_param_it->second].as<op_func_param>();
    name_to_function_param_.erase(func_param_it);

    // if phpdoc mentions "callable" inside this @param
    if (auto type_callable = doc_parsed.type_expr.try_as<op_type_expr_callable>()) {
      cur_func_param->is_callable = type_callable->has_params();
      // we will generate common interface for typed callables later
      if (!cur_func_param->is_callable) {
        f_->is_template = true;
        cur_func_param->template_type_id = id_of_kphp_template_;
        cur_func_param->is_callable = true;
        id_of_kphp_template_++;
        return;
      }
    }

    f_->add_kphp_infer_hint(infer_mask::hint_check, param_i, doc_parsed.type_expr);

    if (infer_cast_) {
      auto expr_type_v = doc_parsed.type_expr.try_as<op_type_expr_type>();
      kphp_error(expr_type_v && expr_type_v->args().empty(), "Too hard rule for cast");
      kphp_error(cur_func_param->type_help == tp_Unknown, fmt_format("Duplicate type rule for argument '{}'", doc_parsed.var_name));

      cur_func_param->type_help = doc_parsed.type_expr.as<op_type_expr_type>()->type_help;
    }
  }

  void check_all_arguments_have_param_or_type_hint() {
    // at this point, all phpdocs have been parsed, and name_to_function_param_ contains arguments that have no @param tag
    // show error if they don't have type hint either
    // missing type hint and phpdoc for one of the parameters
    for (auto name_and_param_id : name_to_function_param_) {
      auto op_func_param = func_params_[name_and_param_id.second].as<meta_op_func_param>();

      if (op_func_param->type_declaration.empty()) {
        kphp_error(false, fmt_format("Specify @param or type hint for ${}", name_and_param_id.first));
      }
    }
  }

  void check_function_has_return_or_type_hint() {
    // at this point, all phpdocs have been parsed, and has_return_php_doc_ is true if @return found
    // if we have no @return and no return hint, assume @return void
    if (f_->return_typehint.empty() && !has_return_php_doc_) {
      bool assume_return_void = !f_->is_constructor() && !f_->assumption_for_return;

      if (assume_return_void) {
        f_->add_kphp_infer_hint(infer_mask::check, -1, GenTree::create_type_help_vertex(tp_void));
      }
    }
  }


private:
  FunctionPtr f_;
  bool infer_cast_{false};
  vk::string_view phpdoc_str_;
  FunctionPtr phpdoc_from_fun_;
  VertexRange func_params_;
  std::size_t id_of_kphp_template_{0};
  std::unordered_map<vk::string_view, int> name_to_function_param_;
  bool has_return_php_doc_{false};
};

static void check_default_args(FunctionPtr fun) {
  bool was_default = false;

  auto params = fun->get_params();
  for (size_t i = 0; i < params.size(); i++) {
    auto param = params[i].as<meta_op_func_param>();

    if (param->has_default_value() && param->default_value()) {
      was_default = true;
      if (fun->type == FunctionData::func_local) {
        kphp_error (!param->var()->ref_flag, fmt_format("Default value in reference function argument [function = {}]", fun->get_human_readable_name()));
      }
    } else {
      kphp_error (!was_default, fmt_format("Default value expected [function = {}] [param_i = {}]", fun->get_human_readable_name(), i));
    }
  }
}

void PrepareFunctionF::execute(FunctionPtr function, DataStream<FunctionPtr> &os) {
  stage::set_name("Prepare function");
  stage::set_function(function);
  kphp_assert (function);

  ParseAndApplyPHPDoc(function).apply();
  check_default_args(function);

  auto is_function_name_allowed = [](vk::string_view name) {
    if (!name.starts_with("__")) {
      return true;
    }

    static std::string allowed_magic_names[]{ClassData::NAME_OF_CONSTRUCT,
                                             ClassData::NAME_OF_CLONE,
                                             FunctionData::get_name_of_self_method(ClassData::NAME_OF_CLONE),
                                             ClassData::NAME_OF_VIRT_CLONE,
                                             FunctionData::get_name_of_self_method(ClassData::NAME_OF_VIRT_CLONE),
                                             ClassData::NAME_OF_INVOKE_METHOD};

    return vk::contains(allowed_magic_names, name);
  };

  kphp_error(!function->class_id || is_function_name_allowed(function->local_name()),
             fmt_format("KPHP doesn't support magic method: {}", function->get_human_readable_name()));

  if (stage::has_error()) {
    return;
  }

  os << function;
}
