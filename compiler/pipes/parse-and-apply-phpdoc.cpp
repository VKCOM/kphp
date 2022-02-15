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
      if (tag.type == php_doc_tag::returns) {
        stage::set_line(tag.line_num);
        parse_return_doc_tag(tag);
      }
    }

    // 1) check that all classes exists and are not traits in @param/@return
    // 2) if self/static/parent, resolve them
    if (f_->return_typehint) {
      f_->return_typehint = phpdoc_finalize_type_hint_and_resolve(f_->return_typehint, f_);
      kphp_error(f_->return_typehint, fmt_format("Failed to parse @return of {}", f_->as_human_readable()));
    }
    for (auto param : f_->get_params()) {
      if (param.as<op_func_param>()->type_hint) {
        param.as<op_func_param>()->type_hint = phpdoc_finalize_type_hint_and_resolve(param.as<op_func_param>()->type_hint, f_);
        kphp_error(param.as<op_func_param>()->type_hint, fmt_format("Failed to parse @param of {}", f_->as_human_readable()));
      }
    }

    // does the user need to specify @param for all and @return unless void?
    bool needs_to_be_fully_typed =
      // if KPHP_REQUIRE_FUNCTIONS_TYPING = 0, only check types if given, typing everything is not mandatory
      G->settings().require_functions_typing.get() &&
      // if 1, typing is mandatory, except these conditions
      f_->type == FunctionData::func_local &&
      !f_->is_lambda() &&                         // lambdas may have implicitly deduced types, they are going to be checked later
      !f_->disabled_warnings.count("return");     // "@kphp-disable-warnings return" in function phpdoc

    if (needs_to_be_fully_typed) {
      check_all_arguments_have_param_or_type_hint();
      check_function_has_return_or_type_hint();
    }
  }

private:
  bool is_phpdoc_meaningful(vk::string_view phpdoc_str) {
    if (phpdoc_str.empty()) {
      return false;
    }
    return vk::contains(phpdoc_str, "@param") || vk::contains(phpdoc_str, "@return");
  }

  FunctionPtr get_phpdoc_from_fun_considering_inheritance() {
    bool is_phpdoc_inherited = false;
    if (f_->is_overridden_method || (f_->modifiers.is_static() && f_->class_id->parent_class)) {
      // inherit phpDoc only if it's not overridden in derived class
      if (!is_phpdoc_meaningful(f_->phpdoc_str)) {
        is_phpdoc_inherited = true;
      }
    } else if (f_->is_constructor() && f_->is_auto_inherited) {
      // inherited constructors copy phpdoc strings, but they still should
      // be analyzer via the base method context (see #210)
      is_phpdoc_inherited = true;
    }

    if (!is_phpdoc_inherited) {
      return f_;
    }
    for (const auto &ancestor : f_->class_id->get_all_ancestors()) {
      if (ancestor == f_->class_id) {
        continue;
      }

      if (f_->modifiers.is_instance()) {
        if (const auto *method = ancestor->members.get_instance_method(f_->local_name())) {
          if (is_phpdoc_meaningful(method->function->phpdoc_str)) {
            return method->function;
          }
        }
      } else if (const auto *method = ancestor->members.get_static_method(f_->local_name())) {
        if (is_phpdoc_meaningful(method->function->phpdoc_str)) {
          return method->function;
        }
      }
    }

    return f_;
  }

  void convert_to_template_function_if_callable_arg() {
    for (auto p : func_params_) {
      auto cur_func_param = p.as<op_func_param>();
      if (const auto *callable = cur_func_param->type_hint ? cur_func_param->type_hint->try_as<TypeHintCallable>() : nullptr) {
        if (!callable->is_typed_callable()) {    // we will generate common interfaces for typed callables in on_finish()
          GenericsDeclarationMixin::make_function_generics_on_callable_arg(f_, cur_func_param);
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
        if (!f_->generics_declaration) {
          GenericsDeclarationMixin::apply_from_phpdoc(f_, phpdoc_from_fun_->phpdoc_str);
        }
        break;
      }

      case php_doc_tag::kphp_const: {
        for (const auto &var_name : split_skipping_delimeters(tag.value, ", ")) {
          auto param = f_->find_param_by_name(var_name.substr(1));
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
          f_->performance_inspections_for_warning.set_from_php_doc(tag.value);
        } catch (const std::exception &ex) {
          kphp_error(false, fmt_format("@kphp-warn-performance bad tag: {}", ex.what()));
        }
        break;
      }

      case php_doc_tag::kphp_analyze_performance: {
        try {
          f_->performance_inspections_for_analysis.set_from_php_doc(tag.value);
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

      case php_doc_tag::kphp_color: {
        std::istringstream is(tag.value);
        std::string color_name;
        is >> color_name;

        kphp_error_return(!color_name.empty(), "An empty tag value");
        kphp_error_return(G->get_function_palette().color_exists(color_name), "Color missing in palette (either a misprint or a new color that needs to be added)");

        f_->colors.add(G->get_function_palette().get_color_by_name(color_name));
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

    if (f_->return_typehint) {
      // @kphp-return takes precedence over a regular @return (@return can contain anything regardless whether it's before or after @kphp-return)
      if (f_->return_typehint->has_genericsT_inside()) {
        return;
      }
      // if both @return and type hint exist, check their compatibility
      f_->return_typehint = merge_php_hint_and_phpdoc(f_->return_typehint, doc_parsed.type_hint, false);
    } else {
      f_->return_typehint = doc_parsed.type_hint;
    }
  }

  void parse_param_doc_tag(const php_doc_tag &tag) {
    PhpDocTagParseResult doc_parsed = phpdoc_parse_type_and_var_name(tag.value, phpdoc_from_fun_);
    if (!doc_parsed) {    // an error has already been printed
      return;
    }

    auto param = f_->find_param_by_name(doc_parsed.var_name);
    if (!param) {
      kphp_error_return(f_ != phpdoc_from_fun_, fmt_format("@param for unexisting argument ${}", doc_parsed.var_name));
      return;
    }

    if (param->type_hint) {
      // @kphp-param takes precedence over a regular @param (@param can contain anything regardless whether it's before or after @kphp-param)
      if (param->type_hint->has_genericsT_inside()) {
        return;
      }
      // if both @param and type hint exist, check their compatibility
      param->type_hint = merge_php_hint_and_phpdoc(param->type_hint, doc_parsed.type_hint, true);
    } else {
      param->type_hint = doc_parsed.type_hint;
    }

    if (infer_cast_) {
      const auto *as_primitive = doc_parsed.type_hint->try_as<TypeHintPrimitive>();
      kphp_error(as_primitive, "Too hard rule for cast");
      kphp_error(!param->is_cast_param, fmt_format("Duplicate type cast for argument '{}'", doc_parsed.var_name));
      param->is_cast_param = true;
    }
  }

  // when both type hint and phpdoc for a param/return exist,
  // check their compatibility and choose one of them
  const TypeHint *merge_php_hint_and_phpdoc(const TypeHint *php_hint, const TypeHint *phpdoc_hint, bool is_param __attribute__ ((unused))) {
    if (php_hint == phpdoc_hint) {
      return php_hint;
    }
    if (!php_hint->is_typedata_constexpr() || !phpdoc_hint->is_typedata_constexpr()) {
      return phpdoc_from_fun_ == f_ ? phpdoc_hint : php_hint;
    }

    // handle when @param/@return is from parent function, but child function has the type hint itself
    if (phpdoc_from_fun_ != f_) {
      // static inheritance allows different params/returns for a function with the same name
      // so, if a parent f() has @return int[], and a child declares f():array, leave 'array'
      if (f_->modifiers.is_static()) {
        return php_hint;
      }
      // non-static inheritance (or implementing an interface method) sometimes should auto-inherit parent phpdoc
      // for example, a parent f() has @param callable(int), and a child declares f(callable $cb) — use typed callable
      return should_inherited_phpdoc_override_self_type_hint(php_hint, phpdoc_hint) ? phpdoc_hint : php_hint;
    }
    // todo this is commented out, as currently vkcom has tons of mismatches
    // some tests in phpt/phpdocs/ are also marked with '@todo'
//    if (!does_php_hint_match_phpdoc(php_hint, phpdoc_hint)) {
//      print_error_php_hint_and_phpdoc_mismatch(php_hint, phpdoc_hint, is_param);
//    }

    return phpdoc_hint;
  }

  // for parent f($a) with @param $a and child f(hint $a),
  // check whether we need to prefer parent @return over self type hint
  bool should_inherited_phpdoc_override_self_type_hint(const TypeHint *php_hint, const TypeHint *phpdoc_hint) {
    // consider:
    // parent: @param callable(int):void f(callable $cb)
    // child:  f(callable $cb)
    // we need to use a typed callable, since 'callable' in child is just a type hint for compatibility
    // but that's not always true, for example a child hint 'int' would just narrow parent 'int|string'
    if (php_hint->try_as<TypeHintCallable>() && phpdoc_hint->try_as<TypeHintCallable>()) {
      return true;
    }
    if (php_hint->try_as<TypeHintArray>() && phpdoc_hint->try_as<TypeHintArray>()) {
      return true;
    }
    if (php_hint->try_as<TypeHintOptional>() || phpdoc_hint->try_as<TypeHintOptional>()) {
      return should_inherited_phpdoc_override_self_type_hint(php_hint->unwrap_optional(), phpdoc_hint->unwrap_optional());
    }
    return false;
  }

  // for f():array with @return int[] (same function, no inheritance),
  // check that type hint and phpdoc type match
  // note, that we can't use to_type_data(), as classes have not yet been resolved
  bool does_php_hint_match_phpdoc(const TypeHint *php_hint, const TypeHint *phpdoc_hint) {
    if (php_hint->try_as<TypeHintPrimitive>()) {
      return php_hint == phpdoc_hint;
    }
    if (php_hint->try_as<TypeHintInstance>()) {
      return php_hint == phpdoc_hint;
    }
    if (php_hint->try_as<TypeHintArray>()) {
      if (phpdoc_hint->try_as<TypeHintArray>()) {
        return true;
      }
      if (const auto *doc_pipe = phpdoc_hint->try_as<TypeHintPipe>()) {
        return std::all_of(doc_pipe->items.begin(), doc_pipe->items.end(), [](const TypeHint *item_hint) -> bool { return item_hint->try_as<TypeHintArray>(); });
      }
      return false;
    }
    if (php_hint->try_as<TypeHintCallable>()) {
      return phpdoc_hint->try_as<TypeHintCallable>();
    }
    if (const auto *php_optional = php_hint->try_as<TypeHintOptional>()) {
      if (const auto *doc_optional = phpdoc_hint->try_as<TypeHintOptional>()) {
        return !doc_optional->or_false && does_php_hint_match_phpdoc(php_optional->inner, doc_optional->inner);
      }
      if (const auto *doc_pipe = phpdoc_hint->try_as<TypeHintPipe>()) {
        return std::all_of(doc_pipe->items.begin(), doc_pipe->items.end(), [this, php_optional](const TypeHint *item_hint) -> bool {
          if (const auto *as_primitive = item_hint->try_as<TypeHintPrimitive>()) {
            return as_primitive->ptype == tp_Null;
          }
          return does_php_hint_match_phpdoc(php_optional->inner, item_hint);
        });
      }
      return false;
    }
    return true;
  }

  void print_error_php_hint_and_phpdoc_mismatch(const TypeHint *php_hint, const TypeHint *phpdoc_hint, bool is_param) __attribute__((noinline)) {
    stage::set_location(f_->root->location);
    std::string php_hint_str = php_hint == TypeHintArray::create_array_of_any() ? "array" : php_hint->as_human_readable();
    std::string doc_hint_str = phpdoc_hint->as_human_readable();
    kphp_error(0, fmt_format("php type hint {} mismatches with {} {}{}",
                             TermStringFormat::paint_green(php_hint_str), is_param ? "@param" : "@return", TermStringFormat::paint_green(doc_hint_str),
                             phpdoc_from_fun_ == f_ ? "" : " inherited from " + phpdoc_from_fun_->as_human_readable()));
  }

  void check_all_arguments_have_param_or_type_hint() {
    // at this point, all phpdocs and type hints have been parsed
    // show error if any param doesn't have type declared type
    // missing type hint and phpdoc for one of the parameters
    for (auto v : func_params_) {
      if (auto param = v.try_as<op_func_param>()) {
        kphp_error(param->type_hint || param->var()->extra_type == op_ex_var_this,
                   fmt_format("Specify @param or type hint for ${}", param->var()->get_string()));
      }
    }
  }

  void check_function_has_return_or_type_hint() {
    // at this point, all phpdocs and type hints have been parsed
    // if we have no @return and no return hint, assume @return void
    if (!f_->return_typehint) {
      bool assume_return_void = !f_->is_constructor() && !f_->assumption_for_return && !f_->is_template();

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
      f.type_hint = calculate_field_type_hint(f.phpdoc_str, f.type_hint, f.var, klass);
      if (f.type_hint) {
        f.type_hint = phpdoc_finalize_type_hint_and_resolve(f.type_hint, klass->get_holder_function());
        kphp_error(f.type_hint, fmt_format("Failed to parse @var of {}", f.var->as_human_readable()));
      }
    });
    klass->members.for_each([&](ClassMemberStaticField &f) {
      f.type_hint = calculate_field_type_hint(f.phpdoc_str, f.type_hint, f.var, klass);
      if (f.type_hint) {
        f.type_hint = phpdoc_finalize_type_hint_and_resolve(f.type_hint, klass->get_holder_function());
        kphp_error(f.type_hint, fmt_format("Failed to parse @var of {}", f.var->as_human_readable()));
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
  static const TypeHint *calculate_field_type_hint(vk::string_view phpdoc_str, const TypeHint *php_type_hint, VarPtr var, ClassPtr klass) {
    // if there is a /** @var int|false */ comment above the class field declaration
    // moreover, it overrides php_type_hint: /** @var int[] */ public array $a; — int[] is used instead of array
    if (auto tag_phpdoc = phpdoc_find_tag_as_string(phpdoc_str, php_doc_tag::var)) {
      auto parsed = phpdoc_parse_type_and_var_name(*tag_phpdoc, stage::get_function());
      if (!kphp_error(parsed, fmt_format("Failed to parse phpdoc of {}", var->as_human_readable()))) {
        return parsed.type_hint;
      }
    }

    // fields that have a type_hint in php (private ?A $field) have it already parsed in gentree
    if (php_type_hint) {
      return php_type_hint;
    }

    // if there is no /** @var ... */ and no php type hint, but class typing is mandatory, use the field initializer
    // public $a = 0; - $a is int
    // public $a = []; - $a is any[]
    if (!G->settings().require_class_typing.get()) {
      return nullptr;
    }
    if (var->init_val) {
      const TypeHint *type_hint = phpdoc_convert_default_value_to_type_hint(var->init_val);
      kphp_error(type_hint, fmt_format("Specify @var to {}", var->as_human_readable()));
      return type_hint;
    }
    if (klass->is_lambda_class()) {   // they are auto-generated, not coming from PHP code
      return nullptr;
    }

    kphp_error(0, fmt_format("Specify @var or type hint or default value to {}", var->as_human_readable()));
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

      case php_doc_tag::kphp_memcache_class:
        G->set_memcache_class(klass);
        break;

      case php_doc_tag::kphp_warn_performance:
        try {
          klass->members.for_each([&tag](const ClassMemberInstanceMethod &f) {
            f.function->performance_inspections_for_warning.set_from_php_doc(tag.value);
          });
          klass->members.for_each([&tag](const ClassMemberStaticMethod &f) {
            f.function->performance_inspections_for_warning.set_from_php_doc(tag.value);
          });
        } catch (const std::exception &ex) {
          kphp_error(false, fmt_format("@kphp-warn-performance bad tag: {}", ex.what()));
        }
        break;

      case php_doc_tag::kphp_analyze_performance:
        try {
          klass->members.for_each([&tag](const ClassMemberInstanceMethod &f) {
            f.function->performance_inspections_for_analysis.set_from_php_doc(tag.value);
          });
          klass->members.for_each([&tag](const ClassMemberStaticMethod &f) {
            f.function->performance_inspections_for_analysis.set_from_php_doc(tag.value);
          });
        } catch (const std::exception &ex) {
          kphp_error(false, fmt_format("@kphp-analyze-performance bad tag: {}", ex.what()));
        }
        break;

      default:
        break;
    }
  }
};

void ParseAndApplyPhpdocF::execute(FunctionPtr function, DataStream<FunctionPtr> &unused_os) {
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

  Base::execute(function, unused_os);
}

void ParseAndApplyPhpdocF::on_finish(DataStream<FunctionPtr> &os) {
  stage::die_if_global_errors();

  // now, when all phpdocs have been parsed,
  // register all existing typed callable interfaces and pass them through pipeline
  for (InterfacePtr interface : G->get_interfaces()) {
    if (interface->is_typed_callable_interface()) {
      G->register_and_require_function(interface->gen_holder_function(interface->name), tmp_stream, true);
      G->require_function(interface->get_instance_method("__invoke")->function, tmp_stream);
    }
  }

  Base::on_finish(os);
}
