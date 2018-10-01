#include "compiler/data.h"

#include <functional>

#include "compiler/bicycle.h"
#include "compiler/io.h"

//IdGen <VertexPtr> tree_id_gen;
//BikeIdGen <VertexPtr> bike_tree_id_gen;



/*** VarData ***/
VarData::VarData(VarData::Type type_) :
  type_(type_),
  id(-1),
  param_i(),
  tinf_node(VarPtr(this)),
  init_val(nullptr),
  static_id(),
  bad_vars(nullptr),
  is_constant(false),
  is_reference(false),
  uninited_flag(false),
  optimize_flag(false),
  tinf_flag(false),
  global_init_flag(false),
  needs_const_iterator_flag(false),
  marked_as_global(false),
  access_type(access_nonmember),
  dependency_level(0),
  phpdoc_token() {}

void VarData::set_uninited_flag(bool f) {
  uninited_flag = f;
}

bool VarData::get_uninited_flag() {
  return uninited_flag;
}

/*** ClassData ***/
ClassData::ClassData() :
  id(0),
  assumptions_inited_vars(0),
  was_constructor_invoked(false) {      // иначе в нет гарантии, что в примитивных типах не окажется мусор
}


/*** FunctionData ***/
FunctionData::FunctionData() :
  id(0),
  root(nullptr),
  is_required(false),
  type_(func_local),
  bad_vars(nullptr),
  assumptions_inited_args(),
  assumptions_inited_return(),
  file_id(),
  class_id(),
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
  namespace_name(""),
  class_name("") {}

FunctionData::FunctionData(VertexPtr root) :
  id(0),
  root(root),
  is_required(false),
  type_(func_local),
  bad_vars(nullptr),
  assumptions_inited_args(),
  assumptions_inited_return(),
  file_id(),
  class_id(),
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
  namespace_name(""),
  class_name("") {}

FunctionPtr FunctionData::create_function(const FunctionInfo &info) {
  VertexAdaptor<meta_op_function> function_root = info.root;
  AUTO_PROF (create_function);
  string function_name = function_root->name().as<op_func_name>()->str_val;
  FunctionPtr function = FunctionPtr(new FunctionData());

  function->name = function_name;
  function->root = function_root;
  function->namespace_name = info.namespace_name;
  function->class_name = info.class_name;
  function->class_context_name = info.class_context;
  function->class_extends = info.extends;
  function->namespace_uses = info.namespace_uses;
  function->access_type = info.access_type;
  function_root->set_func_id(function);
  function->file_id = stage::get_file();
  function->kphp_required = info.kphp_required;

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
  VertexAdaptor<op_func_param_list> param_list = func->root.as<meta_op_function>()->params();
  VertexRange func_args = param_list->params();
  size_t func_args_n = func_args.size();

  FunctionPtr new_function(new FunctionData());
  auto new_func_root =  func->root.as<op_function>().clone();

  for (size_t i = 0; i < func_args_n; ++i) {
    VertexAdaptor<op_func_param> param = new_func_root->params()->ith(i).as<op_func_param>();
    auto id_classPtr_it = template_type_id_to_ClassPtr.find(param->template_type_id);
    if (id_classPtr_it == template_type_id_to_ClassPtr.end()) {
      kphp_assert(param->template_type_id == -1 || template_type_id_to_ClassPtr.empty());
      param->template_type_id = -1;
      continue;
    }
    param->template_type_id = -1;

    const std::pair<AssumType, ClassPtr> &assum_and_class = id_classPtr_it->second;
    if (assum_and_class.first != assum_not_instance) {
      new_function->assumptions.emplace_back(assum_and_class.first, param->var()->get_string(), assum_and_class.second);
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
  new_function->class_name = func->class_name;
  new_function->class_context_name = func->class_context_name;
  new_function->class_extends = func->class_extends;
  new_function->access_type = func->access_type;
  new_function->namespace_uses = func->namespace_uses;
  new_function->is_template = false;
  new_function->name = name_of_function_instance;

  std::function<void(VertexPtr, FunctionPtr)> set_location_for_all = [&set_location_for_all](VertexPtr root, FunctionPtr function_location) {
    root->location.function = function_location;
    for (VertexPtr &v : *root) {
      set_location_for_all(v, function_location);
    }
  };
  set_location_for_all(new_func_root, new_function);

  return new_function;
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

/*** DefineData ***/
DefineData::DefineData() :
  id(),
  val(nullptr),
  type_(def_raw) {}

DefineData::DefineData(VertexPtr val, DefineType type_) :
  id(),
  val(val),
  type_(type_) {}
