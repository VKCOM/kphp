#include "compiler/data/function-data.h"

#include <regex>

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/lambda-class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"
#include "compiler/inferring/public.h"
#include "compiler/io.h"
#include "compiler/pipes/calc-locations.h"
#include "compiler/vertex.h"

FunctionPtr FunctionData::create_function(VertexAdaptor<op_function> root, func_type_t type) {
  static CachedProfiler cache("create_function");
  AutoProfiler prof{*cache};
  FunctionPtr function(new FunctionData());
  root->set_func_id(function);

  function->name = root->name()->get_string();
  function->root = root;
  function->file_id = stage::get_file();
  function->type = type;
  //function->function_in_which_lambda_was_created = function_root->get_func_id()->function_in_which_lambda_was_created;

  return function;
}

bool FunctionData::is_constructor() const {
  return class_id && class_id->construct_function && &*(class_id->construct_function) == this;
}

void FunctionData::update_location_in_body() {
  if (!root) return;

  std::function<void(VertexPtr)> update_location = [&](VertexPtr root) {
    root->location.function = FunctionPtr{this};
    std::for_each(root->begin(), root->end(), update_location);
  };
  update_location(root);
}

std::string FunctionData::encode_template_arg_name(AssumType assum, int id, ClassPtr klass) {
  switch (assum) {
    case assum_not_instance:
    case assum_unknown:
      return "$" + std::to_string(id) + "not_instance";

    case assum_instance_array:
      return "$arr$" + replace_backslashes(klass->name);

    case assum_instance:
      return "$" + replace_backslashes(klass->name);
      
    default:
      __builtin_unreachable();
  }
}

FunctionPtr FunctionData::generate_instance_of_template_function(const std::map<int, std::pair<AssumType, ClassPtr>> &template_type_id_to_ClassPtr,
                                                                 FunctionPtr func,
                                                                 const std::string &name_of_function_instance) {
  kphp_assert_msg(func->is_template, "function must be template");
  auto param_list = func->root->params().as<op_func_param_list>();
  VertexRange func_args = param_list->params();
  auto func_args_n = static_cast<size_t>(func_args.size());

  auto new_func_root = func->root.clone();
  new_func_root->name()->set_string(name_of_function_instance);

  auto new_function = FunctionData::create_function(new_func_root, func->type);

  for (size_t i = 0; i < func_args_n; ++i) {
    VertexAdaptor<op_func_param> param = new_func_root->params().as<op_func_param_list>()->params()[i].as<op_func_param>();
    auto id_classPtr_it = template_type_id_to_ClassPtr.find(param->template_type_id);
    if (id_classPtr_it == template_type_id_to_ClassPtr.end()) {
      kphp_error_act(template_type_id_to_ClassPtr.empty() || param->template_type_id == -1,
                     "Can't deduce template parameter of function (check count of arguments passed).",
                     return {});
      param->template_type_id = -1;
      continue;
    }
    param->template_type_id = -1;

    const std::pair<AssumType, ClassPtr> &assum_and_class = id_classPtr_it->second;
    new_function->assumptions_for_vars.emplace_back(assum_and_class.first, param->var()->get_string(), assum_and_class.second);
    new_function->assumptions_inited_args = 2;
  }

  new_function->root = new_func_root;
  new_function->is_required = false;
  new_function->file_id = func->file_id;
  new_function->class_id = func->class_id;
  new_function->has_variadic_param = func->has_variadic_param;
  new_function->tinf_state = func->tinf_state;
  new_function->phpdoc_token = func->phpdoc_token;
  new_function->min_argn = func->min_argn;
  new_function->context_class = func->context_class;
  new_function->access_type = func->access_type;
  new_function->body_seq = func->body_seq;
  new_function->is_template = false;
  new_function->is_inline = func->is_inline;
  new_function->function_in_which_lambda_was_created = func->function_in_which_lambda_was_created;
  new_function->infer_hints = func->infer_hints;

  // TODO: need copy all lambdas inside template funciton
  //for (auto f : func->lambdas_inside) {
  //  f->function_in_which_lambda_was_created = new_function;
  //}

  new_function->update_location_in_body();

  return new_function;
}

string FunctionData::get_resumable_path() const {
  vector<string> names;
  FunctionPtr f = fork_prev;
  while (f) {
    names.push_back(f->name);
    f = f->fork_prev;
  }
  std::reverse(names.begin(), names.end());
  names.push_back(name);
  f = wait_prev;
  while (f) {
    names.push_back(f->name);
    f = f->wait_prev;
  }
  stringstream res;
  for (int i = 0; i < names.size(); i++) {
    if (i) {
      res << " -> ";
    }
    res << names[i];
  }
  return res.str();
}

std::string FunctionData::get_human_readable_name(const std::string &name) {
  std::smatch matched;
  if (std::regex_match(name, matched, std::regex(R"((.+)\$\$(.+)\$\$(.+))"))) {
    string base_class = matched[1].str(), actual_class = matched[3].str();
    base_class = replace_characters(base_class, '$', '\\');
    actual_class = replace_characters(actual_class, '$', '\\');
    return actual_class + "::" + matched[2].str() + " (" + "inherited from " + base_class + ")";
  }
  //Модифицировать вывод осторожно! По некоторым символам используется поиск регекспами при выводе стектрейса
  return std::regex_replace(std::regex_replace(name, std::regex(R"(\$\$)"), "::"), std::regex("\\$"), "\\");
}

string FunctionData::get_human_readable_name() const {
  if (access_type == access_nonmember) {
    return name;
  }

  return is_lambda() ? "anonymous(...)" : get_human_readable_name(name);
}

void FunctionData::add_kphp_infer_hint(FunctionData::InferHint::infer_mask infer_mask, int param_i, VertexPtr type_rule) {
  set_location(type_rule, root->location);
  infer_hints.emplace_back(InferHint{infer_mask, param_i, type_rule});
}

bool FunctionData::is_lambda_with_uses() const {
  return is_lambda() && class_id && class_id->members.has_any_instance_var();
}

bool FunctionData::is_imported_from_static_lib() const {
  return file_id->owner_lib && !file_id->owner_lib->is_raw_php() && &(*file_id->main_function) != this;
}

VertexRange FunctionData::get_params() const {
  return ::get_function_params(root);
}

bool operator<(FunctionPtr a, FunctionPtr b) {
  return a->name < b->name;
}
