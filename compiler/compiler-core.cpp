#include "compiler/compiler-core.h"

#include <fstream>
#include <numeric>
#include <sys/sendfile.h>
#include <unordered_map>

#include "common/wrappers/mkdir_recursive.h"

#include "compiler/const-manipulations.h"
#include "compiler/data/class-data.h"
#include "compiler/data/define-data.h"
#include "compiler/data/lambda-class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/gentree.h"
#include "compiler/make/make.h"
#include "compiler/threading/hash-table.h"

static FunctionPtr UNPARSED_BUT_REQUIRED_FUNC_PTR = FunctionPtr(reinterpret_cast<FunctionData *>(0x0001));

CompilerCore::CompilerCore() :
  env_(nullptr) {
}

void CompilerCore::start() {
  get_profiler("total").start();
  stage::die_if_global_errors();
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
  env_ = nullptr;

  get_profiler("total").finish();
}

void CompilerCore::register_env(KphpEnviroment *env) {
  kphp_assert (env_ == nullptr);
  env_ = env;
}

const KphpEnviroment &CompilerCore::env() const {
  kphp_assert (env_ != nullptr);
  return *env_;
}

const string &CompilerCore::get_global_namespace() const {
  return env().get_static_lib_name();
}

FunctionPtr CompilerCore::get_function(const string &name) {
  TSHashTable<FunctionPtr>::HTNode *node = functions_ht.at(vk::std_hash(name));
  AutoLocker<Lockable *> locker(node);
  if (!node->data || node->data == UNPARSED_BUT_REQUIRED_FUNC_PTR) {
    return FunctionPtr();
  }

  FunctionPtr f = node->data;
  kphp_assert_msg(f->name == name, format("Bug in compiler: hash collision: `%s' and `%s`", f->name.c_str(), name.c_str()));
  return f;
}

string CompilerCore::unify_file_name(const string &file_name) {
  if (env().get_base_dir().empty()) { //hack: directory of first file will be used ad base_dir
    size_t i = file_name.find_last_of("/");
    kphp_assert (i != string::npos);
    env_->set_base_dir(file_name.substr(0, i + 1));
  }
  const string &base_dir = env().get_base_dir();
  if (strncmp(file_name.c_str(), base_dir.c_str(), base_dir.size())) {
    return file_name;
  }
  return file_name.substr(base_dir.size());
}

std::string CompilerCore::search_file_in_include_dirs(const std::string &file_name, size_t *dir_index) const {
  if (file_name.empty() || file_name[0] == '/' || file_name[0] == '.') {
    return {};
  }
  std::string full_file_name;
  size_t index = 0;
  const auto &includes = env().get_includes();
  for (; index < includes.size() && full_file_name.empty(); ++index) {
    full_file_name = get_full_path(includes[index] + file_name);
  }
  if (!full_file_name.empty() && dir_index) {
    *dir_index = index;
  }
  return full_file_name;
}

SrcFilePtr CompilerCore::register_file(const string &file_name, LibPtr owner_lib) {
  if (file_name.empty()) {
    return SrcFilePtr();
  }

  //search file
  string full_file_name = search_file_in_include_dirs(file_name);
  if (file_name[0] == '/') {
    full_file_name = get_full_path(file_name);
  } else if (full_file_name.empty()) {
    vector<string> cur_include_dirs;
    SrcFilePtr from_file = stage::get_file();
    if (from_file) {
      string from_file_name = from_file->file_name;
      size_t en = from_file_name.find_last_of('/');
      assert (en != string::npos);
      string cur_dir = from_file_name.substr(0, en + 1);
      cur_include_dirs.push_back(cur_dir);
      if (from_file->owner_lib) {
        cur_include_dirs.push_back(from_file->owner_lib->lib_dir());
      }
    }
    if (!from_file || file_name[0] != '.') {
      cur_include_dirs.push_back("");
    }
    int n = (int)cur_include_dirs.size();
    for (int i = 0; i < n && full_file_name.empty(); i++) {
      full_file_name = get_full_path(cur_include_dirs[i] + file_name);
    }
  }

  if (full_file_name.empty()) {
    return {};
  }

  size_t last_pos_of_slash = full_file_name.find_last_of('/');
  last_pos_of_slash = last_pos_of_slash != std::string::npos ? last_pos_of_slash + 1 : 0;

  size_t last_pos_of_dot = full_file_name.find_last_of('.');
  if (last_pos_of_dot == std::string::npos || last_pos_of_dot < last_pos_of_slash) {
    last_pos_of_dot = full_file_name.length();
  }

  string short_file_name = full_file_name.substr(last_pos_of_slash, last_pos_of_dot - last_pos_of_slash);
  string extension = full_file_name.substr(std::min(full_file_name.length(), last_pos_of_dot + 1));
  if (extension != "php") {
    short_file_name += "_";
    short_file_name += extension;
  }

  //register file if needed
  TSHashTable<SrcFilePtr>::HTNode *node = file_ht.at(vk::std_hash(full_file_name));
  if (!node->data) {
    AutoLocker<Lockable *> locker(node);
    if (!node->data) {
      SrcFilePtr new_file = SrcFilePtr(new SrcFile(full_file_name, short_file_name, owner_lib));
      char tmp[50];
      sprintf(tmp, "%zx", vk::std_hash(full_file_name));
      string func_name = gen_unique_name("src_" + new_file->short_file_name + tmp, true);
      new_file->main_func_name = func_name;
      new_file->unified_file_name = unify_file_name(new_file->file_name);
      size_t last_slash = new_file->unified_file_name.rfind('/');
      new_file->unified_dir_name = last_slash == string::npos ? "" : new_file->unified_file_name.substr(0, last_slash);
      node->data = new_file;
    }
  }
  SrcFilePtr file = node->data;
  return file;
}

void CompilerCore::require_function(const string &name, DataStream<FunctionPtr> &os) {
  operate_on_function_locking(name, [&](FunctionPtr &f) {
    if (!f) {
      f = UNPARSED_BUT_REQUIRED_FUNC_PTR;
    } else if (f != UNPARSED_BUT_REQUIRED_FUNC_PTR) {
      require_function(f, os);
    }
  });
}

void CompilerCore::require_function(FunctionPtr function, DataStream<FunctionPtr> &os) {
  if (!function->is_required) {
    function->is_required = true;
    os << function;
  }
}

void CompilerCore::register_function(FunctionPtr function) {
  static DataStream<FunctionPtr> unused;
  register_and_require_function(function, unused, false);
  kphp_assert(!function->is_required);
}

void CompilerCore::register_and_require_function(FunctionPtr function, DataStream<FunctionPtr> &os, bool force_require /*= false*/) {
  operate_on_function_locking(function->name, [&](FunctionPtr &f) {
    bool was_previously_required = f == UNPARSED_BUT_REQUIRED_FUNC_PTR;
    kphp_error(!f || was_previously_required,
               format("Redeclaration of function %s(), the previous declaration was in [%s]",
                       function->get_human_readable_name().c_str(), f->file_id->file_name.c_str()));
    f = function;

    if (was_previously_required || force_require) {
      require_function(f, os);
    }
  });
}

void CompilerCore::register_class(ClassPtr cur_class) {
  TSHashTable<ClassPtr>::HTNode *node = classes_ht.at(vk::std_hash(cur_class->name));
  AutoLocker<Lockable *> locker(node);
  kphp_error (!node->data,
              format("Redeclaration of class [%s], the previous declaration was in [%s]",
                     cur_class->name.c_str(), node->data->file_id->file_name.c_str()));
  node->data = cur_class;
}

LibPtr CompilerCore::register_lib(LibPtr lib) {
  TSHashTable<LibPtr>::HTNode *node = libs_ht.at(vk::std_hash(lib->lib_namespace()));
  AutoLocker<Lockable *> locker(node);
  if (!node->data) {
    node->data = lib;
  }
  return node->data;
}

void CompilerCore::register_main_file(const string &file_name, DataStream<SrcFilePtr> &os) {
  SrcFilePtr res = register_file(file_name, LibPtr{});
  kphp_error (file_name.empty() || res, format("Cannot load main file [%s]", file_name.c_str()));

  if (res && try_require_file(res)) {
    main_files.push_back(res);
    os << res;
  }
}

SrcFilePtr CompilerCore::require_file(const string &file_name, LibPtr owner_lib, DataStream<SrcFilePtr> &os, bool error_if_not_exists /* = true */) {
  SrcFilePtr file = register_file(file_name, owner_lib);
  kphp_error (file || !error_if_not_exists, format("Cannot load file [%s]", file_name.c_str()));
  if (file && try_require_file(file)) {
    os << file;
  }
  return file;
}


ClassPtr CompilerCore::get_class(vk::string_view name) {
  return classes_ht.at(vk::std_hash(name))->data;
}

ClassPtr CompilerCore::get_memcache_class() {
  if (!memcache_class) {            // если нет специального
    return get_class("Memcache");   // то берём из functions.txt
  }
  return memcache_class;
}

void CompilerCore::set_memcache_class(ClassPtr klass) {
  kphp_error(!memcache_class || memcache_class == klass,
             format("Duplicate Memcache realization %s and %s", memcache_class->name.c_str(), klass->name.c_str()));
  memcache_class = klass;
}

bool CompilerCore::register_define(DefinePtr def_id) {
  TSHashTable<DefinePtr>::HTNode *node = defines_ht.at(vk::std_hash(def_id->name));
  AutoLocker<Lockable *> locker(node);

  kphp_error_act (
    !node->data,
    format("Redeclaration of define [%s], the previous declaration was in [%s]",
            def_id->name.c_str(), node->data->file_id->file_name.c_str()),
    return false
  );

  node->data = def_id;
  return true;
}

DefinePtr CompilerCore::get_define(const string &name) {
  return defines_ht.at(vk::std_hash(name))->data;
}

VarPtr CompilerCore::create_var(const string &name, VarData::Type type) {
  VarPtr var = VarPtr(new VarData(type));
  var->name = name;
  stats.on_var_inserting(type);
  return var;
}

VarPtr CompilerCore::get_global_var(const string &name, VarData::Type type,
                                    VertexPtr init_val) {
  TSHashTable<VarPtr>::HTNode *node = global_vars_ht.at(vk::std_hash(name));
  VarPtr new_var;
  if (!node->data) {
    AutoLocker<Lockable *> locker(node);
    if (!node->data) {
      new_var = create_var(name, type);
      new_var->init_val = init_val;
      node->data = new_var;
    }
  }
  VarPtr var = node->data;
  if (!new_var) {
    kphp_assert_msg(var->name == name, "bug in compiler (hash collision)");
    if (init_val) {
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
          string new_array_repr = VertexPtrFormatter::to_string(init_val);
          string hashed_array_repr = VertexPtrFormatter::to_string(var->init_val);

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

VarPtr CompilerCore::create_local_var(FunctionPtr function, const string &name, VarData::Type type) {
  VarPtr var = create_var(name, type);
  var->holder_func = function;
  switch (type) {
    case VarData::var_local_t:
    case VarData::var_local_inplace_t:
      function->local_var_ids.push_back(var);
      break;
    case VarData::var_static_t:
      function->static_var_ids.push_back(var);
      break;
    case VarData::var_param_t:
      var->param_i = (int)function->param_ids.size();
      function->param_ids.push_back(var);
      break;
    default:
      kphp_fail();
  }
  return var;
}

const vector<SrcFilePtr> &CompilerCore::get_main_files() {
  return main_files;
}

vector<VarPtr> CompilerCore::get_global_vars() {
  return global_vars_ht.get_all();
}

vector<ClassPtr> CompilerCore::get_classes() {
  return classes_ht.get_all();
}

vector<DefinePtr> CompilerCore::get_defines() {
  return defines_ht.get_all();
}

vector<LibPtr> CompilerCore::get_libs() {
  return libs_ht.get_all();
}

void CompilerCore::load_index() {
  string index_path = env().get_index();
  if (index_path.empty()) {
    return;
  }
  FILE *f = fopen(index_path.c_str(), "r");
  if (f == nullptr) {
    return;
  }
  cpp_index.load(f);
  fclose(f);
}

void CompilerCore::save_index() {
  string index_path = env().get_index();
  if (index_path.empty()) {
    return;
  }
  string tmp_index_path = index_path + ".tmp";
  FILE *f = fopen(tmp_index_path.c_str(), "w");
  if (f == nullptr) {
    return;
  }
  cpp_index.save(f);
  fclose(f);
  int err = system(("mv " + tmp_index_path + " " + index_path).c_str());
  kphp_error (err == 0, "Failed to rewrite index");
}

const Index& CompilerCore::get_index() {
  return cpp_index;
}

File *CompilerCore::get_file_info(const string &file_name) {
  return cpp_index.insert_file(file_name);
}

void CompilerCore::del_extra_files() {
  cpp_index.del_extra_files();
}

void CompilerCore::init_dest_dir() {
  if (env().get_use_auto_dest()) {
    env_->set_dest_dir_subdir(get_subdir_name());
  }
  env_->init_dest_dirs();
  cpp_dir = env().get_dest_cpp_dir();
  cpp_index.sync_with_dir(cpp_dir);
  cpp_dir = cpp_index.get_dir();
}

std::string CompilerCore::get_subdir_name() const {
  assert (env().get_use_auto_dest());

  const string &name = main_files[0]->short_file_name;
  string hash_string;
  for (auto main_file : main_files) {
    hash_string += main_file->file_name;
    hash_string += ";";
  }
  stringstream ss;
  ss << "auto." << name << "-" << std::hex << vk::std_hash(hash_string);

  return ss.str();
}


bool CompilerCore::try_require_file(SrcFilePtr file) {
  return __sync_bool_compare_and_swap(&file->is_required, false, true);
}

CompilerCore *G;

bool try_optimize_var(VarPtr var) {
  return __sync_bool_compare_and_swap(&var->optimize_flag, false, true);
}

/**
 * If argument is array of twos -> concatenate elements with `$$`
 * If argument is string -> replace `:` with '$'
 * replaces '\' with `$` in result
 *
 * This function doesn't resolve uses, `parent`, `self` etc
 * In php, when you pass callback as string or array you should
 * write full qualified namespace and function name, only in rare cases
 * you can specify `parent` but in another cases you can't e.g.:
 *
 * array_map(['\Namespace\Name::ClassName', 'parent::fun_name'], [1, 2, 3]); - it's ok in php
 *
 * function fun(callable $callback, $x) { $callback($x); }
 * fun(['\Namespace\Name::ClassName', 'parent::fun_name'], 1); - it's `PHP Fatal error`
 *
 * If you need `parent`, please specify manually full parent name.
 */
string conv_to_func_ptr_name(VertexPtr call) {
  VertexPtr name_v = GenTree::get_actual_value(call);
  std::string res;

  if (name_v->type() == op_string) {
    res = replace_characters(name_v->get_string(), ':', '$');
  } else if (name_v->type() == op_array && name_v->size() == 2) {
    VertexPtr class_name = GenTree::get_actual_value(name_v.as<op_array>()->args()[0]);
    VertexPtr fun_name = GenTree::get_actual_value(name_v.as<op_array>()->args()[1]);

    if (class_name->type() == op_string && fun_name->type() == op_string) {
      res = class_name->get_string() + "$$" + fun_name->get_string();
    }
  }

  if (!res.empty() && res[0] == '\\') {
    res.erase(res.begin());
  }

  return replace_backslashes(res);
}

VertexPtr conv_to_func_ptr(VertexPtr call) {
  if (call->type() != op_func_ptr) {
    string name = conv_to_func_ptr_name(call);
    if (!name.empty()) {
      auto new_call = VertexAdaptor<op_func_ptr>::create();
      new_call->str_val = name;
      set_location(new_call, call->get_location());
      call = new_call;
    }
  }

  return call;
}


