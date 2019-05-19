#include "compiler/pipes/prepare-function.h"

#include <sstream>
#include <unordered_map>

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/phpdoc.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"

//static void check_template_function(FunctionPtr func) {
//  if (!func->is_template) return;
//  for (auto l : func->lambdas_inside) {
//    const auto &prev_location = stage::get_location();
//    stage::set_location(l->class_id->construct_function->root->location);
//    kphp_error(!l->is_lambda_with_uses(), "it's not allowed lambda with uses inside template function(or another lambda)");
//    stage::set_location(prev_location);
//  }
//}

/*
 * Анализ @kphp-infer, @kphp-inline и других @kphp-* внутри phpdoc над функцией f
 * Сейчас пока что есть костыль: @kphp-required анализируется всё равно в gentree
 */
static void parse_and_apply_function_kphp_phpdoc(FunctionPtr f) {
  if (f->phpdoc_str.find("@kphp") == std::string::npos) {
    return;   // обычный phpdoc, без @kphp нотаций, тут не парсим; если там инстансы, распарсится по требованию
  }

  using infer_mask = FunctionData::InferHint::infer_mask;

  int infer_type = 0;
  VertexRange func_params = get_function_params(f->root);
  const vector<php_doc_tag> &tags = parse_php_doc(f->phpdoc_str);

  std::unordered_map<std::string, int> name_to_function_param;
  int param_i = 0;
  for (auto param : func_params) {
    name_to_function_param.emplace("$" + param.as<meta_op_func_param>()->var()->get_string(), param_i++);
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
        infer_type |= (infer_mask::check | infer_mask::hint);
        if (tag.value.find("cast") != std::string::npos) {
          infer_type |= infer_mask::cast;
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
        kphp_error(f->is_extern(), "@kphp-extern-func-info used for regular function");
        std::istringstream is(tag.value);
        std::string token;
        while (is >> token) {
          if (token == "can_throw") {
            f->can_throw = true;
          } else if (token == "resumable") {
            f->is_resumable = true;
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

          auto cur_func_param = func_params[func_param_it->second].as<op_func_param>();
          name_to_function_param.erase(func_param_it);

          cur_func_param->template_type_id = id_of_kphp_template;
        }
        id_of_kphp_template++;

        break;
      }

      case php_doc_tag::kphp_const: {
        for (const auto &var_name : split_skipping_delimeters(tag.value, ", ")) {
          auto func_param_it = name_to_function_param.find(var_name);
          kphp_error_return(func_param_it != name_to_function_param.end(), format("@kphp-const tag var name mismatch. found %s.", var_name.c_str()));

          auto cur_func_param = func_params[func_param_it->second].as<op_func_param>();
          cur_func_param->var().as<op_var>()->is_const = true;
        }

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

          auto cur_func_param = func_params[func_param_it->second].as<op_func_param>();
          name_to_function_param.erase(func_param_it);

          if (type_help == "callable") {
            f->is_template = true;
            cur_func_param->template_type_id = id_of_kphp_template;
            cur_func_param->is_callable = true;
            id_of_kphp_template++;
            continue;
          }

          VertexPtr doc_type = phpdoc_parse_type(type_help, f);
          kphp_error_act(doc_type, format("Failed to parse type '%s'", type_help.c_str()), continue);

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
  function->calc_min_argn();

  if (stage::has_error()) {
    return;
  }

  os << function;
}
