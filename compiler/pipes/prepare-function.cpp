#include "compiler/pipes/prepare-function.h"

#include <unordered_map>

#include "compiler/compiler-core.h"
#include "compiler/phpdoc.h"

static void function_apply_header(FunctionPtr func, VertexAdaptor<meta_op_function> header) {
  VertexAdaptor<meta_op_function> root = func->root;
  func->used_in_source = true;

  kphp_assert (root && header);
  kphp_error_return (
    !func->header,
    dl_pstr("Function [%s]: multiple headers", func->get_human_readable_name().c_str())
  );
  func->header = header;

  kphp_error_return (
    !root->type_rule,
    dl_pstr("Function [%s]: type_rule is overided by header", func->get_human_readable_name().c_str())
  );
  root->type_rule = header->type_rule;

  kphp_error_return (
    !(!header->varg_flag && func->varg_flag),
    dl_pstr("Function [%s]: varg_flag mismatch with header", func->get_human_readable_name().c_str())
  );
  func->varg_flag = header->varg_flag;

  if (!func->varg_flag) {
    VertexAdaptor<op_func_param_list> root_params_vertex = root->params();
    VertexAdaptor<op_func_param_list> header_params_vertex = header->params();
    VertexRange root_params = root_params_vertex->params();
    VertexRange header_params = header_params_vertex->params();

    kphp_error (
      root_params.size() == header_params.size(),
      dl_pstr("Bad header for function [%s]", func->get_human_readable_name().c_str())
    );
    int params_n = (int)root_params.size();
    for (int i = 0; i < params_n; i++) {
      kphp_error (
        root_params[i]->size() == header_params[i]->size(),
        dl_pstr(
          "Function [%s]: %dth param has problem with default value",
          func->get_human_readable_name().c_str(), i + 1
        )
      );
      kphp_error (
        root_params[i]->type_help == tp_Unknown,
        dl_pstr("Function [%s]: type_help is overrided by header", func->get_human_readable_name().c_str())
      );
      root_params[i]->type_help = header_params[i]->type_help;
    }
  }
}

static void prepare_function_misc(FunctionPtr func) {
  VertexAdaptor<meta_op_function> func_root = func->root;
  kphp_assert (func_root);
  VertexAdaptor<op_func_param_list> param_list = func_root->params();
  VertexRange params = param_list->args();
  int param_n = (int)params.size();
  bool was_default = false;
  func->min_argn = param_n;
  for (int i = 0; i < param_n; i++) {
    if (func->varg_flag) {
      kphp_error (params[i].as<meta_op_func_param>()->var()->ref_flag == false,
                  "Reference arguments are not supported in varg functions");
    }

    VertexAdaptor<meta_op_func_param> param = params[i].as<meta_op_func_param>();
    if (param->type_declaration == "Callable" || param->type_declaration == "callable") {
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
        kphp_error (params[i].as<meta_op_func_param>()->var()->ref_flag == false,
                    dl_pstr("Default value in reference function argument [function = %s]", func->get_human_readable_name().c_str()));
      }
    } else {
      kphp_error (!was_default,
                  dl_pstr("Default value expected [function = %s] [param_i = %d]",
                          func->get_human_readable_name().c_str(), i));
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

  int infer_type = 0;
  vector<VertexPtr> prepend_cmd;
  VertexPtr func_params = f->root.as<op_function>()->params();
  const vector<php_doc_tag> &tags = parse_php_doc(f->phpdoc_token->str_val);

  std::unordered_map<std::string, VertexAdaptor<op_func_param>> name_to_function_param;
  for (auto param_ptr : *func_params) {
    VertexAdaptor<op_func_param> param = param_ptr.as<op_func_param>();
    name_to_function_param.emplace("$" + param->var()->get_string(), param);
  }

  std::size_t id_of_kphp_template = 0;
  stage::set_location(f->root->get_location());
  for (auto &tag : tags) {
    stage::set_line(tag.line_num);
    switch (tag.type) {
      case php_doc_tag::kphp_inline: {
        f->root->inline_flag = true;
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
            infer_type |= 1;
          } else if (token == "hint") {
            infer_type |= 2;
          } else if (token == "cast") {
            infer_type |= 4;
          } else {
            kphp_error(0, dl_pstr("Unknown kphp-infer tag type '%s'", token.c_str()));
          }
        }
        break;
      }

      case php_doc_tag::kphp_disable_warnings: {
        std::istringstream is(tag.value);
        string token;
        while (is >> token) {
          if (!f->disabled_warnings.insert(token).second) {
            kphp_warning(dl_pstr("Warning '%s' has been disabled twice", token.c_str()));
          }
        }
        break;
      }

      case php_doc_tag::kphp_required: {
        f->kphp_required = true;
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
          kphp_error_return(func_param_it != name_to_function_param.end(), dl_pstr("@kphp-template tag var name mismatch. found %s.", var_name.c_str()));

          VertexAdaptor<op_func_param> cur_func_param = func_param_it->second;
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
        case php_doc_tag::param: {
          kphp_error_return(!name_to_function_param.empty(), "Too many @param tags");
          std::istringstream is(tag.value);
          string type_help, var_name;
          kphp_error(is >> type_help, "Failed to parse @param tag");
          kphp_error(is >> var_name, "Failed to parse @param tag");

          auto func_param_it = name_to_function_param.find(var_name);
          kphp_error_return(func_param_it != name_to_function_param.end(), dl_pstr("@param tag var name mismatch. found %s.", var_name.c_str()));

          VertexAdaptor<op_func_param> cur_func_param = func_param_it->second;
          VertexAdaptor<op_var> var = cur_func_param->var().as<op_var>();
          name_to_function_param.erase(func_param_it);

          if (type_help == "callable" || type_help == "Callable") {
            f->is_template = true;
            cur_func_param->template_type_id = id_of_kphp_template;
            id_of_kphp_template++;
            continue;
          }

          VertexPtr doc_type = phpdoc_parse_type(type_help, f);
          kphp_error(doc_type, dl_pstr("Failed to parse type '%s'", type_help.c_str()));

          if (infer_type & 1) {
            auto doc_type_check = VertexAdaptor<op_lt_type_rule>::create(doc_type);
            auto doc_rule_var = VertexAdaptor<op_var>::create();
            doc_rule_var->str_val = var->str_val;
            doc_rule_var->type_rule = doc_type_check;
            set_location(doc_rule_var, f->root->location);
            prepend_cmd.push_back(doc_rule_var);
          }
          if (infer_type & 2) {
            auto doc_type_check = VertexAdaptor<op_common_type_rule>::create(doc_type);
            auto doc_rule_var = VertexAdaptor<op_var>::create();
            doc_rule_var->str_val = var->str_val;
            doc_rule_var->type_rule = doc_type_check;
            set_location(doc_rule_var, f->root->location);
            prepend_cmd.push_back(doc_rule_var);
          }
          if (infer_type & 4) {
            kphp_error(doc_type->type() == op_type_rule && doc_type.as<op_type_rule>()->args().empty(),
                       dl_pstr("Too hard rule '%s' for cast", type_help.c_str()));
            kphp_error(cur_func_param->type_help == tp_Unknown,
                       dl_pstr("Duplicate type rule for argument '%s'", var_name.c_str()));
            cur_func_param->type_help = doc_type.as<op_type_rule>()->type_help;
          }

          break;
        }
        default:
          break;
      }
    }
  }

  size_t cnt_left_params_without_tag = f->is_instance_function() && !f->is_constructor() ? 1 : 0;  // пропускаем неявный this
  if (infer_type && name_to_function_param.size() != cnt_left_params_without_tag) {
    std::string err_msg = "Not enough @param tags. Need tags for function arguments:\n";
    for (auto name_and_function_param : name_to_function_param) {
      err_msg += name_and_function_param.first;
      err_msg += "\n";
    }
    stage::set_location(f->root->get_location());
    kphp_error(false, err_msg.c_str());
  }

  // из { cmd } делаем { prepend_cmd; cmd }
  if (!prepend_cmd.empty()) {
    for (auto i : *f->root.as<op_function>()->cmd()) {
      prepend_cmd.push_back(i);
    }
    auto new_cmd = VertexAdaptor<op_seq>::create(prepend_cmd);
    ::set_location(new_cmd, f->root->location);
    f->root.as<op_function>()->cmd() = new_cmd;
  }
}

static void prepare_function(FunctionPtr function) {
  parse_and_apply_function_kphp_phpdoc(function);
  prepare_function_misc(function);

  VertexPtr header = G->get_extern_func_header(function->name);
  if (header) {
    function_apply_header(function, header);
  }
  if (function->root && function->root->varg_flag) {
    function->varg_flag = true;
  }
}

void PrepareFunctionF::execute(FunctionPtr function, DataStream<FunctionPtr> &os) {
  stage::set_name("Prepare function");
  stage::set_function(function);
  kphp_assert (function);

  prepare_function(function);

  if (stage::has_error()) {
    return;
  }

  os << function;
}
