// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/function-data.h"

#include "common/termformat/termformat.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/generics-mixins.h"
#include "compiler/data/lib-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"
#include "compiler/inferring/public.h"
#include "compiler/type-hint.h"
#include "compiler/vertex.h"
#include "common/algorithms/contains.h"

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

  return function;
}

FunctionPtr FunctionData::clone_from(FunctionPtr other, const std::string &new_name, VertexAdaptor<op_function> root_instead_of_cloning) {
  FunctionPtr res(new FunctionData(*other));
  res->root = root_instead_of_cloning ?: other->root.clone();
  res->root->func_id = res;
  res->is_required = false;
  res->name = new_name;
  res->update_location_in_body();
  res->name_gen_map = {};
  res->tinf_node.init_as_return_value(res);
  res->assumption_processing_thread = std::thread::id{};

  return res;
}

bool FunctionData::is_constructor() const {
  return class_id && class_id->construct_function && class_id->construct_function == get_self();
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
  auto v_op_function = VertexAdaptor<op_function>::create(root->param_list().clone(), root->cmd());
  auto self_function = clone_from(get_self(), replace_backslashes(class_id->name) + "$$" + get_name_of_self_method(), v_op_function);
  self_function->is_virtual_method = false;

  class_id->members.add_instance_method(self_function);   // don't need a lock here, it's invoked from a sync pipe

  root->cmd_ref() = VertexAdaptor<op_seq>::create();
  G->register_and_require_function(self_function, os, true);
}

std::string FunctionData::get_name_of_self_method(vk::string_view name) {
  return std::string{name} + "_self$";
}

std::string FunctionData::get_name_of_self_method() const {
  return get_name_of_self_method(local_name());
}

int FunctionData::get_min_argn() const {
  int min_argn = 0;
  for (auto p : get_params()) {
    if (p.as<op_func_param>()->has_default_value()) {
      break;
    }
    min_argn++;
  }

  return min_argn;
}

std::string FunctionData::get_resumable_path() const {
  std::vector<std::string> names;
  FunctionPtr f = fork_prev;
  while (f) {
    names.push_back(f->as_human_readable());
    f = f->fork_prev;
  }
  std::reverse(names.begin(), names.end());
  names.push_back(TermStringFormat::paint(name, TermStringFormat::red));
  f = wait_prev;
  while (f) {
    names.push_back(f->as_human_readable());
    f = f->wait_prev;
  }
  return vk::join(names, " -> ");
}

std::string FunctionData::get_throws_call_chain() const {
  if (!can_throw()) {
    return "";
  }
  std::vector<std::string> names;
  FunctionPtr f = throws_reason;
  Location last;
  names.push_back(as_human_readable());
  if (f) {
    while (true) {
      names.push_back(f->as_human_readable());
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
  chain.emplace_front(fun->as_human_readable());
  while (!fun->performance_inspections_for_warning.has_their_own_inspections()) {
    auto caller_it = std::find_if(fun->performance_inspections_for_warning_parents.begin(), fun->performance_inspections_for_warning_parents.end(), check_inspection);
    kphp_assert(caller_it != fun->performance_inspections_for_warning_parents.end());
    fun = *caller_it;
    chain.emplace_front(fun->as_human_readable());
  }

  return vk::join(chain, " -> ");
}

// some internal functions may appear in error messages;
// we don't want to print something like _explode_nth() instead of expode()
// that would be unhelpful for the user
static const std::unordered_map<vk::string_view, std::string> internal_function_pretty_names = {
  {"_explode_nth", "explode"},
  {"_explode_1", "explode"},
  {"_microtime_float", "microtime"},
  {"_microtime_string", "microtime"},
  {"_hrtime_int", "hrtime"},
  {"_hrtime_array", "hrtime"},
};

std::string FunctionData::as_human_readable(bool add_details) const {
  std::vector<VertexAdaptor<op_func_param>> params;
  for (auto p : get_params()) {
    if (p.as<op_func_param>()->var()->extra_type != op_ex_var_this && p->extra_type != op_ex_param_from_use) {
      params.emplace_back(p.as<op_func_param>());
    }
  }

  std::string params_names_str = vk::join(params, ",", [](auto p) { return "$" + p->var()->str_val; });
  std::string result_name;

  if (is_lambda()) {
    result_name = "function(" + params_names_str + ")";
  } else if (modifiers.is_instance() && class_id && class_id->is_lambda_class() && local_name() == "__invoke") {
    result_name = "function(" + params_names_str + ")";
  } else if (modifiers.is_instance() && class_id && class_id->is_typed_callable_interface() && local_name() == "__invoke") {
    result_name = "callable(" + vk::join(params, ",", [](auto p) { return p->type_hint->as_human_readable(); }) + "):" + return_typehint->as_human_readable();
  } else if (instantiationTs && outer_function) {
    result_name = outer_function->as_human_readable(add_details);
    result_name.erase(result_name.rfind('<'));
    result_name += "<" + vk::join(*outer_function->genericTs, ", ", [this](const GenericsDeclarationMixin::GenericsItem &item) {
      const TypeHint *itemT = instantiationTs->find(item.nameT);
      return itemT == nullptr ? "nullptr" : itemT->as_human_readable();
    }) + ">";
    if (add_details) {
      SrcFilePtr file = instantiationTs->location.get_file();
      std::string line = std::to_string(instantiationTs->location.line);
      result_name += " (instantiated at " + (file ? file->relative_file_name : "unknown file") + ":" + line + ")";
    }
  } else {

    auto pos1 = name.find("$$");
    auto pos2 = name.rfind("$$");

    // if name is just a function "function_name", doesn't belong to any class
    if (pos1 == std::string::npos) {
      if (is_internal) {
        const auto &pretty_name = internal_function_pretty_names.find(name);
        result_name = pretty_name != internal_function_pretty_names.end() ? pretty_name->second : name;
      } else {
        result_name = name;
      }
    }
      // if name is "class$$method"
    else if (pos2 == pos1) {
      std::string base_class = replace_characters(name.substr(0, pos1), '$', '\\');
      std::string method_name = name.substr(pos1 + 2);
      result_name = base_class + "::" + method_name;
    }
      // if name is "class$$method$$context"
    else {
      std::string base_class = replace_characters(name.substr(0, pos1), '$', '\\');
      std::string method_name = name.substr(pos1 + 2, pos2 - pos1 - 2);
      std::string context_class = replace_characters(name.substr(pos2 + 2), '$', '\\');
      result_name = add_details ? base_class + "::" + method_name + " (static=" + context_class + ")" : context_class + "::" + method_name;
    }

    if (genericTs) {
      result_name += genericTs->as_human_readable();
    }
  }

  return result_name;
}

bool FunctionData::is_imported_from_static_lib() const {
  return file_id && file_id->owner_lib && !file_id->owner_lib->is_raw_php() && !is_main_function();
}

bool FunctionData::is_invoke_method() const {
  return modifiers.is_instance() && (class_id->is_lambda_class() || class_id->is_typed_callable_interface()) && vk::string_view{name}.ends_with("__invoke");
}

vk::string_view FunctionData::get_tmp_string_specialization() const {
  static std::unordered_map<vk::string_view, vk::string_view> funcs = {
    {"substr", "_tmp_substr"},
    {"trim", "_tmp_trim"},
    // TODO: ltrim, rtrim, strstr
  };
  auto it = funcs.find(name);
  return it == funcs.end() ? "" : it->second;
}

VertexAdaptor<op_func_param> FunctionData::find_param_by_name(vk::string_view var_name) {
  for (auto p : get_params()) {
    if (p.as<op_func_param>()->var()->str_val == var_name) {
      return p.as<op_func_param>();
    }
  }
  return {};
}

VertexAdaptor<op_var> FunctionData::find_use_by_name(const std::string &var_name) {
  for (auto var_as_use : uses_list) {
    if (var_as_use->str_val == var_name) {
      return var_as_use;
    }
  }
  return {};
}

VarPtr FunctionData::find_var_by_name(const std::string &var_name) {
  auto search_in_vector = [&var_name](const std::vector<VarPtr> &lookup_vars_list) -> VarPtr {
    auto var_it = std::find_if(lookup_vars_list.begin(), lookup_vars_list.end(),
                               [&var_name](VarPtr var) { return var->name == var_name; });
    return var_it == lookup_vars_list.end() ? VarPtr{} : *var_it;
  };

  VarPtr      found = search_in_vector(local_var_ids);
  if (!found) found = search_in_vector(global_var_ids);
  if (!found) found = search_in_vector(static_var_ids);
  if (!found) found = search_in_vector(param_ids);
  return found ?: VarPtr{};
}

bool FunctionData::does_need_codegen() const {
  return type != FunctionData::func_class_holder &&
         type != FunctionData::func_extern &&
         (body_seq != body_value::empty || G->get_main_file()->main_function == get_self() || is_lambda());
}

bool operator<(FunctionPtr a, FunctionPtr b) {
  return a->name < b->name;
}
