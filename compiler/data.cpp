#include "compiler/data.h"

#include "compiler/bicycle.h"

//IdGen <VertexPtr> tree_id_gen;
//BikeIdGen <VertexPtr> bike_tree_id_gen;



/*** VarData ***/
VarData::VarData (VarData::Type type_) :
  type_ (type_),
  id (-1),
  tinf_node (VarPtr (this)),
  init_val (NULL),
  static_id(),
  bad_vars (NULL),
  is_constant (false),
  is_reference (false),
  uninited_flag (false),
  optimize_flag (false),
  tinf_flag (false),
  global_init_flag (false),
  needs_const_iterator_flag (false),
  dependency_level(0) {
}

void VarData::set_uninited_flag (bool f) {
  uninited_flag = f;
}
bool VarData::get_uninited_flag() {
  return uninited_flag;
}

/*** ClassData ***/
ClassData::ClassData () :
    id(0),
    assumptions_inited_vars(0) {      // иначе в нет гарантии, что в примитивных типах не окажется мусор
}

/*** FunctionSet ***/
int FunctionSet::size() {
  return (int)functions.size();
}

FunctionPtr FunctionSet::operator[] (int i) {
  kphp_assert (0 <= i && i < size());
  return functions[i];
}

FunctionSet::FunctionSet()
  : is_required (false) {
}

bool FunctionSet::add_function (FunctionPtr new_function) {
  std::vector <FunctionPtr>::iterator match = std::find (functions.begin(), functions.end(), new_function);
  if (match != functions.end()) {
    return false;
  }

  functions.push_back (new_function);
  return true;
}



/*** FunctionData ***/
FunctionData::FunctionData () :
  root (NULL),
  is_required (false),
  type_ (func_local),
  bad_vars (NULL),
  file_id (),
  req_id(),
  class_id(),
  varg_flag (false),
  tinf_state (0),
  const_data (NULL),
  min_argn (0),
  is_extern (false),
  used_in_source (false),
  is_callback (false),
  namespace_name (""),
  class_name ("") {
}

FunctionData::FunctionData (VertexPtr root) :
  root (root),
  is_required (false),
  type_ (func_local),
  bad_vars (NULL),
  file_id (),
  req_id(),
  class_id(),
  varg_flag (false),
  tinf_state (0),
  const_data (NULL),
  min_argn (0),
  is_extern (false),
  used_in_source (false),
  is_callback (false),
  namespace_name (""),
  class_name ("") {
}

bool FunctionData::is_static_init_empty_body() const {
  FOREACH (const_var_ids, i) {
    if (!(*i)->global_init_flag) {
      return false;
    }
  }
  FOREACH (header_const_var_ids, i) {
    if (!(*i)->global_init_flag) {
      return false;
    }
  }
  return true;
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
DefineData::DefineData ()
  : val (NULL) {
}

DefineData::DefineData (VertexPtr val, DefineType type_)
  : val (val), type_ (type_) {
}
