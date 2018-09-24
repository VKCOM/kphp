#include "compiler/data.h"

#include "compiler/bicycle.h"

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
  is_actually_called(false),
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
  is_actually_called(false),
  namespace_name(""),
  class_name("") {}

bool FunctionData::is_static_init_empty_body() const {
  auto global_init_flag_is_set = [](const VarPtr &v) { return v->global_init_flag; };

  return std::all_of(const_var_ids.begin(), const_var_ids.end(), global_init_flag_is_set) &&
         std::all_of(header_const_var_ids.begin(), header_const_var_ids.end(), global_init_flag_is_set);
}

string FunctionData::get_resumable_path() const {
  vector<string> names;
  FunctionPtr f = fork_prev;
  while (f.not_null()) {
    names.push_back(f->name);
    f = f->fork_prev;
  }
  std::reverse(names.begin(), names.end());
  names.push_back(name);
  f = wait_prev;
  while (f.not_null()) {
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
