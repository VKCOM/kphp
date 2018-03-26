#include "compiler/compiler-core.h"

#include "compiler/gentree.h"
#include "compiler/make.h"
#include "compiler/pass-register-vars.hpp"

CompilerCore::CompilerCore() :
  env_ (NULL) {
}
void CompilerCore::start() {
  PROF (total).start();
  stage::die_if_global_errors();
  load_index();
}

void CompilerCore::finish() {
  if (stage::warnings_count > 0) {
    printf("[%d WARNINGS GENERATED]\n", stage::warnings_count);
  }
  stage::die_if_global_errors();
  del_extra_files();
  save_index();
  stage::die_if_global_errors();

  delete env_;
  env_ = NULL;

  PROF (total).finish();
}
void CompilerCore::register_env (KphpEnviroment *env) {
  kphp_assert (env_ == NULL);
  env_ = env;
}
const KphpEnviroment &CompilerCore::env() {
  kphp_assert (env_ != NULL);
  return *env_;
}


bool CompilerCore::add_to_function_set (FunctionSetPtr function_set, FunctionPtr function,
    bool req) {
  AutoLocker <FunctionSetPtr> locker (function_set);
  if (req) {
    kphp_assert (function_set->size() == 0);
    function_set->is_required = true;
  }
  function->function_set = function_set;
  function_set->add_function (function);
  return function_set->is_required;
}

FunctionSetPtr CompilerCore::get_function_set (function_set_t type __attribute__((unused)), const string &name, bool force) {
  HT <FunctionSetPtr> *ht = &function_set_ht;

  HT <FunctionSetPtr>::HTNode *node = ht->at (hash_ll (name));
  if (node->data.is_null()) {
    if (!force) {
      return FunctionSetPtr();
    }
    AutoLocker <Lockable *> locker (node);
    if (node->data.is_null()) {
      FunctionSetPtr new_func_set = FunctionSetPtr (new FunctionSet());
      new_func_set->name = name;
      node->data = new_func_set;
    }
  }
  FunctionSetPtr function_set = node->data;
  kphp_assert (function_set->name == name/*, "Bug in compiler: hash collision"*/);
  return function_set;
}
FunctionPtr CompilerCore::get_function_unsafe (const string &name) {
  FunctionSetPtr func_set = get_function_set (fs_function, name, true);
  kphp_assert (func_set->size() == 1);
  FunctionPtr func = func_set[0];
  kphp_assert (func.not_null());
  return func;
}

FunctionPtr CompilerCore::create_function (const FunctionInfo &info) {
  VertexAdaptor <meta_op_function>  function_root = info.root;
  AUTO_PROF (create_function);
  string function_name = function_root->name().as <op_func_name>()->str_val;
  FunctionPtr function = FunctionPtr (new FunctionData());

  function->name = function_name;
  function->root = function_root;
  function->namespace_name = info.namespace_name;
  function->class_name = info.class_name;
  function->class_context_name = info.class_context;
  function->class_extends = info.extends;
  function->namespace_uses = info.namespace_uses;
  function->disabled_warnings = info.disabled_warnings;
  function_root->set_func_id (function);
  function->file_id = stage::get_file();

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

ClassPtr CompilerCore::create_class(const ClassInfo &info) {
  ClassPtr klass = ClassPtr (new ClassData());
  klass->name = (info.namespace_name.empty() ? "" : info.namespace_name + "\\") + info.name;
  klass->file_id = stage::get_file();
  klass->root = info.root;
  klass->extends = info.extends;

  string init_function_name_str = stage::get_file()->main_func_name;
  klass->init_function = get_function_unsafe (init_function_name_str);
  klass->init_function->class_id = klass;
  klass->static_fields.insert(info.static_fields.begin(), info.static_fields.end());
  FOREACH(info.constants, i) {
    klass->constants.insert((*i).first);
  }
  FOREACH(info.static_methods, method_ptr) {
    (*method_ptr).second->class_id = klass;
    if ((*method_ptr).second == klass->init_function) {
      continue;
    }
    klass->static_methods.insert(*method_ptr);
  }
  return klass;
}

string CompilerCore::unify_file_name (const string &file_name) {
  if (env().get_base_dir().empty()) { //hack: directory of first file will be used ad base_dir
    size_t i = file_name.find_last_of ("/");
    kphp_assert (i != string::npos);
    env_->set_base_dir (file_name.substr (0, i + 1));
  }
  const string &base_dir = env().get_base_dir();
  if (strncmp (file_name.c_str(), base_dir.c_str(), base_dir.size())) {
    return file_name;
  }
  return file_name.substr (base_dir.size());
}

SrcFilePtr CompilerCore::register_file (const string &file_name, const string &context) {
  if (file_name.empty()) {
    return SrcFilePtr();
  }

  //search file
  string full_file_name;
  if (file_name[0] != '/' && file_name[0] != '.') {
    int n = (int)env().get_includes().size();
    for (int i = 0; i < n && full_file_name.empty(); i++) {
      full_file_name = get_full_path (env().get_includes()[i] + file_name);
    }
  }
  if (file_name[0] == '/') {
    full_file_name = get_full_path (file_name);
  } else if (full_file_name.empty()) {
    vector <string> cur_include_dirs;
    SrcFilePtr from_file = stage::get_file();
    if (from_file.not_null()) {
      string from_file_name = from_file->file_name;
      size_t en = from_file_name.find_last_of ('/');
      assert (en != string::npos);
      string cur_dir = from_file_name.substr (0, en + 1);
      cur_include_dirs.push_back (cur_dir);
    }
    if (from_file.is_null() || file_name[0] != '.') {
      cur_include_dirs.push_back ("");
    }
    int n = (int)cur_include_dirs.size();
    for (int i = 0; i < n && full_file_name.empty(); i++) {
      full_file_name = get_full_path (cur_include_dirs[i] + file_name);
    }
  }

  kphp_error_act (
    !full_file_name.empty(),
    dl_pstr ("Cannot load file [%s]", file_name.c_str()),
    return SrcFilePtr()
  );


  //find short_file_name
  int st = -1;
  int en = (int)full_file_name.size();
  for (int i = en - 1; i > st; i--) {
    if (full_file_name[i] == '/') {
      st = i;
      break;
    }
  }
  st++;

  int dot_pos = en;
  for (int i = st; i < en; i++) {
    if (full_file_name[i] == '.') {
      dot_pos = i;
    }
  }
  //TODO: en == full_file_name.size()
  string short_file_name = full_file_name.substr (st, dot_pos - st);
  for (int i = 0; i < (int)short_file_name.size(); i++) {
    if (short_file_name[i] == '.') {
    }
  }
  string extension = full_file_name.substr (min (en, dot_pos + 1));
  if (extension != "php") {
    short_file_name += "_";
    short_file_name += extension;
  }

  //register file if needed
  HT <SrcFilePtr>::HTNode *node = file_ht.at (hash_ll (full_file_name + context));
  if (node->data.is_null()) {
    AutoLocker <Lockable *> locker (node);
    if (node->data.is_null()) {
      SrcFilePtr new_file = SrcFilePtr (new SrcFile (full_file_name, short_file_name, context));
      char tmp[50];
      sprintf (tmp, "%x", hash (full_file_name + context));
      string func_name = gen_unique_name ("src$" + new_file->short_file_name + tmp, true);
      new_file->main_func_name = func_name;
      new_file->unified_file_name = unify_file_name (new_file->file_name);
      size_t last_slash = new_file->unified_file_name.rfind("/");
      new_file->unified_dir_name = last_slash == string::npos ? "" : new_file->unified_file_name.substr(0, last_slash);
      node->data = new_file;
    }
  }
  SrcFilePtr file = node->data;
  return file;
}

ClassPtr CompilerCore::get_class(const string &name) {
  return classes_ht.at (hash_ll (name))->data;
}

bool CompilerCore::register_define (DefinePtr def_id) {
  HT <DefinePtr>::HTNode *node = defines_ht.at (hash_ll (def_id->name));
  AutoLocker <Lockable *> locker (node);

  kphp_error_act (
    node->data.is_null(),
    dl_pstr ("Redeclaration of define [%s], the previous declaration was in [%s]",
        def_id->name.c_str(),
        node->data->file_id->file_name.c_str()),
    return false
  );

  node->data = def_id;
  return true;
}

DefinePtr CompilerCore::get_define (const string &name) {
  return defines_ht.at (hash_ll (name))->data;
}
VarPtr CompilerCore::create_var (const string &name, VarData::Type type) {
  VarPtr var = VarPtr (new VarData (type));
  var->name = name;
  return var;
}

VarPtr CompilerCore::get_global_var (const string &name, VarData::Type type,
    VertexPtr init_val) {
  HT <VarPtr>::HTNode *node = global_vars_ht.at (hash_ll (name));
  VarPtr new_var;
  if (node->data.is_null()) {
    AutoLocker <Lockable *> locker (node);
    if (node->data.is_null()) {
      new_var = create_var (name, type);
      new_var->init_val = init_val;
      new_var->is_constant = type == VarData::var_const_t;
      node->data = new_var;
    }
  }
  VarPtr var = node->data;
  if (new_var.is_null()) {
    kphp_assert_msg(var->name == name, "bug in compiler (hash collision)");
    if (init_val.not_null()) {
      kphp_assert(var->init_val->type() == init_val->type());
      switch (init_val->type()) {
        case op_string:
          kphp_assert(var->init_val->get_string() == init_val->get_string());
          break;
        case op_conv_regexp: {
          string &new_regexp = init_val.as<op_conv_regexp>()->expr().as<op_string>()->str_val;
          string &hashed_regexp = var->init_val.as<op_conv_regexp>()->expr().as<op_string>()->str_val;
          string msg = "hash collision: " + new_regexp + "; " + hashed_regexp;

          kphp_assert_msg(hashed_regexp == new_regexp, msg.c_str());
          break;
        }
        case op_array: {
          string new_array_repr, hashed_array_repr;
          CollectConstVarsPass::serialize_array(init_val, &new_array_repr);
          CollectConstVarsPass::serialize_array(var->init_val, &hashed_array_repr);

          string msg = "hash collision: " + new_array_repr + "; " + hashed_array_repr;

          kphp_assert_msg(new_array_repr == hashed_array_repr, msg.c_str());
          break;
        }
        default:
          break;
      }
    }
  }
  return var;
}

VarPtr CompilerCore::create_local_var (FunctionPtr function, const string &name,
    VarData::Type type) {
  VarData::Type real_type = type;
  if (type == VarData::var_static_t) {
    real_type = VarData::var_global_t;
  }
  VarPtr var = create_var (name, real_type);
  var->holder_func = function;
  switch (type) {
    case VarData::var_local_t:
      function->local_var_ids.push_back (var);
      break;
    case VarData::var_static_t:
      function->static_var_ids.push_back (var);
      break;
    case VarData::var_param_t:
      var->param_i = (int)function->param_ids.size();
      function->param_ids.push_back (var);
      break;
    default:
      kphp_fail();
  }
  return var;
}

const vector <SrcFilePtr> &CompilerCore::get_main_files() {
  return main_files;
}
vector <VarPtr> CompilerCore::get_global_vars() {
  return global_vars_ht.get_all();
}

vector <ClassPtr> CompilerCore::get_classes() {
  return classes_ht.get_all();
}

void CompilerCore::load_index() {
  string index_path = env().get_index();
  if (index_path.empty()) {
    return;
  }
  FILE *f = fopen (index_path.c_str(), "r");
  if (f == NULL) {
    return;
  }
  cpp_index.load (f);
  fclose (f);
}

void CompilerCore::save_index() {
  string index_path = env().get_index();
  if (index_path.empty()) {
    return;
  }
  string tmp_index_path = index_path + ".tmp";
  FILE *f = fopen (tmp_index_path.c_str(), "w");
  if (f == NULL) {
    return;
  }
  cpp_index.save (f);
  fclose (f);
  int err = system (("mv " + tmp_index_path + " " + index_path).c_str());
  kphp_error (err == 0, "Failed to rewrite index");
  kphp_fail();
}

File *CompilerCore::get_file_info (const string &file_name) {
  return cpp_index.get_file (file_name, true);
}

void CompilerCore::del_extra_files() {
  cpp_index.del_extra_files();
}

void CompilerCore::init_dest_dir() {
  if (env().get_use_auto_dest()) {
    string name = main_files[0]->short_file_name;
    string hash_string;
    FOREACH (main_files, file) {
      hash_string += (*file)->file_name;
      hash_string += ";";
    }
    long long h = hash (hash_string);
    stringstream ss;
    ss << "auto." << name << "-" << std::hex << h;
    env_->set_dest_dir_subdir (ss.str());
  }
  env_->init_dest_dirs();
  cpp_dir = env().get_dest_cpp_dir();
  cpp_index.sync_with_dir (cpp_dir);
  cpp_dir = cpp_index.get_dir();
}

bool compare_mtime(File *f, File *g) {
  if (f->mtime != g->mtime) {
    return f->mtime > g->mtime;
  }
  return f->path < g->path;
}

bool kphp_make (File *bin, Index *obj_dir, Index *cpp_dir, 
    File *lib_version_file, File *link_file, const KphpEnviroment &kphp_env) {
  KphpMake make;
  long long lib_mtime = lib_version_file->mtime;
  if (lib_mtime == 0) {
    fprintf (stdout, "Can't read mtime of lib_version_file [%s]\n", 
        lib_version_file->path.c_str());
    return false;
  }
  long long link_mtime = link_file->mtime;
  if (link_mtime == 0) {
    fprintf (stdout, "Can't read mtime of link_file [%s]\n", 
        link_file->path.c_str());
    return false;
  }
  make.create_cpp_target (link_file);

  vector <File *> objs;
  vector <File *> files = cpp_dir->get_files();
  std::sort (files.begin(), files.end(), compare_mtime);
  vector<long long> header_mtime(files.size());
  for (size_t i = 0; i < files.size(); i++) {
    header_mtime[i] = files[i]->mtime;
  }

  for (size_t i = 0; i < files.size(); ++i) {
    File *h_file = files[i];
    if (h_file->ext == ".h") {
      long long &h_mtime = header_mtime[i];
      FOREACH (h_file->includes, it) {
        File *header = cpp_dir->get_file (*it, false);
        kphp_assert (header != NULL);
        kphp_assert (header->on_disk);
        long long dep_mtime = header_mtime[std::lower_bound (files.begin(), files.end(), header, compare_mtime) - files.begin()];
        h_mtime = std::max(h_mtime, dep_mtime);
      }
    }
  }
  if (G->env().get_use_subdirs()) {
    FOREACH (files, cpp_file_i) {
      File *cpp_file = *cpp_file_i;
      if (cpp_file->ext == ".cpp") {
        File *obj_file = obj_dir->get_file (cpp_file->name_without_ext + ".o", true);
        obj_file->compile_with_debug_info_flag = cpp_file->compile_with_debug_info_flag;
        make.create_cpp2obj_target (cpp_file, obj_file);
        Target *cpp_target = cpp_file->target;
        FOREACH (cpp_file->includes, it) {
          File *header = cpp_dir->get_file (*it, false);
          kphp_assert (header != NULL);
          kphp_assert (header->on_disk);
          long long dep_mtime = header_mtime[std::lower_bound (files.begin(), files.end(), header, compare_mtime) - files.begin()];
          cpp_target->force_changed (dep_mtime);
        }
        cpp_target->force_changed (lib_mtime);
        objs.push_back (obj_file);
      }
    }
    fprintf (stderr, "objs cnt = %d\n", (int)objs.size());

    map <string, vector <File *> > subdirs;
    vector <File *> tmp_objs;
    FOREACH (objs, obj_i) {
      File *obj_file = *obj_i;
      string name = obj_file->subdir;
      name = name.substr (0, (int)name.size() - 1);
      if (name.empty()) {
        tmp_objs.push_back (obj_file);
        continue;
      }
      name += ".o";
      subdirs[name].push_back (obj_file);
    }

    objs = tmp_objs;
    FOREACH (subdirs, it) {
      File *obj_file = obj_dir->get_file (it->first, true);
      make.create_objs2obj_target (it->second, obj_file);
      objs.push_back (obj_file);
    }
    fprintf (stderr, "objs cnt = %d\n", (int)objs.size());
    objs.push_back (link_file);
    make.create_objs2bin_target (objs, bin);
  } else {
    FOREACH (files, cpp_file_i) {
      File *cpp_file = *cpp_file_i;
      if (cpp_file->ext == ".cpp") {
        File *obj_file = obj_dir->get_file (cpp_file->name + ".o", true);
        Target *cpp_target = cpp_file->target;
        FOREACH (cpp_file->includes, it) {
          File *header = cpp_dir->get_file (*it, false);
          kphp_assert (header != NULL);
          kphp_assert (header->on_disk);
          long long dep_mtime = header_mtime[std::lower_bound (files.begin(), files.end(), header, compare_mtime) - files.begin()];
          cpp_target->force_changed (dep_mtime);
        }
        cpp_target->force_changed (lib_mtime);
        objs.push_back (obj_file);
      }
    }
    fprintf (stderr, "objs cnt = %d\n", (int)objs.size());
    objs.push_back (link_file);
    make.create_objs2bin_target (objs, bin);
  }
  make.init_env (kphp_env);
  return make.make_target (bin, kphp_env.get_jobs_count());
}

void CompilerCore::make() {
  AUTO_PROF (make);
  stage::set_name ("Make");
  cpp_index.del_extra_files();

  Index obj_index;
  obj_index.sync_with_dir (env().get_dest_objs_dir());

  File bin_file (env().get_binary_path());
  kphp_assert (bin_file.upd_mtime() >= 0);
  File lib_version_file (env().get_lib_version());
  kphp_assert (lib_version_file.upd_mtime() >= 0);
  File link_file (env().get_link_file());
  kphp_assert (link_file.upd_mtime() >= 0);

  if (env().get_make_force()) {
    obj_index.del_extra_files();
    bin_file.unlink();
  }

  bool ok = kphp_make (&bin_file, &obj_index, &cpp_index, 
      &lib_version_file, &link_file, env());
  kphp_error (ok, "Make failed");
  stage::die_if_global_errors();
  obj_index.del_extra_files();

  if (!env().get_user_binary_path().empty()) {
    File user_bin_file (env().get_user_binary_path());
    kphp_assert (user_bin_file.upd_mtime() >= 0);
    string cmd = "ln --force " + bin_file.path + " " + user_bin_file.path
      + " 2> /dev/null || cp " + bin_file.path + " " + user_bin_file.path;
    int err = system (cmd.c_str());
    if (env().get_verbosity() >= 3) {
      fprintf (stderr, "[%s]: %d %d\n", cmd.c_str(), err, WEXITSTATUS (err));
    }
    if (err == -1 || !WIFEXITED (err) || WEXITSTATUS (err)) {
      if (err == -1) {
        perror ("system failed");
      }
      kphp_error (0, dl_pstr ("Failed [%s]", cmd.c_str()));
      stage::die_if_global_errors();
    }
  }
}

CompilerCore *G;

bool try_optimize_var (VarPtr var) {
  return __sync_bool_compare_and_swap (&var->optimize_flag, false, true);
}

string conv_to_func_ptr_name(VertexPtr call) {
  VertexPtr name_v = GenTree::get_actual_value (call);
  string name;
  if (name_v->type() == op_string) {
    name = name_v.as <op_string>()->str_val;
  } else if (name_v->type() == op_func_name) {
    name = name_v.as <op_func_name>()->str_val;
  } else if (name_v->type() == op_array) {
    if (name_v->size() == 2) {
      VertexPtr class_name = name_v->ith(0);
      VertexPtr fun_name = name_v->ith(1);
      class_name = GenTree::get_actual_value(class_name);
      fun_name = GenTree::get_actual_value(fun_name);
      if (class_name->type() == op_string && fun_name->type() == op_string) {
        name = class_name.as<op_string>()->str_val + "::" + fun_name.as<op_string>()->str_val;
      }
    }
  }
  if (name.find("::") != string::npos && name[0] != '\\') {
    return "";
  }
  return name;
}

VertexPtr conv_to_func_ptr(VertexPtr call, FunctionPtr current_function) {
  if (call->type() != op_func_ptr) {
    string name = conv_to_func_ptr_name(call);
    if (!name.empty()) {
      name = get_full_static_member_name(current_function, name, true);
      CREATE_VERTEX (new_call, op_func_ptr);
      new_call->str_val = name;
      set_location (new_call, call->get_location());
      call = new_call;
    }
  }

  return call;
}

VertexPtr set_func_id (VertexPtr call, FunctionPtr func) {
  kphp_assert (call->type() == op_func_ptr || call->type() == op_func_call);
  kphp_assert (func.not_null());
  kphp_assert (call->get_func_id().is_null() || call->get_func_id() == func);
  if (call->get_func_id() == func) {
    return call;
  }
  //fprintf (stderr, "%s\n", func->name.c_str());

  call->set_func_id (func);
  if (call->type() == op_func_ptr) {
    func->is_callback = true;
    return call;
  }

  if (func->root.is_null()) {
    kphp_fail();
    return call;
  }

  VertexAdaptor <meta_op_function> func_root = func->root;
  VertexAdaptor <op_func_param_list> param_list = func_root->params();
  VertexRange call_args = call.as <op_func_call>()->args();
  VertexRange func_args = param_list->params();
  int call_args_n = (int)call_args.size();
  int func_args_n = (int)func_args.size();

  if (func->varg_flag) {
    for (int i = 0; i < call_args_n; i++) {
      kphp_error_act (
        call_args[i]->type() != op_func_name,
        "Unexpected function pointer",
        return VertexPtr()
      );
    }
    VertexPtr args;
    if (call_args_n == 1 && call_args[0]->type() == op_varg) {
      args = VertexAdaptor <op_varg> (call_args[0])->expr();
    } else {
      CREATE_VERTEX (new_args, op_array, call->get_next());
      new_args->location = call->get_location();
      args = new_args;
    }
    vector <VertexPtr> tmp (1, GenTree::conv_to <tp_array> (args));
    COPY_CREATE_VERTEX (new_call, call, op_func_call, tmp);
    return new_call;
  }

  for (int i = 0; i < call_args_n; i++) {
    if (i < func_args_n) {
      if (func_args[i]->type() == op_func_param) {
        if (call_args[i]->type() == op_func_name) {
          string msg = "Unexpected function pointer: " + call_args[i]->get_string();
          kphp_error(false, msg.c_str());
          continue;
        } else if (call_args[i]->type() == op_varg) {
          string msg = "function: `" + func->name +"` must takes variable-length argument list";
          kphp_error_act(false, msg.c_str(), break);
        }
        VertexAdaptor <op_func_param> param = func_args[i];
        if (param->type_help != tp_Unknown) {
          call_args[i] = GenTree::conv_to (call_args[i], param->type_help, param->var()->ref_flag);
        }
      } else if (func_args[i]->type() == op_func_param_callback) {
        call_args[i] = conv_to_func_ptr(call_args[i], stage::get_function());
        kphp_error (call_args[i]->type() == op_func_ptr, "Function pointer expected");
      } else {
        kphp_fail();
      }
    }
  }
  return call;
}

VertexPtr try_set_func_id (VertexPtr call) {
  if (call->type() != op_func_ptr && call->type() != op_func_call) {
    return call;
  }

  if (call->get_func_id().not_null()) {
    return call;
  }

  const string &name = call->get_string();
  FunctionSetPtr function_set = G->get_function_set (fs_function, name, true);
  FunctionPtr function;
  int functions_cnt = (int)function_set->size();

  kphp_error_act (
    functions_cnt != 0,
    dl_pstr ("Unknown function [%s]\n%s\n", name.c_str(), 
      stage::get_function_history().c_str()),
    return call
  );

  kphp_assert (function_set->is_required);

  if (functions_cnt == 1) {
    function = function_set[0];
  }

  kphp_error_act (
    function.not_null(),
    dl_pstr ("Function overloading is not supported properly [%s]", name.c_str()),
    return call
  );

  call = set_func_id (call, function);
  return call;
}
