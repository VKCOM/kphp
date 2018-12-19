#include "compiler/compiler-core.h"

#include <fstream>
#include <numeric>

#include "compiler/const-manipulations.h"
#include "compiler/data/class-data.h"
#include "compiler/data/define-data.h"
#include "compiler/data/src-file.h"
#include "compiler/gentree.h"
#include "compiler/make.h"
#include "compiler/threading/hash-table.h"

FunctionPtr UNPARSED_BUT_REQUIRED_FUNC_PTR = FunctionPtr(reinterpret_cast<FunctionData *>(0x0001));

CompilerCore::CompilerCore() :
  env_(nullptr) {
}

void CompilerCore::start() {
  PROF (total).start();
  stage::die_if_global_errors();
  create_builtin_classes();
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

  PROF (total).finish();
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
  TSHashTable<FunctionPtr>::HTNode *node = functions_ht.at(hash_ll(name));
  AutoLocker<Lockable *> locker(node);
  if (!node->data || node->data == UNPARSED_BUT_REQUIRED_FUNC_PTR) {
    return FunctionPtr();
  }

  FunctionPtr f = node->data;
  kphp_assert_msg(f->name == name, format("Bug in compiler: hash collision: `%s' and `%s`", f->name.c_str(), name.c_str()));
  return f;
}

VertexPtr CompilerCore::get_extern_func_header(const string &name) {
  TSHashTable<VertexPtr>::HTNode *node = extern_func_headers_ht.at(hash_ll(name));
  return node->data;
}

void CompilerCore::save_extern_func_header(const string &name, VertexPtr header) {
  TSHashTable<VertexPtr>::HTNode *node = extern_func_headers_ht.at(hash_ll(name));
  AutoLocker<Lockable *> locker(node);
  kphp_error_return (
    !node->data,
    format("Several headers for one function [%s] are found", name.c_str())
  );
  node->data = header;
}

void CompilerCore::create_builtin_classes() {
  ClassPtr exception = ClassPtr(new ClassData());
  exception->name = "Exception";
  exception->init_function = FunctionPtr(new FunctionData());
  classes_ht.at(hash_ll(exception->name))->data = exception;

  ClassPtr memcache = ClassPtr(new ClassData());
  memcache->name = "Memcache";
  memcache->init_function = FunctionPtr(new FunctionData());
  classes_ht.at(hash_ll(memcache->name))->data = memcache;
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

SrcFilePtr CompilerCore::register_file(const string &file_name, const string &context, LibPtr owner_lib) {
  if (file_name.empty()) {
    return SrcFilePtr();
  }

  //search file
  string full_file_name;
  if (file_name[0] != '/' && file_name[0] != '.') {
    int n = (int)env().get_includes().size();
    for (int i = 0; i < n && full_file_name.empty(); i++) {
      full_file_name = get_full_path(env().get_includes()[i] + file_name);
    }
  }
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
  TSHashTable<SrcFilePtr>::HTNode *node = file_ht.at(hash_ll(full_file_name + context));
  if (!node->data) {
    AutoLocker<Lockable *> locker(node);
    if (!node->data) {
      SrcFilePtr new_file = SrcFilePtr(new SrcFile(full_file_name, short_file_name, context, owner_lib));
      char tmp[50];
      sprintf(tmp, "%x", hash(full_file_name + context));
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
    } else if (f != UNPARSED_BUT_REQUIRED_FUNC_PTR && !f->is_required) {
      f->is_required = true;      // этот флаг прежде всего нужен, чтоб в output отправить только раз
      os << f;
    }
  });
}

FunctionPtr CompilerCore::register_function(const FunctionInfo &info, DataStream<FunctionPtr> &os) {
  const VertexPtr &root = info.root;
  if (!root) {
    return FunctionPtr();
  }
  FunctionPtr function;
  if (root->type() == op_function || root->type() == op_func_decl) {
    function = root->get_func_id() ?: FunctionData::create_function(info);
  } else if (root->type() == op_extern_func) {
    string function_name = root.as<meta_op_function>()->name()->get_string();
    save_extern_func_header(function_name, root);
    return FunctionPtr();
  } else {
    kphp_fail();
  }

  bool auto_require = info.kphp_required
                      || function->type() == FunctionData::func_global
                      || function->type() == FunctionData::func_extern
                      || (function->is_instance_function() && !function->is_lambda());

  operate_on_function_locking(function->name, [&](FunctionPtr &f) {
    if (f == UNPARSED_BUT_REQUIRED_FUNC_PTR) {
      auto_require = true;
    }
    kphp_error(!f || f == UNPARSED_BUT_REQUIRED_FUNC_PTR,
               format("Redeclaration of function %s(), the previous declaration was in [%s]",
                       function->get_human_readable_name().c_str(), f->file_id->file_name.c_str()));
    f = function;
  });

  if (auto_require) {
    function->is_required = true;
    os << function;
  }

  return function;
}

ClassPtr CompilerCore::register_class(ClassPtr cur_class) {
  vk::string_view sv{cur_class->name};
  if (!sv.starts_with(FunctionData::get_lambda_namespace())) {
    string init_function_name_str = stage::get_file()->main_func_name;
    cur_class->init_function = get_function(init_function_name_str);
    cur_class->init_function->class_id = cur_class;
  }

  TSHashTable<ClassPtr>::HTNode *node = classes_ht.at(hash_ll(cur_class->name));
  AutoLocker<Lockable *> locker(node);
  kphp_error_act (
    !node->data,
    format("Redeclaration of class [%s], the previous declaration was in [%s]",
            cur_class->name.c_str(), node->data->file_id->file_name.c_str()),
    return ClassPtr()
  );
  node->data = cur_class;
  return cur_class;
}

LibPtr CompilerCore::register_lib(LibPtr lib) {
  TSHashTable<LibPtr>::HTNode *node = libs_ht.at(hash_ll(lib->lib_namespace()));
  AutoLocker<Lockable *> locker(node);
  if (!node->data) {
    node->data = lib;
  }
  return node->data;
}

void CompilerCore::register_main_file(const string &file_name, DataStream<SrcFilePtr> &os) {
  SrcFilePtr res = register_file(file_name, "", LibPtr{});
  kphp_error (file_name.empty() || res, format("Cannot load main file [%s]", file_name.c_str()));

  if (res && try_require_file(res)) {
    if (!env().get_functions().empty()) {
      string prefix = "<?php require_once (\"" + env().get_functions() + "\");?>";
      res->add_prefix(prefix);
    }
    main_files.push_back(res);
    os << res;
  }
}

pair<SrcFilePtr, bool> CompilerCore::require_file(const string &file_name, const string &context,
                                                  LibPtr owner_lib, DataStream<SrcFilePtr> &os) {
  SrcFilePtr file = register_file(file_name, context, owner_lib);
  kphp_error (file_name.empty() || file, format("Cannot load file [%s]", file_name.c_str()));
  bool required = false;
  if (file && try_require_file(file)) {
    required = true;
    os << file;
  }
  return {file, required};
}


ClassPtr CompilerCore::get_class(const string &name) {
  return classes_ht.at(hash_ll(name))->data;
}

bool CompilerCore::register_define(DefinePtr def_id) {
  TSHashTable<DefinePtr>::HTNode *node = defines_ht.at(hash_ll(def_id->name));
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
  return defines_ht.at(hash_ll(name))->data;
}

VarPtr CompilerCore::create_var(const string &name, VarData::Type type) {
  VarPtr var = VarPtr(new VarData(type));
  var->name = name;
  return var;
}

VarPtr CompilerCore::get_global_var(const string &name, VarData::Type type,
                                    VertexPtr init_val) {
  TSHashTable<VarPtr>::HTNode *node = global_vars_ht.at(hash_ll(name));
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

File *CompilerCore::get_file_info(const string &file_name) {
  return cpp_index.get_file(file_name, true);
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
  ss << "auto." << name << "-" << std::hex << hash(hash_string);

  return ss.str();
}

bool compare_mtime(File *f, File *g) {
  if (f->mtime != g->mtime) {
    return f->mtime > g->mtime;
  }
  return f->path < g->path;
}

std::map<File *, long long> create_dep_mtime(Index *cpp_dir) {
  std::map<File *, long long> dep_mtime;
  std::priority_queue<pair<long long, File *>> mtime_queue;
  std::map<File *, vector<File *>> reverse_includes;

  std::vector<File *> files = cpp_dir->get_files();

  auto lib_version_it = std::find_if(files.begin(), files.end(), [](File *file) { return file->name == "_lib_version.h"; });
  kphp_assert(lib_version_it != files.end());
  File *lib_version = *lib_version_it;

  for (const auto &file : files) {
    for (const auto &include : file->includes) {
      File *header = cpp_dir->get_file(include, false);
      kphp_assert (header != nullptr);
      kphp_assert (header->on_disk);
      reverse_includes[header].push_back(file);
    }
    dep_mtime[file] = std::max(file->mtime, lib_version->mtime);
    mtime_queue.emplace(dep_mtime[file], file);
  }

  while (!mtime_queue.empty()) {
    long long mtime;
    File *file;
    std::tie(mtime, file) = mtime_queue.top();
    mtime_queue.pop();

    if (dep_mtime[file] != mtime || !reverse_includes.count(file)) {
      continue;
    }

    for (auto including_file: reverse_includes[file]) {
      auto &including_file_mtime = dep_mtime[including_file];
      if (including_file_mtime < mtime) {
        including_file_mtime = mtime;
        mtime_queue.emplace(including_file_mtime, including_file);
      }
    }
  }
  return dep_mtime;
}

std::vector<File *> create_obj_files(KphpMake *make, Index *obj_dir, Index *cpp_dir) {
  std::vector<File *> files = cpp_dir->get_files();
  std::map<File *, long long> dep_mtime = create_dep_mtime(cpp_dir);

  std::vector<File *> objs;
  for (auto cpp_file : files) {
    if (cpp_file->ext == ".cpp") {
      File *obj_file = obj_dir->get_file(cpp_file->name_without_ext + ".o", true);
      obj_file->compile_with_debug_info_flag = cpp_file->compile_with_debug_info_flag;
      make->create_cpp2obj_target(cpp_file, obj_file);
      Target *cpp_target = cpp_file->target;
      cpp_target->force_changed(dep_mtime[cpp_file]);
      objs.push_back(obj_file);
    }
  }
  fprintf(stderr, "objs cnt = %zu\n", objs.size());

  std::map<string, vector<File *>> subdirs;
  std::vector<File *> tmp_objs;
  for (auto obj_file : objs) {
    std::string name = obj_file->subdir;
    if (!name.empty()) {
      name.pop_back();
    }
    if (name.empty()) {
      tmp_objs.push_back(obj_file);
      continue;
    }
    name += ".o";
    subdirs[name].push_back(obj_file);
  }

  objs = std::move(tmp_objs);
  for (const auto &name_and_files : subdirs) {
    File *obj_file = obj_dir->get_file(name_and_files.first, true);
    make->create_objs2obj_target(name_and_files.second, obj_file);
    objs.push_back(obj_file);
  }
  fprintf(stderr, "objs cnt = %zu\n", objs.size());
  return objs;
}

bool kphp_make(File *bin, Index *obj_dir, Index *cpp_dir, std::forward_list<File> external_libs, const KphpEnviroment &kphp_env) {
  KphpMake make;
  std::vector<File *> lib_objs;
  for (File &link_file: external_libs) {
    if (link_file.mtime == 0) {
      fprintf(stdout, "Can't read mtime of link_file [%s]\n", link_file.path.c_str());
      return false;
    }
    make.create_cpp_target(&link_file);
    lib_objs.emplace_back(&link_file);
  }
  std::vector<File *> objs = create_obj_files(&make, obj_dir, cpp_dir);
  std::copy(lib_objs.begin(), lib_objs.end(), std::back_inserter(objs));
  make.create_objs2bin_target(objs, bin);
  make.init_env(kphp_env);
  return make.make_target(bin, kphp_env.get_jobs_count());
}

bool kphp_make_static_lib(File *static_lib, Index *obj_dir, Index *cpp_dir, const KphpEnviroment &kphp_env) {
  KphpMake make;
  std::vector<File *> objs = create_obj_files(&make, obj_dir, cpp_dir);
  make.create_objs2static_lib_target(objs, static_lib);
  make.init_env(kphp_env);
  return make.make_target(static_lib, kphp_env.get_jobs_count());
}

void copy_file(const File &source_file, const string &destination, bool show_cmd) {
  File destination_file(destination);
  kphp_assert (destination_file.upd_mtime() >= 0);
  const string cmd = "ln --force " + source_file.path + " " + destination_file.path + " 2> /dev/null"
                     + "||  cp " + source_file.path + " " + destination_file.path;
  const int err = system(cmd.c_str());
  if (show_cmd) {
    fprintf(stderr, "[%s]: %d %d\n", cmd.c_str(), err, WEXITSTATUS (err));
  }
  if (err == -1 || !WIFEXITED (err) || WEXITSTATUS (err)) {
    if (err == -1) {
      perror("system failed");
    }
    kphp_error (0, format("Failed [%s]", cmd.c_str()));
    stage::die_if_global_errors();
  }
}

void CompilerCore::copy_static_lib_to_out_dir(const File &static_archive, bool show_copy_cmd) const {
  Index out_dir;
  out_dir.set_dir(env().get_static_lib_out_dir());
  out_dir.del_extra_files();

  // copy static archive
  LibData out_lib(env().get_static_lib_name(), out_dir.get_dir());
  copy_file(static_archive, out_lib.static_archive_path(), show_copy_cmd);

  // copy functions.txt of this static archive
  File functions_txt_tmp(env().get_dest_cpp_dir() + LibData::functions_txt_tmp_file());
  copy_file(functions_txt_tmp, out_lib.functions_txt_file(), show_copy_cmd);

  // copy runtime lib sha256 of this static archive
  File runtime_lib_sha256(env().get_runtime_sha256_file());
  copy_file(runtime_lib_sha256, out_lib.runtime_lib_sha256_file(), show_copy_cmd);

  Index headers_tmp_dir;
  headers_tmp_dir.sync_with_dir(env().get_dest_cpp_dir() + LibData::headers_tmp_dir());
  Index out_headers_dir;
  out_headers_dir.set_dir(out_lib.headers_dir());
  // copy cpp header files of this static archive
  for (File *header_file: headers_tmp_dir.get_files()) {
    copy_file(*header_file, out_headers_dir.get_dir() + header_file->name, show_copy_cmd);
  }
}

std::forward_list<File> CompilerCore::collect_external_libs() {
  const string &binary_runtime_sha256 = env().get_runtime_sha256();
  stage::die_if_global_errors();

  std::forward_list<File> external_libs;
  external_libs.emplace_front(env().get_link_file());
  for (const auto &lib: get_libs()) {
    if (lib && !lib->is_raw_php()) {
      std::string lib_runtime_sha256 = KphpEnviroment::read_runtime_sha256_file(lib->runtime_lib_sha256_file());

      kphp_error_act(binary_runtime_sha256 == lib_runtime_sha256,
                     format("Mismatching between sha256 of binary runtime '%s' and lib runtime '%s'",
                            env().get_runtime_sha256_file().c_str(), lib->runtime_lib_sha256_file().c_str()),
                     continue);
      external_libs.emplace_front(lib->static_archive_path());
    }
  }

  for (File &file: external_libs) {
    kphp_assert (file.upd_mtime() >= 0);
    if (env().get_verbosity() >= 1) {
      fprintf(stderr, "Use static lib [%s]\n", file.path.c_str());
    }
  }

  stage::die_if_global_errors();
  return external_libs;
}

void CompilerCore::make() {
  AUTO_PROF (make);
  stage::set_name("Make");
  cpp_index.del_extra_files();

  Index obj_index;
  obj_index.sync_with_dir(env().get_dest_objs_dir());

  File bin_file(env().get_binary_path());
  kphp_assert (bin_file.upd_mtime() >= 0);

  if (env().get_make_force()) {
    obj_index.del_extra_files();
    bin_file.unlink();
  }

  const bool ok =
    env().is_static_lib_mode()
    ? kphp_make_static_lib(&bin_file, &obj_index, &cpp_index, env())
    : kphp_make(&bin_file, &obj_index, &cpp_index, collect_external_libs(), env());

  kphp_error (ok, "Make failed");
  stage::die_if_global_errors();
  obj_index.del_extra_files();

  const bool show_copy_cmd = env().get_verbosity() >= 3;
  if (!env().get_user_binary_path().empty()) {
    copy_file(bin_file, env().get_user_binary_path(), show_copy_cmd);
  }
  if (env().is_static_lib_mode()) {
    copy_static_lib_to_out_dir(bin_file, show_copy_cmd);
  }
}
bool CompilerCore::try_require_file(SrcFilePtr file) {
  return __sync_bool_compare_and_swap(&file->is_required, false, true);
}

CompilerCore *G;

bool try_optimize_var(VarPtr var) {
  return __sync_bool_compare_and_swap(&var->optimize_flag, false, true);
}

string conv_to_func_ptr_name(VertexPtr call) {
  VertexPtr name_v = GenTree::get_actual_value(call);

  switch (name_v->type()) {
    case op_string:
    case op_func_name:
      return name_v->get_string();

    case op_array: {
      if (name_v->size() == 2) {
        VertexPtr class_name = GenTree::get_actual_value(name_v.as<op_array>()->args()[0]);
        VertexPtr fun_name = GenTree::get_actual_value(name_v.as<op_array>()->args()[1]);

        if (class_name->type() == op_string && fun_name->type() == op_string) {
          return class_name->get_string() + "::" + fun_name->get_string();
        }
      }
      break;
    }

    default:
      break;
  }

  return "";
}

VertexPtr conv_to_func_ptr(VertexPtr call, FunctionPtr current_function) {
  if (call->type() != op_func_ptr) {
    string name = conv_to_func_ptr_name(call);
    if (!name.empty()) {
      name = get_full_static_member_name(current_function, name, true);
      auto new_call = VertexAdaptor<op_func_ptr>::create();
      new_call->str_val = name;
      set_location(new_call, call->get_location());
      call = new_call;
    }
  }

  return call;
}


