#include "compiler/data.h"
#include <regex>

#include "compiler/compiler-core.h"
#include "compiler/gentree.h"
#include "compiler/type-inferer.h"

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
  dependency_level(0) {}

void VarData::set_uninited_flag(bool f) {
  uninited_flag = f;
}

bool VarData::get_uninited_flag() {
  return uninited_flag;
}

string VarData::get_human_readable_name() const {
  return (this->class_id ? (this->class_id->name + " :: $" + this->name) : "$" + this->name);
}

const ClassMemberStaticField *VarData::as_class_static_field() const {
  kphp_assert(is_class_static_var() && class_id);
  return class_id->members.get_static_field(get_local_name_from_global_$$(name));
}

const ClassMemberInstanceField *VarData::as_class_instance_field() const {
  kphp_assert(is_class_instance_var() && class_id);
  return class_id->members.get_instance_field(name);
}

/*** ClassData ***/
ClassData::ClassData() :
  id(0),
  class_type(ctype_class),
  assumptions_inited_vars(0),
  was_constructor_invoked(false),
  members(this) {
}

void ClassData::set_name_and_src_name(const string &name) {
  this->name = name;
  this->src_name = std::string("C$").append(replace_backslashes(name));
  this->header_name = replace_characters(src_name + ".h", '$', '@');
}

void ClassData::debugPrint() {
  string str_class_type =
    class_type == ctype_interface ? "interface" :
    class_type == ctype_trait ? "trait" :
    "class";
  printf("=== %s %s\n", str_class_type.c_str(), name.c_str());

  members.for_each([](ClassMemberConstant &m) {
    printf("const %s\n", m.local_name().c_str());
  });
  members.for_each([](ClassMemberStaticField &m) {
    printf("static $%s\n", m.local_name().c_str());
  });
  members.for_each([](ClassMemberStaticMethod &m) {
    printf("static %s()\n", m.local_name().c_str());
  });
  members.for_each([](ClassMemberInstanceField &m) {
    printf("var $%s\n", m.local_name().c_str());
  });
  members.for_each([](ClassMemberInstanceMethod &m) {
    printf("method %s()\n", m.local_name().c_str());
  });
}

std::string ClassData::get_name_of_invoke_function_for_extern(FunctionPtr extern_function) const {
  std::string invoke_method_name = replace_backslashes(name) + "$$" + "__invoke";
  for (size_t i = 0; i < extern_function->min_argn; ++i) {
    invoke_method_name += "$" + std::to_string(i) + "not_instance";
  }
  return invoke_method_name;
}

FunctionPtr ClassData::get_invoke_function_for_extern_function(FunctionPtr extern_function) const {
  /**
   * Will be rewritten in future
   * need finding `invoke` method which conforms to parameters types of extern_function
   */
  kphp_assert(extern_function->is_extern);
  std::string invoke_method_name = get_name_of_invoke_function_for_extern(extern_function);
  auto found_method = members.find_member([&](const ClassMemberInstanceMethod &f) {
    return f.global_name() == invoke_method_name;
  });

  return found_method ? found_method->function : FunctionPtr();
}

FunctionPtr ClassData::get_template_of_invoke_function() const {
  kphp_assert(new_function->is_lambda());
  auto found_method = members.find_member([&](const ClassMemberInstanceMethod &f) {
    return f.local_name() == "__invoke";
  });
  // это может быть is_template, а может быть и нет

  return found_method ? found_method->function : FunctionPtr();
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
  is_template(false) {}

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
  is_template(false) {}

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
  VertexAdaptor<op_func_param_list> param_list = func->root.as<meta_op_function>()->params();
  VertexRange func_args = param_list->params();
  size_t func_args_n = func_args.size();

  FunctionPtr new_function(new FunctionData());
  auto new_func_root =  func->root.as<op_function>().clone();

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

/*** DefineData ***/
DefineData::DefineData() :
  id(),
  val(nullptr),
  type_(def_raw) {}

DefineData::DefineData(VertexPtr val, DefineType type_) :
  id(),
  val(val),
  type_(type_) {}
