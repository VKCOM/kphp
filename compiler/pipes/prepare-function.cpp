#include "compiler/pipes/prepare-function.h"

#include <unordered_map>

#include "compiler/compiler-core.h"
#include "compiler/phpdoc.h"
#include "compiler/data/class-data.h"

static void function_apply_header(FunctionPtr func, VertexAdaptor<meta_op_function> header) {
  VertexAdaptor<meta_op_function> root = func->root;
  func->used_in_source = true;

  kphp_assert (root && header);
  kphp_error_return (
    !func->header,
    format("Function [%s]: multiple headers", func->get_human_readable_name().c_str())
  );
  func->header = header;

  kphp_error_return (
    !root->type_rule,
    format("Function [%s]: type_rule is overided by header", func->get_human_readable_name().c_str())
  );
  root->type_rule = header->type_rule;

  kphp_error_return (
    !(!header->varg_flag && func->varg_flag),
    format("Function [%s]: varg_flag mismatch with header", func->get_human_readable_name().c_str())
  );
  func->varg_flag = header->varg_flag;

  if (!func->varg_flag) {
    VertexAdaptor<op_func_param_list> root_params_vertex = root->params();
    VertexAdaptor<op_func_param_list> header_params_vertex = header->params();
    VertexRange root_params = root_params_vertex->params();
    VertexRange header_params = header_params_vertex->params();

    kphp_error (
      root_params.size() == header_params.size(),
      format("Bad header for function [%s]", func->get_human_readable_name().c_str())
    );
    int params_n = (int)root_params.size();
    for (int i = 0; i < params_n; i++) {
      kphp_error (
        root_params[i]->size() == header_params[i]->size(),
        format(
          "Function [%s]: %dth param has problem with default value",
          func->get_human_readable_name().c_str(), i + 1
        )
      );
      kphp_error (
        root_params[i]->type_help == tp_Unknown,
        format("Function [%s]: type_help is overrided by header", func->get_human_readable_name().c_str())
      );
      root_params[i]->type_help = header_params[i]->type_help;
    }
  }
}

static void check_template_function(FunctionPtr func) {
  if (!func->is_template) return;

  for (auto l : func->lambdas_inside) {
    const auto &prev_location = stage::get_location();
    stage::set_location(l->class_id->construct_function->root->location);
    kphp_error(!l->is_lambda_with_uses(), "it's not allowed lambda with uses inside template function(or another lambda)");
    stage::set_location(prev_location);
  }
}

static void prepare_function_misc(FunctionPtr func) {
  check_template_function(func);
  VertexRange params = get_function_params(func->root.as<meta_op_function>());
  int param_n = (int)params.size();
  bool was_default = false;
  func->min_argn = param_n;
  for (int i = 0; i < param_n; i++) {
    VertexAdaptor<meta_op_func_param> param = params[i].as<meta_op_func_param>();

    if (func->varg_flag) {
      kphp_error (!param->var()->ref_flag,
                  "Reference arguments are not supported in varg functions");
    }

    if (param->type_declaration == "callable") {
      param->is_callable = true;
      param->template_type_id = param_n + i;
      param->type_declaration.clear();
      func->is_template = true;
    }

    if (param->has_default_value() && param->default_value()) {
      if (!was_default) {
        was_default = true;
        func->min_argn = i;
      }
      if (func->type() == FunctionData::func_local) {
        kphp_error (!param->var()->ref_flag,
                    format("Default value in reference function argument [function = %s]", func->get_human_readable_name().c_str()));
      }
    } else {
      kphp_error (!was_default,
                  format("Default value expected [function = %s] [param_i = %d]", func->get_human_readable_name().c_str(), i));
    }
  }
}

/*
 * Анализ @kphp-infer, @kphp-inline и других @kphp-* внутри phpdoc над функцией f
 * Сейчас пока что есть костыль: @kphp-required анализируется всё равно в gentree
 */
static void parse_and_apply_function_kphp_phpdoc(FunctionPtr f) {
  if (f->phpdoc_token == nullptr || f->phpdoc_token->type() != tok_phpdoc_kphp) {
    return;   // обычный phpdoc, без @kphp нотаций, тут не парсим; если там инстансы, распарсится по требованию
  }

  using infer_mask = FunctionData::InferHint::infer_mask;

  int infer_type = 0;
  VertexRange func_params = get_function_params(f->root);
  const vector<php_doc_tag> &tags = parse_php_doc(f->phpdoc_token->str_val);

  std::unordered_map<std::string, int> name_to_function_param;
  int param_i = 0;
  for (VertexAdaptor<meta_op_func_param> param : func_params) {
    name_to_function_param.emplace("$" + param->var()->get_string(), param_i++);
  }

  std::size_t id_of_kphp_template = 0;
  stage::set_location(f->root->get_location());
  for (auto &tag : tags) {
    stage::set_line(tag.line_num);
    switch (tag.type) {
      case php_doc_tag::kphp_inline: {
        f->is_inline = true;
        continue;
      }

      case php_doc_tag::kphp_sync: {
        f->should_be_sync = true;
        continue;
      }

      case php_doc_tag::kphp_infer: {
        kphp_error(infer_type == 0, "Double kphp-infer tag found");
        std::istringstream is(tag.value);
        string token;
        while (is >> token) {
          if (token == "check") {
            infer_type |= infer_mask::check;
          } else if (token == "hint") {
            infer_type |= infer_mask::hint;
          } else if (token == "cast") {
            infer_type |= infer_mask::cast;
          } else {
            kphp_error(0, format("Unknown kphp-infer tag type '%s'", token.c_str()));
          }
        }
        break;
      }

      case php_doc_tag::kphp_disable_warnings: {
        std::istringstream is(tag.value);
        std::string token;
        while (is >> token) {
          if (!f->disabled_warnings.insert(token).second) {
            kphp_warning(format("Warning '%s' has been disabled twice", token.c_str()));
          }
        }
        break;
      }

      case php_doc_tag::kphp_extern_func_info: {
        kphp_error(f->type() == FunctionData::func_extern, "@kphp-extern-func-info used for regular function");
        std::istringstream is(tag.value);
        std::string token;
        while (is >> token) {
          if (token == "can_throw") {
            f->can_throw = true;
          } else if (token == "resumable") {
            f->root->resumable_flag = true;
          } else if (token == "cpp_template_call") {
            f->cpp_template_call = true;
          } else {
            kphp_error(0, format("Unknown @kphp-extern-func-info %s", token.c_str()));
          }
        }
        break;
      }

      case php_doc_tag::kphp_lib_export: {
        f->kphp_lib_export = true;
        infer_type |= (infer_mask::check | infer_mask::hint);
        break;
      }

      case php_doc_tag::kphp_template: {
        f->is_template = true;
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

          auto func_param_it = name_to_function_param.find(var_name);
          kphp_error_return(func_param_it != name_to_function_param.end(), format("@kphp-template tag var name mismatch. found %s.", var_name.c_str()));

          VertexAdaptor<op_func_param> cur_func_param = func_params[func_param_it->second];
          name_to_function_param.erase(func_param_it);

          cur_func_param->template_type_id = id_of_kphp_template;
        }
        id_of_kphp_template++;

        break;
      }

      default:
        break;
    }
  }

  if (infer_type) {             // при наличии @kphp-infer парсим все @param'ы
    for (auto &tag : tags) {    // (вторым проходом, т.к. @kphp-infer может стоять в конце)
      stage::set_line(tag.line_num);
      switch (tag.type) {
        case php_doc_tag::returns: {
          std::istringstream is(tag.value);
          std::string type_help;
          kphp_error(is >> type_help, "Failed to parse @return/@returns tag");
          VertexPtr doc_type = phpdoc_parse_type(type_help, f);
          kphp_error_act(doc_type, format("Failed to parse type '%s'", type_help.c_str()), break);
          
          if (infer_type & infer_mask::check) {
            auto type_rule = VertexAdaptor<op_lt_type_rule>::create(doc_type);
            type_rule->void_flag = doc_type->void_flag;
            f->add_kphp_infer_hint(infer_mask::check, -1, type_rule);
          }
          // hint для return'а не делаем совсем, чтобы не грубить вывод типов, только check
          break;
        }
        case php_doc_tag::param: {
          kphp_error_return(!name_to_function_param.empty(), "Too many @param tags");
          std::istringstream is(tag.value);
          std::string type_help, var_name;
          kphp_error(is >> type_help, "Failed to parse @param tag");
          kphp_error(is >> var_name, "Failed to parse @param tag");

          auto func_param_it = name_to_function_param.find(var_name);
          kphp_error_return(func_param_it != name_to_function_param.end(),
            format("@param tag var name mismatch. found %s.", var_name.c_str()));
          int param_i = func_param_it->second;

          VertexAdaptor<op_func_param> cur_func_param = func_params[func_param_it->second];
          name_to_function_param.erase(func_param_it);

          if (type_help == "callable") {
            f->is_template = true;
            cur_func_param->template_type_id = id_of_kphp_template;
            cur_func_param->is_callable = true;
            id_of_kphp_template++;
            continue;
          }

          VertexPtr doc_type = phpdoc_parse_type(type_help, f);
          kphp_error(doc_type, format("Failed to parse type '%s'", type_help.c_str()));

          if (infer_type & infer_mask::check) {
            auto type_rule = VertexAdaptor<op_lt_type_rule>::create(doc_type);
            f->add_kphp_infer_hint(infer_mask::check, param_i, type_rule);
          }
          if (infer_type & infer_mask::hint) {
            auto type_rule = VertexAdaptor<op_common_type_rule>::create(doc_type);
            f->add_kphp_infer_hint(infer_mask::hint, param_i, type_rule);
          }
          if (infer_type & infer_mask::cast) {
            kphp_error(doc_type->type() == op_type_rule && doc_type.as<op_type_rule>()->args().empty(),
                       format("Too hard rule '%s' for cast", type_help.c_str()));
            kphp_error(cur_func_param->type_help == tp_Unknown,
                       format("Duplicate type rule for argument '%s'", var_name.c_str()));
            cur_func_param->type_help = doc_type.as<op_type_rule>()->type_help;
          }

          break;
        }
        default:
          break;
      }
    }
  }

  size_t cnt_left_params_without_tag = f->has_implicit_this_arg() ? 1 : 0;  // пропускаем неявный this
  if (infer_type && name_to_function_param.size() != cnt_left_params_without_tag) {
    std::string err_msg = "Not enough @param tags. Need tags for function arguments:\n";
    for (auto name_and_function_param : name_to_function_param) {
      err_msg += name_and_function_param.first;
      err_msg += "\n";
    }
    stage::set_location(f->root->get_location());
    kphp_error(false, err_msg.c_str());
  }
}

void PrepareFunctionF::execute(FunctionPtr function, DataStream<FunctionPtr> &os) {
  stage::set_name("Prepare function");
  stage::set_function(function);
  kphp_assert (function);

  parse_and_apply_function_kphp_phpdoc(function);
  prepare_function_misc(function);

  VertexPtr header = G->get_extern_func_header(function->name);
  if (header) {
    function_apply_header(function, header);
  }
  if (function->root && function->root->varg_flag) {
    function->varg_flag = true;
  }

  if (stage::has_error()) {
    return;
  }

  os << function;
}
