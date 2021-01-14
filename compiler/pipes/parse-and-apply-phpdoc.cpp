// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/parse-and-apply-phpdoc.h"

#include <sstream>

#include "common/algorithms/contains.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/gentree.h"
#include "compiler/phpdoc.h"
#include "compiler/type-hint.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"

// Inspects @kphp-tags in phpdoc above a function (note that @kphp-required is analyzed in gentree)
// Also converts @param/@return to type hints and marks a function as a template on @kphp-template and untyped callables
class ParseAndApplyPhpDocForFunction {
  FunctionPtr f_;
  bool infer_cast_{false};
  FunctionPtr phpdoc_from_fun_;
  VertexRange func_params_;
  std::size_t id_of_kphp_template_{0};

public:
  explicit ParseAndApplyPhpDocForFunction(FunctionPtr f) : f_(f), func_params_(f->get_params()) {

    phpdoc_from_fun_ = get_phpdoc_from_fun_considering_inheritance();
    stage::set_location(phpdoc_from_fun_->root->get_location());

    const auto &phpdoc_tags = parse_php_doc(phpdoc_from_fun_->phpdoc_str);

    // first, search for @kphp-... tags
    // @param/@return will be scanned later, as @kphp- annotations can affect their behavior
    if (vk::contains(phpdoc_from_fun_->phpdoc_str, "@kphp")) {
      for (const auto &tag : phpdoc_tags) {
        stage::set_line(tag.line_num);
        parse_kphp_doc_tag(tag);
      }
    }

    // at first, parse @param tags: do it before @return
    for (const auto &tag : phpdoc_tags) {
      if (tag.type == php_doc_tag::param) {
        stage::set_line(tag.line_num);
        parse_param_doc_tag(tag);
      }
    }

    // 'callable' or typed callable params act like @kphp-template
    convert_to_template_function_if_callable_arg();

    // now, parse @return tags
    for (const auto &tag : phpdoc_tags) {
      if (tag.type == php_doc_tag::returns && f_->type == FunctionData::func_local) {  // @return @tl\... in functions.txt will be parsed only by assumptions
        stage::set_line(tag.line_num);
        parse_return_doc_tag(tag);
      }
    }

    // 1) check that all classes exists and are not traits in @param/@return
    // 2) if self/static/parent, resolve them
    if (f_->return_typehint) {
      f_->return_typehint = phpdoc_finalize_type_hint_and_resolve(f_->return_typehint, f_);
      kphp_error(f_->return_typehint, fmt_format("Failed to parse @return of {}", f_->get_human_readable_name()));
    }
    for (auto param : f_->get_params()) {
      if (param.as<op_func_param>()->type_hint) {
        param.as<op_func_param>()->type_hint = phpdoc_finalize_type_hint_and_resolve(param.as<op_func_param>()->type_hint, f_);
        kphp_error(param.as<op_func_param>()->type_hint, fmt_format("Failed to parse @param of {}", f_->get_human_readable_name()));
      }
    }

    // does the user need to specify @param for all and @return unless void?
    bool needs_to_be_fully_typed =
      // if KPHP_REQUIRE_FUNCTIONS_TYPING = 0, only check types if given, typing everything is not mandatory
      G->settings().require_functions_typing.get() &&
      // if 1, typing is mandatory, except these conditions
      f_->type == FunctionData::func_local &&
      !f_->is_lambda() &&                         // lambda functions don't require strict typing (it's inconvenient)
      !f_->disabled_warnings.count("return");     // "@kphp-disable-warnings return" in function phpdoc

    if (needs_to_be_fully_typed) {
      check_all_arguments_have_param_or_type_hint();
      check_function_has_return_or_type_hint();
    }
  }

private:
  FunctionPtr get_phpdoc_from_fun_considering_inheritance() {
    bool is_phpdoc_inherited = f_->is_overridden_method || (f_->modifiers.is_static() && f_->class_id->parent_class);
    // inherit phpDoc only if it's not overridden in derived class
    is_phpdoc_inherited &= f_->phpdoc_str.empty() ||
                           (!vk::contains(f_->phpdoc_str, "@param") && !vk::contains(f_->phpdoc_str, "@return"));

    if (!is_phpdoc_inherited) {
      return f_;
    }
    for (const auto &ancestor : f_->class_id->get_all_ancestors()) {
      if (ancestor == f_->class_id) {
        continue;
      }

      if (f_->modifiers.is_instance()) {
        if (auto method = ancestor->members.get_instance_method(f_->local_name())) {
          return method->function;
        }
      } else if (auto method = ancestor->members.get_static_method(f_->local_name())) {
        return method->function;
      }
    }

    return f_;
  }

  VertexAdaptor<op_func_param> find_param_by_name(vk::string_view name) {
    for (auto v : func_params_) {
      if (auto param = v.try_as<op_func_param>()) {
        if (param->var()->get_string() == name) {
          return param;
        }
      }
    }
    return {};
  }

  void convert_to_template_function_if_callable_arg() {
    for (auto p : func_params_) {
      auto cur_func_param = p.as<op_func_param>();
      if (const auto *callable = cur_func_param->type_hint ? cur_func_param->type_hint->try_as<TypeHintCallable>() : nullptr) {
        if (callable->is_typed_callable()) {    // we will generate common interface for typed callables later
          cur_func_param->is_callable = true;
        } else {                                // untyped callables is like a @kphp-template parameter
          cur_func_param->template_type_id = id_of_kphp_template_++;
          cur_func_param->is_callable = true;
          f_->is_template = true;
        }
      }
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
            // since we don't know which exceptions it can throw, mark is as \Exception
            f_->exceptions_thrown.insert(G->get_class("Exception"));
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
        f_->is_pure = true;
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

          auto param = find_param_by_name(var_name.substr(1));
          kphp_error_return(param, fmt_format("@kphp-template tag var name mismatch. found {}.", var_name));
          param->template_type_id = id_of_kphp_template_;
        }
        id_of_kphp_template_++;

        break;
      }

      case php_doc_tag::kphp_const: {
        for (const auto &var_name : split_skipping_delimeters(tag.value, ", ")) {
          auto param = find_param_by_name(var_name.substr(1));
          kphp_error_return(param, fmt_format("@kphp-const tag var name mismatch. found {}.", var_name));
          param->var()->is_const = true;
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
          f_->performance_inspections_for_analysis.add_from_php_doc(tag.value);
        } catch (const std::exception &ex) {
          kphp_error(false, fmt_format("@kphp-analyze-performance bad tag: {}", ex.what()));
        }
        break;
      }

      case php_doc_tag::kphp_throws: {
        std::istringstream is(tag.value);
        std::string klass;
        while (is >> klass) {
          f_->check_throws.emplace_front(klass); // the reversed order doesn't matter
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

    // if @return exists, it overrides a return type hint in code (unless a @return is from a parent phpdoc)
    if (f_->return_typehint && phpdoc_from_fun_ != f_) {
      return;
    }
    // @param for a template parameter can contain anything, skip (note: only @return in phpdoc! as a type hint, it remains)
    if (f_->is_template) {
      return;
    }
    f_->return_typehint = doc_parsed.type_hint;
  }

  void parse_param_doc_tag(const php_doc_tag &tag) {
    PhpDocTagParseResult doc_parsed = phpdoc_parse_type_and_var_name(tag.value, phpdoc_from_fun_);
    if (!doc_parsed) {    // an error has already been printed
      return;
    }

    auto param = find_param_by_name(doc_parsed.var_name);
    kphp_error_return(param, fmt_format("@param for unexisting argument ${}", doc_parsed.var_name));

    // if @param exists, it overrides an argument type hint in code (unless a @param is from a parent phpdoc)
    if (param->type_hint && phpdoc_from_fun_ != f_) {
      return;
    }
    // @param for a template parameter can contain anything, skip (but callable + @param typed callable is ok)
    if (param->template_type_id != -1) {
      return;
    }
    param->type_hint = doc_parsed.type_hint;

    if (infer_cast_) {
      auto expr_type_v = doc_parsed.type_hint->try_as<TypeHintPrimitive>();
      kphp_error(expr_type_v, "Too hard rule for cast");
      kphp_error(!param->is_cast_param, fmt_format("Duplicate type cast for argument '{}'", doc_parsed.var_name));
      param->is_cast_param = true;
    }
  }

  void check_all_arguments_have_param_or_type_hint() {
    // at this point, all phpdocs and type hints have been parsed
    // show error if any param doesn't have type declared type
    // missing type hint and phpdoc for one of the parameters
    for (auto v : func_params_) {
      if (auto param = v.try_as<op_func_param>()) {
        bool ok = param->type_hint ||
                  param->var()->extra_type == op_ex_var_this ||
                  param->template_type_id != -1;
        kphp_error(ok, fmt_format("Specify @param or type hint for ${}", param->var()->get_string()));
      }
    }
  }

  void check_function_has_return_or_type_hint() {
    // at this point, all phpdocs and type hints have been parsed
    // if we have no @return and no return hint, assume @return void
    if (!f_->return_typehint) {
      bool assume_return_void = !f_->is_constructor() && !f_->assumption_for_return && !f_->is_template;

      if (assume_return_void) {
        f_->return_typehint = TypeHintPrimitive::create(tp_void);
      }
    }
  }
};

// Inspects @var above all fields and converts them to type hints
// Also analyzes @kphp-serializable and other tags above the class itself
class ParseAndApplyPhpDocForClass {
  ClassPtr klass;

public:
  explicit ParseAndApplyPhpDocForClass(ClassPtr klass) : klass(klass) {

    // if there is @var / type hint near class field — save it; otherwise, convert the field default value to a type hint
    // note: it's safe to use init_val here (even if it refers to constants of other classes): defines were inlined at previous pipe
    klass->members.for_each([&](ClassMemberInstanceField &f) {
      f.type_hint = calculate_field_type_hint(f.phpdoc_str, f.var, klass);
      if (f.type_hint) {
        f.type_hint = phpdoc_finalize_type_hint_and_resolve(f.type_hint, klass->get_holder_function());
        kphp_error(f.type_hint, fmt_format("Failed to parse @var of {}", f.var->get_human_readable_name()));
      }
    });
    klass->members.for_each([&](ClassMemberStaticField &f) {
      f.type_hint = calculate_field_type_hint(f.phpdoc_str, f.var, klass);
      if (f.type_hint) {
        f.type_hint = phpdoc_finalize_type_hint_and_resolve(f.type_hint, klass->get_holder_function());
        kphp_error(f.type_hint, fmt_format("Failed to parse @var of {}", f.var->get_human_readable_name()));
      }
    });

    const auto &phpdoc_tags = parse_php_doc(klass->phpdoc_str);

    // now apply @kphp-serializable and so on
    for (const auto &tag : phpdoc_tags) {
      parse_kphp_doc_tag(tag);
    }

  }

private:
  // calculate type_hint for a field using all rules
  // returns TypeHint or nullptr
  static const TypeHint *calculate_field_type_hint(vk::string_view phpdoc_str, VarPtr var, ClassPtr klass) {
    // if there is a /** @var int|false */ comment above the class field declaration
    if (auto tag_phpdoc = phpdoc_find_tag_as_string(phpdoc_str, php_doc_tag::var)) {
      auto parsed = phpdoc_parse_type_and_var_name(*tag_phpdoc, stage::get_function());
      if (!kphp_error(parsed, fmt_format("Failed to parse phpdoc of {}", var->get_human_readable_name()))) {
        return parsed.type_hint;
      }
    }

    // if there is no /** @var ... */ but class typing is mandatory, use the field initializer expression to express field type
    // public $a = 0; - $a is int
    // public $a = []; - $a is any[]
    if (!G->settings().require_class_typing.get()) {
      return nullptr;
    }
    if (var->init_val) {
      const TypeHint *type_hint = phpdoc_convert_default_value_to_type_hint(var->init_val);
      kphp_error(type_hint, fmt_format("Specify @var to {}", var->get_human_readable_name()));
      return type_hint;
    }

    kphp_error(klass->is_lambda(),
               fmt_format("Specify @var or default value to {}", var->get_human_readable_name()));
    return nullptr;
  }

  void parse_kphp_doc_tag(const php_doc_tag &tag) {
    switch (tag.type) {
      case php_doc_tag::kphp_serializable:
        klass->is_serializable = true;
        kphp_error(klass->is_class(), "@kphp-serialize is allowed only for classes");
        break;

      case php_doc_tag::kphp_tl_class:
        klass->is_tl_class = true;
        break;

      default:
        break;
    }
  }
};

void ParseAndApplyPhpdocF::execute(FunctionPtr function, DataStream<FunctionPtr> &os) {
  stage::set_name("Apply phpdocs");
  stage::set_function(function);
  kphp_assert (function);

  if (function->type == FunctionData::func_class_holder) {
    ParseAndApplyPhpDocForClass{function->class_id};
  } else {
    ParseAndApplyPhpDocForFunction{function};
  }

  if (stage::has_error()) {
    return;
  }

  os << function;
}
