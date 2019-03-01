#include "compiler/compiler-core.h"

#include <fstream>
#include <numeric>
#include <sys/sendfile.h>
#include <unordered_map>

#include "compiler/const-manipulations.h"
#include "compiler/data/class-data.h"
#include "compiler/data/define-data.h"
#include "compiler/data/lambda-class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/gentree.h"
#include "compiler/make.h"
#include "compiler/threading/hash-table.h"
#include "common/wrappers/mkdir_recursive.h"

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

VertexPtr CompilerCore::get_extern_func_header(const string &name) {
  TSHashTable<VertexPtr>::HTNode *node = extern_func_headers_ht.at(vk::std_hash(name));
  return node->data;
}

void CompilerCore::save_extern_func_header(const string &name, VertexPtr header) {
  TSHashTable<VertexPtr>::HTNode *node = extern_func_headers_ht.at(vk::std_hash(name));
  AutoLocker<Lockable *> locker(node);
  kphp_error_return (
    !node->data,
    format("Several headers for one function [%s] are found", name.c_str())
  );
  node->data = header;
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

long long get_imported_header_mtime(const std::string &header_path, const std::forward_list<Index> &imported_headers) {
  for (const Index &lib_headers_dir: imported_headers) {
    if (File *header = lib_headers_dir.get_file(header_path)) {
      return header->mtime;
    }
  }
  kphp_error(false, format("Can't file lib header file '%s'", header_path.c_str()));
  return 0;
}

void hard_link_or_copy_impl(const std::string &from, const std::string &to, bool replace, bool allow_copy) noexcept {
  if (!link(from.c_str(), to.c_str())) {
    return;
  }

  if (errno == EEXIST) {
    if (replace) {
      kphp_error_return(!unlink(to.c_str()),
                        format("Can't remove file '%s': %s", to.c_str(), strerror(errno)));
      hard_link_or_copy_impl(from, to, false, allow_copy);
    }
    return;
  }

  // the file is placed on other device, or we have a permission problems, try to copy it
  if (vk::any_of_equal(errno, EXDEV, EPERM) && allow_copy) {
    struct stat file_stat;
    kphp_error_return (!stat(from.c_str(), &file_stat),
                       format("Can't get file stat '%s': %s", from.c_str(), strerror(errno)));

    std::string tmp_file = to + ".XXXXXX";
    int tmp_fd = mkstemp(&tmp_file[0]);
    kphp_error_return(tmp_fd != -1,
                      format("Can't create tmp file '%s': %s", tmp_file.c_str(), strerror(errno)));
    kphp_error_return(fchmod(tmp_fd, file_stat.st_mode) != -1,
                      format("Can't change permissions of tmp file '%s': %s", tmp_file.c_str(), strerror(errno)));
    int from_fd = open(from.c_str(), O_RDONLY);
    const ssize_t s = sendfile(tmp_fd, from_fd, nullptr, file_stat.st_size);
    close(from_fd);
    close(tmp_fd);
    kphp_error_return(s != -1, format("Can't copy file from '%s' to '%s': %s", from.c_str(), tmp_file.c_str(), strerror(errno)));
    kphp_assert(s == file_stat.st_size);
    hard_link_or_copy_impl(tmp_file, to, replace, false);
    unlink(tmp_file.c_str());
    return;
  }

  kphp_error(0, format("Can't copy file from '%s' to '%s': %s", from.c_str(), to.c_str(), strerror(errno)));
}

void hard_link_or_copy(const std::string &from, const std::string &to, bool replace = true) {
  hard_link_or_copy_impl(from, to, replace, true);
  stage::die_if_global_errors();
}

std::string kphp_make_precompiled_header(Index *obj_dir, const KphpEnviroment &kphp_env) {
  std::string gch_dir = "/tmp/kphp_gch/";
  gch_dir.append(kphp_env.get_runtime_sha256()).append(1, '/');
  gch_dir.append(kphp_env.get_cxx_flags_sha256()).append(1, '/');

  const std::string header_filename = "php_functions.h";
  const std::string gch_filename = header_filename + ".gch";
  const std::string gch_path = gch_dir + gch_filename;
  if (access(gch_path.c_str(), F_OK) != -1) {
    return gch_dir;
  }

  KphpMake make;
  File php_functions_h(kphp_env.get_path() + "PHP/" + header_filename);
  kphp_error_act(php_functions_h.upd_mtime() > 0,
                 format("Can't read mtime of '%s'", php_functions_h.path.c_str()),
                 return {});

  File *php_functions_h_gch = obj_dir->insert_file(kphp_env.get_dest_objs_dir() + gch_filename);
  make.create_cpp2obj_target(&php_functions_h, php_functions_h_gch);
  File sha256_version_file(kphp_env.get_runtime_sha256_file());
  kphp_assert(sha256_version_file.upd_mtime() > 0);
  php_functions_h.target->force_changed(sha256_version_file.mtime);
  make.init_env(kphp_env);
  if (!make.make_target(php_functions_h_gch, 1)) {
    return {};
  }

  const mode_t old_mask = umask(0);
  const bool dir_created = mkdir_recursive(gch_dir.c_str(), 0777);
  umask(old_mask);
  kphp_error_act(dir_created,
                 format("Can't create tmp dir '%s': %s", gch_dir.c_str(), strerror(errno)),
                 return {});

  hard_link_or_copy(php_functions_h.path, gch_dir + header_filename, false);
  hard_link_or_copy(php_functions_h_gch->path, gch_path, false);
  php_functions_h_gch->unlink();
  return gch_dir;
}

std::unordered_map<File *, long long> create_dep_mtime(const Index &cpp_dir, const std::forward_list<Index> &imported_headers) {
  std::unordered_map<File *, long long> dep_mtime;
  std::priority_queue<std::pair<long long, File *>> mtime_queue;
  std::unordered_map<File *, std::vector<File *>> reverse_includes;

  std::vector<File *> files = cpp_dir.get_files();

  auto lib_version_it = std::find_if(files.begin(), files.end(), [](File *file) { return file->name == "_lib_version.h"; });
  kphp_assert(lib_version_it != files.end());
  File *lib_version = *lib_version_it;

  for (const auto &file : files) {
    for (const auto &include : file->includes) {
      File *header = cpp_dir.get_file(include);
      kphp_assert (header != nullptr);
      kphp_assert (header->on_disk);
      reverse_includes[header].push_back(file);
    }

    long long max_mtime = std::max(file->mtime, lib_version->mtime);
    for (const auto &lib_include : file->lib_includes) {
      max_mtime = std::max(max_mtime, get_imported_header_mtime(lib_include, imported_headers));
    }

    dep_mtime[file] = max_mtime;
    mtime_queue.emplace(max_mtime, file);
  }

  while (!mtime_queue.empty()) {
    long long mtime;
    File *file;
    std::tie(mtime, file) = mtime_queue.top();
    mtime_queue.pop();

    if (dep_mtime[file] != mtime) {
      continue;
    }

    auto file_includes_it = reverse_includes.find(file);
    if (file_includes_it == reverse_includes.end()) {
      continue;
    }

    for (auto including_file: file_includes_it->second) {
      auto &including_file_mtime = dep_mtime[including_file];
      if (including_file_mtime < mtime) {
        including_file_mtime = mtime;
        mtime_queue.emplace(including_file_mtime, including_file);
      }
    }
  }
  return dep_mtime;
}

std::vector<File *> create_obj_files(KphpMake *make, Index &obj_dir, const Index &cpp_dir,
                                     const std::forward_list<Index> &imported_headers) {
  std::vector<File *> files = cpp_dir.get_files();
  std::unordered_map<File *, long long> dep_mtime = create_dep_mtime(cpp_dir, imported_headers);

  std::vector<File *> objs;
  for (auto cpp_file : files) {
    if (cpp_file->ext == ".cpp") {
      File *obj_file = obj_dir.insert_file(cpp_file->name_without_ext + ".o");
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
    File *obj_file = obj_dir.insert_file(name_and_files.first);
    make->create_objs2obj_target(name_and_files.second, obj_file);
    objs.push_back(obj_file);
  }
  fprintf(stderr, "objs cnt = %zu\n", objs.size());
  return objs;
}

bool kphp_make(File &bin, Index &obj_dir, const Index &cpp_dir, std::forward_list<File> imported_libs,
               const std::forward_list<Index> &imported_headers, const KphpEnviroment &kphp_env,
               const std::string &gch_dir) {
  KphpMake make;
  std::vector<File *> lib_objs;
  for (File &link_file: imported_libs) {
    make.create_cpp_target(&link_file);
    lib_objs.emplace_back(&link_file);
  }
  std::vector<File *> objs = create_obj_files(&make, obj_dir, cpp_dir, imported_headers);
  std::copy(lib_objs.begin(), lib_objs.end(), std::back_inserter(objs));
  make.create_objs2bin_target(objs, &bin);
  make.init_env(kphp_env);
  if (!gch_dir.empty()) {
    make.add_gch_dir(gch_dir);
  }
  return make.make_target(&bin, kphp_env.get_jobs_count());
}

bool kphp_make_static_lib(File &static_lib, Index &obj_dir, const Index &cpp_dir,
                          const std::forward_list<Index> &imported_headers, const KphpEnviroment &kphp_env,
                          const std::string &gch_dir) {
  KphpMake make;
  std::vector<File *> objs = create_obj_files(&make, obj_dir, cpp_dir, imported_headers);
  make.create_objs2static_lib_target(objs, &static_lib);
  make.init_env(kphp_env);
  if (!gch_dir.empty()) {
    make.add_gch_dir(gch_dir);
  }
  return make.make_target(&static_lib, kphp_env.get_jobs_count());
}

void CompilerCore::copy_static_lib_to_out_dir(File &&static_archive) const {
  Index out_dir;
  out_dir.set_dir(env().get_static_lib_out_dir());
  out_dir.del_extra_files();

  // copy static archive
  LibData out_lib(env().get_static_lib_name(), out_dir.get_dir());
  hard_link_or_copy(static_archive.path, out_lib.static_archive_path());
  static_archive.unlink();

  // copy functions.txt of this static archive
  File functions_txt_tmp(env().get_dest_cpp_dir() + LibData::functions_txt_tmp_file());
  hard_link_or_copy(functions_txt_tmp.path, out_lib.functions_txt_file());
  static_archive.unlink();

  // copy runtime lib sha256 of this static archive
  File runtime_lib_sha256(env().get_runtime_sha256_file());
  hard_link_or_copy(runtime_lib_sha256.path, out_lib.runtime_lib_sha256_file());

  Index headers_tmp_dir;
  headers_tmp_dir.sync_with_dir(env().get_dest_cpp_dir() + LibData::headers_tmp_dir());
  Index out_headers_dir;
  out_headers_dir.set_dir(out_lib.headers_dir());
  // copy cpp header files of this static archive
  for (File *header_file: headers_tmp_dir.get_files()) {
    hard_link_or_copy(header_file->path, out_headers_dir.get_dir() + header_file->name);
    static_archive.unlink();
  }
}

std::forward_list<File> CompilerCore::collect_imported_libs() {
  const string &binary_runtime_sha256 = env().get_runtime_sha256();
  stage::die_if_global_errors();

  std::forward_list<File> imported_libs;
  imported_libs.emplace_front(env().get_link_file());
  for (const auto &lib: get_libs()) {
    if (lib && !lib->is_raw_php()) {
      std::string lib_runtime_sha256 = KphpEnviroment::read_runtime_sha256_file(lib->runtime_lib_sha256_file());

      kphp_error_act(binary_runtime_sha256 == lib_runtime_sha256,
                     format("Mismatching between sha256 of binary runtime '%s' and lib runtime '%s'",
                            env().get_runtime_sha256_file().c_str(), lib->runtime_lib_sha256_file().c_str()),
                     continue);
      imported_libs.emplace_front(lib->static_archive_path());
    }
  }

  for (File &file: imported_libs) {
    kphp_error_act(file.upd_mtime() > 0, format("Can't read mtime of link_file [%s]", file.path.c_str()), continue);
    if (env().get_verbosity() >= 1) {
      fprintf(stderr, "Use static lib [%s]\n", file.path.c_str());
    }
  }

  stage::die_if_global_errors();
  return imported_libs;
}

std::forward_list<Index> CompilerCore::collect_imported_headers() {
  std::forward_list<Index> imported_headers;
  for (const auto &lib: get_libs()) {
    if (lib && !lib->is_raw_php()) {
      imported_headers.emplace_front();
      imported_headers.front().sync_with_dir(lib->lib_dir());
    }
  }
  return imported_headers;
}

void CompilerCore::make() {
  AutoProfiler profiler{get_profiler("make")};
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

  std::string gch_dir;
  bool ok = true;
  const bool pch_allowed = !env().get_no_pch();
  if (pch_allowed) {
    gch_dir = kphp_make_precompiled_header(&obj_index, env());
    ok = !gch_dir.empty();
    kphp_error (ok, "Make precompiled header failed");
  }
  if (ok) {
    auto lib_header_dirs = collect_imported_headers();
    ok = env().is_static_lib_mode()
         ? kphp_make_static_lib(bin_file, obj_index, cpp_index, lib_header_dirs, env(), gch_dir)
         : kphp_make(bin_file, obj_index, cpp_index, collect_imported_libs(), lib_header_dirs, env(), gch_dir);
    kphp_error (ok, "Make failed");
  }

  stage::die_if_global_errors();
  obj_index.del_extra_files();

  if (!env().get_user_binary_path().empty()) {
    hard_link_or_copy(bin_file.path, env().get_user_binary_path());
  }
  if (env().is_static_lib_mode()) {
    copy_static_lib_to_out_dir(std::move(bin_file));
  }
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


