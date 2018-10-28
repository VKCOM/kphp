#include "compiler/data/function-data.h"

#include <regex>

#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"
#include "compiler/io.h"
#include "compiler/type-inferer.h"
#include "compiler/vertex.h"

FunctionData::FunctionData() :
  id(0),
  root(nullptr),
  is_required(false),
  type_(func_local),
  bad_vars(nullptr),
  assumptions_inited_args(),
  assumptions_inited_return(),
  varg_flag(false),
  tinf_state(0),
  const_data(nullptr),
  phpdoc_token(),
  min_argn(0),
  is_extern(false),
  used_in_source(false),
  is_callback(false),
  should_be_sync(),
  kphp_required(false),
  is_template(false),
  access_type(access_nonmember) {}

FunctionData::FunctionData(VertexPtr root) :
  id(0),
  root(root),
  is_required(false),
  type_(func_local),
  bad_vars(nullptr),
  assumptions_inited_args(),
  assumptions_inited_return(),
  varg_flag(false),
  tinf_state(0),
  const_data(nullptr),
  phpdoc_token(),
  min_argn(0),
  is_extern(false),
  used_in_source(false),
  is_callback(false),
  should_be_sync(),
  kphp_required(false),
  is_template(false),
  access_type(access_nonmember) {}

FunctionPtr FunctionData::create_function(const FunctionInfo &info) {
  VertexAdaptor<meta_op_function> function_root = info.root;
  AUTO_PROF (create_function);
  string function_name = function_root->name().as<op_func_name>()->str_val;
  FunctionPtr function = FunctionPtr(new FunctionData());

  function->name = function_name;
  function->root = function_root;
  function->namespace_name = info.namespace_name;
  function->class_context_name = info.class_context;
  function->access_type = info.access_type;
  function_root->set_func_id(function);
  function->file_id = stage::get_file();
  function->kphp_required = info.kphp_required;
  function->set_function_in_which_lambda_was_created(function_root->get_func_id()->function_in_which_lambda_was_created);

  if (function_root->type() == op_func_decl) {
    function->is_extern = true;
    function->type() = FunctionData::func_extern;
  } else {
    switch (function_root->extra_type) {
      case op_ex_func_switch:
        function->type() = FunctionData::func_switch;
        break;
      case op_ex_func_global:
        function->type() = FunctionData::func_global;
        break;
      default:
        function->type() = FunctionData::func_local;
        break;
    }
  }

  if (function->type() == FunctionData::func_global) {
    if (stage::get_file()->main_func_name == function->name) {
      stage::get_file()->main_function = function;
    }
  }

  return function;
}

FunctionPtr FunctionData::generate_instance_of_template_function(const std::map<int, std::pair<AssumType, ClassPtr>> &template_type_id_to_ClassPtr,
                                                                 FunctionPtr func,
                                                                 const std::string &name_of_function_instance) {
  kphp_assert_msg(func->is_template, "function must be template");
  VertexAdaptor<op_func_param_list> param_list = func->root.as<meta_op_function>()->params();
  VertexRange func_args = param_list->params();
  size_t func_args_n = func_args.size();

  FunctionPtr new_function(new FunctionData());
  auto new_func_root = func->root.as<op_function>().clone();

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
    if (assum_and_class.first != assum_not_instance) {
      new_function->assumptions_for_vars.emplace_back(assum_and_class.first, param->var()->get_string(), assum_and_class.second);
      new_function->assumptions_inited_args = 2;
    }
  }

  new_func_root->name()->set_string(name_of_function_instance);

  new_function->root = new_func_root;
  new_function->root->set_func_id(new_function);
  new_function->is_required = true;
  new_function->type() = func->type();
  new_function->file_id = func->file_id;
  new_function->class_id = func->class_id;
  new_function->varg_flag = func->varg_flag;
  new_function->tinf_state = func->tinf_state;
  new_function->const_data = func->const_data;
  new_function->phpdoc_token = func->phpdoc_token;
  new_function->min_argn = func->min_argn;
  new_function->is_extern = func->is_extern;
  new_function->used_in_source = func->used_in_source;
  new_function->kphp_required = true;
  new_function->namespace_name = func->namespace_name;
  new_function->class_context_name = func->class_context_name;
  new_function->access_type = func->access_type;
  new_function->is_template = false;
  new_function->name = name_of_function_instance;
  new_function->function_in_which_lambda_was_created = func->function_in_which_lambda_was_created;

  std::function<void(VertexPtr, FunctionPtr)> set_location_for_all = [&set_location_for_all](VertexPtr root, FunctionPtr function_location) {
    root->location.function = function_location;
    for (VertexPtr &v : *root) {
      set_location_for_all(v, function_location);
    }
  };
  set_location_for_all(new_func_root, new_function);

  return new_function;
}

ClassPtr FunctionData::is_lambda(VertexPtr v) {
  if (v->type() == op_func_ptr && v->get_func_id()) {
    return is_lambda(v->get_func_id()->root);
  }

  if (tinf::Node *tinf_node = get_tinf_node(v)) {
    if (tinf_node->get_type()->ptype() != tp_Unknown) {
      if (ClassPtr klass = tinf_node->get_type()->class_type()) {
        return klass->new_function->is_lambda() ? klass : ClassPtr{};
      }
      return ClassPtr{};
    }
  }

  switch (v->type()) {
    case op_function:
    case op_constructor_call: {
      if (v->get_func_id()->is_lambda()) {
        return v->get_func_id()->class_id;
      }
      return {};
    }

    case op_func_call:
    case op_var: {
      ClassPtr c;
      AssumType assum = infer_class_of_expr(stage::get_function(), v, c);
      if (assum == assum_instance && c->new_function && c->new_function->is_lambda()) {
        return c;
      }
      return {};
    }

    default:
      return {};
  }
}

bool FunctionData::is_static_init_empty_body() const {
  auto global_init_flag_is_set = [](const VarPtr &v) { return v->global_init_flag; };

  return std::all_of(const_var_ids.begin(), const_var_ids.end(), global_init_flag_is_set) &&
         std::all_of(header_const_var_ids.begin(), header_const_var_ids.end(), global_init_flag_is_set);
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

string FunctionData::get_human_readable_name() const {
  if (this->access_type == access_nonmember) {
    return this->name;
  }
  std::smatch matched;
  if (std::regex_match(this->name, matched, std::regex("(.+)\\$\\$(.+)\\$\\$(.+)"))) {
    string base_class = matched[1].str(), actual_class = matched[3].str();
    base_class = replace_characters(base_class, '$', '\\');
    actual_class = replace_characters(actual_class, '$', '\\');
    return actual_class + " :: " + matched[2].str() + " (" + "inherited from " + base_class + ")";
  }
  //Модифицировать вывод осторожно! По некоторым символам используется поиск регекспами при выводе стектрейса
  return std::regex_replace(std::regex_replace(this->name, std::regex("\\$\\$"), " :: "), std::regex("\\$"), "\\");
}


VertexRange FunctionData::get_params() {
  return ::get_function_params(root.as<meta_op_function>());
}
bool FunctionData::is_constructor() const {
  return class_id && class_id->new_function && &*(class_id->new_function) == this;
}

bool operator<(FunctionPtr a, FunctionPtr b) {
  return a->name < b->name;
}
