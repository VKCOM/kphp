// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/function-data.h"

#include "common/termformat/termformat.h"

#include "compiler/code-gen/writer.h"
#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/lib-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"
#include "compiler/inferring/public.h"
#include "compiler/type-hint.h"
#include "compiler/vertex.h"

FunctionPtr FunctionData::create_function(std::string name, VertexAdaptor<op_function> root, func_type_t type) {
  static CachedProfiler cache("Create Function");
  AutoProfiler prof{*cache};
  FunctionPtr function(new FunctionData());
  root->func_id = function;

  function->name = std::move(name);
  function->root = root;
  function->file_id = stage::get_file();
  function->type = type;
  function->tinf_node.init_as_return_value(function);
  //function->function_in_which_lambda_was_created = function_root->get_func_id()->function_in_which_lambda_was_created;

  return function;
}

FunctionPtr FunctionData::clone_from(const std::string &new_name, FunctionPtr other, VertexAdaptor<op_function> new_root) {
  FunctionPtr res(new FunctionData(*other));
  res->root = new_root;
  res->is_required = false;
  res->name = new_name;
  res->update_location_in_body();
  res->name_gen_map = {};
  res->tinf_node.init_as_return_value(res);

  res->assumptions_for_vars = {};
  res->assumption_args_status = AssumptionStatus::unknown;
  res->assumption_for_return = Assumption::undefined();
  res->assumption_return_status = AssumptionStatus::unknown;
  res->assumption_return_processing_thread = std::thread::id{};

  new_root->func_id = res;

  return res;
}

bool FunctionData::is_constructor() const {
  return class_id && class_id->construct_function && class_id->construct_function == get_self();
}

bool FunctionData::is_main_function() const {
  return type == func_type_t::func_main;
}

void FunctionData::update_location_in_body() {
  if (!root) return;

  SrcFilePtr file = root->location.file;

  std::function<void(VertexPtr)> update_location = [&](VertexPtr root) {
    root->location.file = file;
    root->location.function = get_self();
    std::for_each(root->begin(), root->end(), update_location);
  };
  update_location(root);
}

std::string FunctionData::encode_template_arg_name(const Assumption &assumption, int id) {
  if (assumption.has_instance()) {
    if (ClassPtr klass = assumption.try_as_class()) {
      return "$" + replace_backslashes(klass->name);
    }
    if (Assumption inner = assumption.get_inner_if_array()) {
      return "$arr" + encode_template_arg_name(inner, id);
    }
  }
  return "$" + std::to_string(id) + "not_instance";
}

FunctionPtr FunctionData::generate_instance_of_template_function(const std::map<int, Assumption> &template_type_id_to_ClassPtr,
                                                                 FunctionPtr func,
                                                                 const std::string &name_of_function_instance) {
  kphp_assert_msg(func->is_template, "function must be template");
  auto func_args_n = static_cast<size_t>(func->get_params().size());

  auto new_func_root = func->root.clone();

  auto new_function = FunctionData::clone_from(name_of_function_instance, func, new_func_root);
  new_function->is_template = false;
  // initialize assumptions for arguments of new_function
  calc_assumption_for_argument(new_function, "");

  for (size_t i = 0; i < func_args_n; ++i) {
    auto param = new_function->get_params()[i].as<op_func_param>();
    auto id_classPtr_it = template_type_id_to_ClassPtr.find(param->template_type_id);
    if (id_classPtr_it == template_type_id_to_ClassPtr.end()) {
      kphp_error_act(template_type_id_to_ClassPtr.empty() || param->template_type_id == -1,
                     "Can't deduce template parameter of function (check count of arguments passed).",
                     return {});
      param->template_type_id = -1;
      continue;
    }
    param->template_type_id = -1;

    auto &assumption = id_classPtr_it->second;
    auto assum_for_param_it = std::find_if(new_function->assumptions_for_vars.begin(), new_function->assumptions_for_vars.end(),
                                           [&name = param->var()->str_val](auto &name_assum) { return name_assum.first == name; });
    if (assum_for_param_it != new_function->assumptions_for_vars.end()) {
      assum_for_param_it->second = assumption;
    } else {
      new_function->assumptions_for_vars.emplace_back(param->var()->str_val, assumption);
    }
  }

  // TODO: need copy all lambdas inside template function
  //for (auto f : func->lambdas_inside) {
  //  f->function_in_which_lambda_was_created = new_function;
  //}

  return new_function;
}

std::vector<VertexAdaptor<op_var>> FunctionData::get_params_as_vector_of_vars(int shift) const {
  auto func_params = get_params();
  kphp_assert(func_params.size() >= shift);

  std::vector<VertexAdaptor<op_var>> res_params(static_cast<size_t>(func_params.size() - shift));
  std::transform(std::next(func_params.begin(), shift), func_params.end(), res_params.begin(),
                 [](VertexPtr param) { return param.as<op_func_param>()->var().clone(); }
  );

  return res_params;
}

void FunctionData::move_virtual_to_self_method(DataStream<FunctionPtr> &os) {
  auto self_function_vertex = VertexAdaptor<op_function>::create(root->param_list().clone(), root->cmd());
  auto self_function = clone_from(replace_backslashes(class_id->name) + "$$" + get_name_of_self_method(), get_self(), self_function_vertex);
  // It's safe to copy assumptions here and useful only for __virt_clone_self method
  kphp_assert(assumption_return_status != AssumptionStatus::processing);
  self_function->assumption_for_return = assumption_for_return;
  self_function->assumption_return_status = assumption_return_status;
  class_id->members.safe_add_instance_method(self_function);
  self_function->is_virtual_method = false;

  root->cmd_ref() = VertexAdaptor<op_seq>::create();
  G->register_and_require_function(self_function, os, true);
}

std::string FunctionData::get_name_of_self_method(vk::string_view name) {
  return std::string{name} + "_self$";
}

std::string FunctionData::get_name_of_self_method() const {
  return get_name_of_self_method(local_name());
}

int FunctionData::get_min_argn() {
  if (min_argn != -1) {
    return min_argn;
  }

  auto has_default = [](VertexPtr v) {
    auto param = v.as<op_func_param>();
    return param->has_default_value() && param->default_value();
  };

  auto params = get_params();
  auto it_param_with_default_value = std::find_if(params.begin(), params.end(), has_default);
  min_argn = std::distance(params.begin(), it_param_with_default_value);

  return min_argn;
}

string FunctionData::get_resumable_path() const {
  vector<string> names;
  FunctionPtr f = fork_prev;
  while (f) {
    names.push_back(f->get_human_readable_name());
    f = f->fork_prev;
  }
  std::reverse(names.begin(), names.end());
  names.push_back(TermStringFormat::paint(name, TermStringFormat::red));
  f = wait_prev;
  while (f) {
    names.push_back(f->get_human_readable_name());
    f = f->wait_prev;
  }
  return vk::join(names, " -> ");
}

string FunctionData::get_throws_call_chain() const {
  if (!can_throw()) {
    return "";
  }
  vector<string> names;
  FunctionPtr f = throws_reason;
  Location last;
  names.push_back(get_human_readable_name());
  if (f) {
    while (true) {
      names.push_back(f->get_human_readable_name());
      if (f->throws_reason) {
        f = f->throws_reason;
      } else {
        break;
      }
    }
    last = f->throws_location;
  } else {
    last = throws_location;
  }
  return vk::join(names, " -> ") + (last.get_line() != -1 ? fmt_format(" (line {})", last.get_line()) : "");
}

std::string FunctionData::get_performance_inspections_warning_chain(PerformanceInspections::Inspections inspection, bool search_disabled_inspection) const noexcept {
  const auto check_inspection = [search_disabled_inspection, inspection](FunctionPtr f) {
    return search_disabled_inspection
           ? (f->performance_inspections_for_warning.disabled_inspections() & inspection)
           : (f->performance_inspections_for_warning.inspections() & inspection);
  };

  std::forward_list<std::string> chain;
  FunctionPtr fun = get_self();
  chain.emplace_front(fun->get_human_readable_name());
  while (!fun->performance_inspections_for_warning.has_their_own_inspections()) {
    auto caller_it = std::find_if(fun->performance_inspections_for_warning_parents.begin(), fun->performance_inspections_for_warning_parents.end(), check_inspection);
    kphp_assert(caller_it != fun->performance_inspections_for_warning_parents.end());
    fun = *caller_it;
    chain.emplace_front(fun->get_human_readable_name());
  }

  return vk::join(chain, " -> ");
}

std::string FunctionData::get_human_readable_name(const std::string &name, bool add_details) {
  auto pos1 = name.find("$$");
  auto pos2 = name.rfind("$$");

  // if name is just a function "function_name", doesn't belong to any class
  if (pos1 == std::string::npos) {
    return name;
  }
  // if name is "class$$method"
  else if (pos2 == pos1) {
    std::string base_class = replace_characters(name.substr(0, pos1), '$', '\\');
    std::string method_name = name.substr(pos1 + 2);
    return base_class + "::" + method_name;
  }
  // if name is "class$$method$$context"
  else {
    std::string base_class = replace_characters(name.substr(0, pos1), '$', '\\');
    std::string method_name = name.substr(pos1 + 2, pos2 - pos1 - 2);
    std::string context_class = replace_characters(name.substr(pos2 + 2), '$', '\\');
    return add_details ? base_class + "::" + method_name + " (static=" + context_class + ")" : context_class + "::" + method_name;
  }
}

string FunctionData::get_human_readable_name(bool add_details) const {
  std::string result_name;
  if (is_lambda()) {
    result_name = get_params().empty() ? "anonymous()" : "anonymous(...)";
  } else {
    result_name = get_human_readable_name(name, add_details);
  }

  if (add_details && is_instantiation_of_template_function()) {
    auto &file = instantiation_of_template_function_location.get_file()->unified_file_name;
    auto line = std::to_string(instantiation_of_template_function_location.line);
    result_name += " (instantiated at " + file + ":" + line + ")";
  }

  return result_name;
}

bool FunctionData::is_lambda_with_uses() const {
  return is_lambda() && class_id && class_id->members.has_any_instance_var();
}

bool FunctionData::is_imported_from_static_lib() const {
  return file_id->owner_lib && !file_id->owner_lib->is_raw_php() && !is_main_function();
}

// in fact it's range of op_func_param, but it can't be expressed with VertexRange and needs .as<op_func_param>() when using
VertexRange FunctionData::get_params() const {
  return root->param_list()->params();
}

bool FunctionData::check_cnt_params(int expected_cnt_params, FunctionPtr called_func) {
  if (!called_func) {
    return false;
  }

  int min_cnt_params = called_func->get_min_argn() - called_func->has_implicit_this_arg();
  int max_cnt_params = static_cast<int>(called_func->get_params().size()) - called_func->has_implicit_this_arg();
  if (expected_cnt_params < min_cnt_params || max_cnt_params < expected_cnt_params) {
    kphp_error(false, fmt_format("Wrong arguments count: {} (expected {}..{})", expected_cnt_params, min_cnt_params, max_cnt_params));
    return false;
  }

  return true;
}

bool FunctionData::does_need_codegen(FunctionPtr f) {
  return f->type != FunctionData::func_class_holder &&
         f->type != FunctionData::func_extern &&
         (f->body_seq != body_value::empty || G->get_main_file()->main_function == f);
}

bool operator<(FunctionPtr a, FunctionPtr b) {
  return a->name < b->name;
}
