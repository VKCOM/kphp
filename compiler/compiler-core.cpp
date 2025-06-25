// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/compiler-core.h"

#include <atomic>
#include <dirent.h>
#include <cctype>

#include "common/algorithms/contains.h"
#include "common/wrappers/mkdir_recursive.h"
#include "common/smart_ptrs/unique_ptr_with_delete_function.h"

#include "compiler/const-manipulations.h"
#include "compiler/data/composer-json-data.h"
#include "compiler/data/define-data.h"
#include "compiler/data/ffi-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/lib-data.h"
#include "compiler/data/modulite-data.h"
#include "compiler/data/src-dir.h"
#include "compiler/data/src-file.h"
#include "compiler/index.h"
#include "compiler/name-gen.h"
#include "compiler/runtime_build_info.h"

namespace {

void close_dir(DIR *d) {
  closedir(d);
}

void collect_composer_folders(const std::string &path, std::vector<std::string> &result) {
  // can't use nftw here as it doesn't provide a portable way to stop directory traversal;
  // we don't want to visit *all* files in the vendor tree
  //
  // suppose we have this composer-generated layout:
  //     vendor/pkg1/
  //     * composer.json
  //     * src/
  //     vendor/ns/pkg2/
  //     * composer.json
  //     * src/
  // all pkg directories can have a lot of files inside src/,
  // if we can stop as soon as we find composer.json, a lot of
  // redundant work is avoided

  vk::unique_ptr_with_delete_function<DIR, close_dir> dp{opendir(path.c_str())};
  if (dp == nullptr) {
    kphp_warning(fmt_format("find composer files: opendir({}) failed: {}", path.c_str(), strerror(errno)));
    return;
  }

  // since composer package can't have nested composer.json file, we stop
  // directory traversal if we found it; otherwise we descend further;
  // dirs contains all directories that we need to visit when descending
  bool recurse = true;
  std::vector<std::string> dirs;

  while (const auto *entry = readdir(dp.get())) {
    if (entry->d_name[0] == '.') {
      continue;
    }

    if (std::strcmp(entry->d_name, "composer.json") == 0) {
      result.push_back(path);
      recurse = false;
      break;
    }

    // by default, composer does no copy for packages; it creates a symlink instead
    if (entry->d_type == DT_LNK) {
      // collect only those links that point to a directory
      auto link_path = path + "/" + entry->d_name;
      struct stat link_info;
      stat(link_path.c_str(), &link_info);
      if (S_ISDIR(link_info.st_mode)) {
        dirs.push_back(std::move(link_path));
      }
    } else if (entry->d_type == DT_DIR) {
      dirs.emplace_back(path + "/" + entry->d_name);
    }
  }

  if (recurse) {
    for (const auto &dir : dirs) {
      collect_composer_folders(dir, result);
    }
  }
}

// Collect all composer.json file roots that can be found in the given directory.
std::vector<std::string> find_composer_folders(const std::string &dir) {
  std::vector<std::string> result;
  collect_composer_folders(dir, result);
  return result;
}

}

static FunctionPtr UNPARSED_BUT_REQUIRED_FUNC_PTR = FunctionPtr(reinterpret_cast<FunctionData *>(0x0001));

CompilerCore::CompilerCore() :
  settings_(nullptr) {
}

void CompilerCore::start() {
  stage::die_if_global_errors();
}

void CompilerCore::finish() {
  if (stage::warnings_count > 0) {
    fmt_print("[{} WARNINGS GENERATED]\n", stage::warnings_count);
  }
  stage::die_if_global_errors();
  del_extra_files();
  save_index();
  stage::die_if_global_errors();

  delete settings_;
  settings_ = nullptr;
}

void CompilerCore::register_settings(CompilerSettings *settings) {
  kphp_assert(settings_ == nullptr);
  settings_ = settings;
  const auto mode = settings->mode.get();

  if (mode == "cli") {
    output_mode = OutputMode::cli;
  } else if (mode == "lib") {
    output_mode = OutputMode::lib;
  } else if (mode == "k2-cli") {
    output_mode = OutputMode::k2_cli;
  } else if (mode == "k2-server") {
    output_mode = OutputMode::k2_server;
  } else if (mode == "k2-oneshot") {
    output_mode = OutputMode::k2_oneshot;
  } else if (mode == "k2-multishot") {
    output_mode = OutputMode::k2_multishot;
  } else {
    output_mode = OutputMode::server;
  }
}

const CompilerSettings &CompilerCore::settings() const {
  kphp_assert (settings_ != nullptr);
  return *settings_;
}

const std::string &CompilerCore::get_global_namespace() const {
  return settings().static_lib_name.get();
}

FunctionPtr CompilerCore::get_function(const std::string &name) {
  auto *node = functions_ht.at(vk::std_hash(name));
  AutoLocker<Lockable *> locker(node);
  if (!node->data || node->data == UNPARSED_BUT_REQUIRED_FUNC_PTR) {
    return {};
  }

  FunctionPtr f = node->data;
  kphp_assert_msg(f->name == name, fmt_format("Bug in compiler: hash collision: `{}' and `{}`", f->name, name));
  return f;
}

std::string CompilerCore::search_file_in_include_dirs(const std::string &file_name, size_t *dir_index) const {
  if (file_name.empty() || file_name[0] == '/' || file_name[0] == '.') {
    return {};
  }
  std::string full_file_name;
  size_t index = 0;
  const auto &includes = settings().includes.get();
  for (; index < includes.size() && full_file_name.empty(); ++index) {
    full_file_name = get_full_path(includes[index] + file_name);
  }
  if (!full_file_name.empty() && dir_index) {
    *dir_index = index;
  }
  return full_file_name;
}

// search_required_file resolves the file_name like it would be expanded when user as require() argument;
// it uses search_file_in_include_dirs as well as the current file relative search
std::string CompilerCore::search_required_file(const std::string &file_name) const {
  std::string full_file_name = search_file_in_include_dirs(file_name);
  if (file_name[0] == '/') {
    return get_full_path(file_name);
  }

  if (full_file_name.empty()) {
    std::vector<std::string> cur_include_dirs;
    SrcFilePtr from_file = stage::get_file();
    if (from_file) {
      std::string from_file_name = from_file->file_name;
      size_t en = from_file_name.find_last_of('/');
      assert (en != std::string::npos);
      std::string cur_dir = from_file_name.substr(0, en + 1);
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

  return full_file_name;
}

FFIRoot &CompilerCore::get_ffi_root() {
  return ffi;
}

OutputMode CompilerCore::get_output_mode() const {
  return output_mode;
}


vk::string_view CompilerCore::calc_relative_name(SrcFilePtr file, bool builtin) const {
  vk::string_view full_file_name = file->file_name;
  if (full_file_name.starts_with(settings_->base_dir.get())) {
    return full_file_name.substr(settings_->base_dir.get().size());
  } else if (settings_->is_composer_enabled() && full_file_name.starts_with(settings_->composer_root.get())) {
    return full_file_name.substr(settings_->composer_root.get().size());
  } else if (builtin) {
    return file->short_file_name;
  } else {
    return full_file_name;
  }
}

SrcFilePtr CompilerCore::register_file(const std::string &file_name, LibPtr owner_lib, bool builtin) {
  if (file_name.empty()) {
    return {};
  }

  std::string full_file_name = search_required_file(file_name);

  if (full_file_name.empty()) {
    return {};
  }

  size_t last_pos_of_slash = full_file_name.find_last_of('/');
  last_pos_of_slash = last_pos_of_slash != std::string::npos ? last_pos_of_slash + 1 : 0;

  size_t last_pos_of_dot = full_file_name.find_last_of('.');
  if (last_pos_of_dot == std::string::npos || last_pos_of_dot < last_pos_of_slash) {
    last_pos_of_dot = full_file_name.length();
  }

  vk::string_view full_dir_name = vk::string_view{full_file_name.c_str(), last_pos_of_slash};
  std::string short_file_name = full_file_name.substr(last_pos_of_slash, last_pos_of_dot - last_pos_of_slash);
  std::string extension = full_file_name.substr(std::min(full_file_name.length(), last_pos_of_dot + 1));
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
      new_file->is_from_functions_file = builtin;
      new_file->relative_file_name = static_cast<std::string>(calc_relative_name(new_file, builtin));
      size_t last_slash = new_file->relative_file_name.rfind('/');
      new_file->relative_dir_name = last_slash == std::string::npos ? "" : new_file->relative_file_name.substr(0, last_slash);

      std::string func_name = "src_" + new_file->short_file_name + fmt_format("{:x}", vk::std_hash(new_file->relative_file_name));
      new_file->main_func_name = replace_non_alphanum(std::move(func_name));
      node->data = new_file;
    }
  }

  TSHashTable<SrcDirPtr>::HTNode *node_file_dir = dirs_ht.at(vk::std_hash(full_dir_name));
  if (!node_file_dir->data) {
    AutoLocker<Lockable *> locker(node_file_dir);
    if (!node_file_dir->data) {
      node_file_dir->data = register_dir(full_dir_name);
    }
  }

  SrcFilePtr file = node->data;
  file->dir = node_file_dir->data;
  kphp_assert(file && file->dir && file->dir->parent_dir);

  return file;
}

SrcDirPtr CompilerCore::register_dir(vk::string_view full_dir_name) {
  static CachedProfiler cache{"Load src dirs tree"};
  AutoProfiler prof{*cache};

  SrcDirPtr dir = SrcDirPtr(new SrcDir(static_cast<std::string>(full_dir_name)));

  if (full_dir_name.size() > 1) {
    size_t last_pos_of_slash = full_dir_name.rfind('/', full_dir_name.size() - 2);
    vk::string_view parent_dir_name = full_dir_name.substr(0, last_pos_of_slash + 1);

    TSHashTable<SrcDirPtr>::HTNode *node_parent_dir = dirs_ht.at(vk::std_hash(parent_dir_name));
    if (!node_parent_dir->data) {
      AutoLocker<Lockable *> locker(node_parent_dir);
      if (!node_parent_dir->data) {
        node_parent_dir->data = register_dir(parent_dir_name);
      }
    }
    dir->parent_dir = node_parent_dir->data;
//    printf("%s -> %s\n", dir->full_dir_name.c_str(), dir->parent_dir->full_dir_name.c_str());
  }

  return dir;
}

void CompilerCore::require_function(const std::string &name, DataStream<FunctionPtr> &os) {
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

void CompilerCore::parse_tracked_builtins(const std::string& builtins_list) noexcept {
  constexpr auto skip_whitespace = [](const std::string & str, size_t start_pos) noexcept {
    while (start_pos < str.size() && std::isspace(str[start_pos]) != 0) {
      start_pos++;
    }
    return start_pos;
  };
  constexpr auto read_builtin = [](const std::string & str, size_t start_pos) noexcept {
    while (start_pos < str.size() && std::isspace(str[start_pos]) == 0) {
      start_pos++;
    }
    return start_pos;
  };

  for (size_t index = 0; index < builtins_list.size();) {
    const size_t builtin_name_start = skip_whitespace(builtins_list, index);
    const size_t builtin_name_end = read_builtin(builtins_list, builtin_name_start);

    tracked_builtins.emplace(builtins_list.substr(builtin_name_start, builtin_name_end - builtin_name_start));
    index = builtin_name_end;
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
               fmt_format("Redeclaration of function {}(), the previous declaration was in {}", function->as_human_readable(), f->file_id->file_name));
    f = function;

    if (was_previously_required || force_require) {
      require_function(f, os);
    }
  });
}

ClassPtr CompilerCore::try_register_class(ClassPtr cur_class) {
  TSHashTable<ClassPtr>::HTNode *node = classes_ht.at(vk::std_hash(cur_class->name));
  AutoLocker<Lockable *> locker(node);
  if (!node->data) {
    node->data = cur_class;
  }
  return node->data;
}

bool CompilerCore::register_class(ClassPtr cur_class) {
  auto registered_class = try_register_class(cur_class);
  kphp_error (registered_class == cur_class,
              fmt_format("Redeclaration of class {}, the previous declaration was in {}", cur_class->name, registered_class->file_id->file_name));
  return registered_class == cur_class;
}

LibPtr CompilerCore::register_lib(LibPtr lib) {
  TSHashTable<LibPtr, 1000>::HTNode *node = libs_ht.at(vk::std_hash(lib->lib_namespace()));
  AutoLocker<Lockable *> locker(node);
  if (!node->data) {
    node->data = lib;
  }
  return node->data;
}

ModulitePtr CompilerCore::register_modulite(ModulitePtr modulite) {
  auto *node = modulites_ht.at(vk::std_hash(modulite->modulite_name));
  AutoLocker<Lockable *> locker(node);
  kphp_error(!node->data, fmt_format("Redeclaration of modulite {}, declared in:\n- {}\n- {}", modulite->modulite_name, modulite->yaml_file->relative_file_name, node->data->yaml_file->relative_file_name));
  node->data = modulite;
  return node->data;
}

ModulitePtr CompilerCore::get_modulite(vk::string_view name) {
  const auto *result = modulites_ht.find(vk::std_hash(name));
  return result ? *result : ModulitePtr{};
}

ComposerJsonPtr CompilerCore::register_composer_json(ComposerJsonPtr composer_json) {
  auto *node = composer_json_ht.at(vk::std_hash(composer_json->package_name));
  AutoLocker<Lockable *> locker(node);
  kphp_error(!node->data, fmt_format("Redeclaration of composer package {}, declared in:\n- {}\n- {}", composer_json->package_name, composer_json->json_file->relative_file_name, node->data->json_file->relative_file_name));
  node->data = composer_json;
  kphp_assert(composer_json->json_file->dir);
  composer_json->json_file->dir->has_composer_json = true;
  return node->data;
}

ComposerJsonPtr CompilerCore::get_composer_json(vk::string_view name) {
  const auto *result = composer_json_ht.find(vk::std_hash(name));
  return result ? *result : ComposerJsonPtr{};
}

ComposerJsonPtr CompilerCore::get_composer_json_at_dir(SrcDirPtr dir) {
  std::vector<ComposerJsonPtr> at_dir = composer_json_ht.get_all_if([dir](ComposerJsonPtr j) {
    return j->json_file->dir == dir;
  });
  kphp_assert(at_dir.size() < 2);
  return at_dir.empty() ? ComposerJsonPtr{} : at_dir.front();
}

void CompilerCore::register_main_file(const std::string &file_name, DataStream<SrcFilePtr> &os) {
  kphp_assert(!main_file);

  SrcFilePtr res = register_file(file_name, LibPtr{});
  kphp_error (file_name.empty() || res, fmt_format("Cannot load main file [{}]", file_name));

  if (res && try_require_file(res)) {
    main_file = res;
    os << res;
  }
}

SrcFilePtr CompilerCore::require_file(const std::string &file_name, LibPtr owner_lib, DataStream<SrcFilePtr> &os, bool error_if_not_exists /* = true */, bool builtin) {
  SrcFilePtr file = register_file(file_name, owner_lib, builtin);
  kphp_error (file || !error_if_not_exists, fmt_format("Cannot load file [{}]", file_name));
  if (file && try_require_file(file)) {
    os << file;
  }
  return file;
}


ClassPtr CompilerCore::get_class(vk::string_view name) {
  const auto *result = classes_ht.find(vk::std_hash(name));
  return result ? *result : ClassPtr{};
}

ClassPtr CompilerCore::get_memcache_class() {
  if (!memcache_class) {            // if specific memcache implementation is not set
    return get_class("Memcache");   // take it from the functions.txt
  }
  return memcache_class;
}

void CompilerCore::set_memcache_class(ClassPtr klass) {
  kphp_error(!memcache_class || memcache_class == klass,
             fmt_format("Duplicate Memcache realization {} and {}", memcache_class->name, klass->name));
  memcache_class = klass;
}

bool CompilerCore::register_define(DefinePtr def_id) {
  TSHashTable<DefinePtr>::HTNode *node = defines_ht.at(vk::std_hash(def_id->name));
  AutoLocker<Lockable *> locker(node);

  kphp_error_act (
    !node->data,
    fmt_format("Redeclaration of define [{}], the previous declaration was in [{}]",
               def_id->name, node->data->file_id->file_name),
    return false
  );

  node->data = def_id;
  return true;
}

DefinePtr CompilerCore::get_define(std::string_view name) {
  // we don't support namespaces for constants yet, but we
  // permit var_dump(\foo) syntax and interpret it as just var_dump(foo);
  if (!name.empty() && name.front() == '\\') {
    name.remove_prefix(1);
  }
  const auto *result = defines_ht.find(vk::std_hash(name));
  return result ? *result : DefinePtr{};
}

VarPtr CompilerCore::create_var(const std::string &name, VarData::Type type) {
  VarPtr var = VarPtr(new VarData(type));
  var->name = name;
  var->tinf_node.init_as_variable(var);
  stats.on_var_inserting(type);
  return var;
}

VarPtr CompilerCore::get_global_var(const std::string &name, VertexPtr init_val) {
  auto *node = globals_ht.at(vk::std_hash(name));

  if (!node->data) {
    AutoLocker<Lockable *> locker(node);
    if (!node->data) {
      node->data = create_var(name, VarData::var_global_t);
      node->data->init_val = init_val;
      node->data->is_builtin_runtime = VarData::does_name_eq_any_builtin_runtime(name);
    }
  }

  return node->data;
}

VarPtr CompilerCore::get_constant_var(const std::string &name, VertexPtr init_val, bool *is_new_inserted) {
  auto *node = constants_ht.at(vk::std_hash(name));
  VarPtr new_var;
  if (!node->data) {
    AutoLocker<Lockable *> locker(node);
    if (!node->data) {
      new_var = create_var(name, VarData::var_const_t);
      new_var->init_val = init_val;
      node->data = new_var;
    }
  }

  // when a string 'str' meets in several places in php code (same for [1,2,3] and other consts)
  // it's created by a thread that first found it, and all others just ref to the same node
  // here we make var->init_val->location stable, as it's sometimes used in code generation (locations of regexps, for example)
  if (!new_var) {
    AutoLocker<Lockable *> locker(node);
    if (node->data->init_val->get_location() < init_val->get_location()) {
      std::swap(node->data->init_val, init_val);
    }
  }

  if (is_new_inserted) {
    *is_new_inserted = static_cast<bool>(new_var);
  }

  VarPtr var = node->data;
  // assert that one and the same init_val leads to one and the same var
  if (!new_var) {
    kphp_assert(init_val);
    kphp_assert(var->init_val->type() == init_val->type());
    kphp_assert_msg(var->name == name, fmt_format("bug in compiler (hash collision) {} {}", var->name, name));

    switch (init_val->type()) {
      case op_string:
        kphp_assert(var->init_val->get_string() == init_val->get_string());
        break;
      case op_conv_regexp: {
        const std::string &new_regexp = init_val.as<op_conv_regexp>()->expr().as<op_string>()->str_val;
        const std::string &hashed_regexp = var->init_val.as<op_conv_regexp>()->expr().as<op_string>()->str_val;
        kphp_assert_msg(hashed_regexp == new_regexp, fmt_format("hash collision of regexp: {} vs {}", new_regexp, hashed_regexp));
        break;
      }
      case op_array: {
        std::string new_array_repr = VertexPtrFormatter::to_string(init_val);
        std::string hashed_array_repr = VertexPtrFormatter::to_string(var->init_val);
        kphp_assert_msg(new_array_repr == hashed_array_repr, fmt_format("hash collision of arrays: {} vs {}", new_array_repr, hashed_array_repr));
        break;
      }
      default:
        break;
    }
  }
  return var;
}

VarPtr CompilerCore::create_local_var(FunctionPtr function, const std::string &name, VarData::Type type) {
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
      var->tinf_node.init_as_argument(var);
      function->param_ids.push_back(var);
      break;
    default:
    kphp_fail();
  }
  return var;
}

std::vector<VarPtr> CompilerCore::get_global_vars() {
  return globals_ht.get_all_if([](VarPtr v) {
    // traits' static vars are added at the moment of parsing (class-members.cpp)
    // but later never used, and tinf never executed for them
    if (v->is_class_static_var() && v->class_id->is_trait()) {
      return false;
    }
    // static vars for classes that are unused, are also present here
    // probably, in the future, we'll detect unused globals and don't export them to C++ even as Unknown
    return true;
  });
}

std::vector<VarPtr> CompilerCore::get_constants_vars() {
  return constants_ht.get_all();
}

std::vector<ClassPtr> CompilerCore::get_classes() {
  return classes_ht.get_all();
}

std::vector<InterfacePtr> CompilerCore::get_interfaces() {
  return classes_ht.get_all_if([](ClassPtr klass) {
    return klass->is_interface();
  });
}

std::vector<DefinePtr> CompilerCore::get_defines() {
  return defines_ht.get_all();
}

std::vector<LibPtr> CompilerCore::get_libs() {
  return libs_ht.get_all();
}

std::vector<SrcDirPtr> CompilerCore::get_dirs() {
  return dirs_ht.get_all();
}

std::vector<ModulitePtr> CompilerCore::get_modulites() {
  return modulites_ht.get_all();
}

const ComposerAutoloader &CompilerCore::get_composer_autoloader() const {
  return composer_class_loader;
}

void CompilerCore::load_index() {
  if (!settings().no_index_file.get()) {
    cpp_index.load_from_index_file();
  }
}

void CompilerCore::save_index() {
  if (!settings().no_index_file.get()) {
    cpp_index.save_into_index_file();
  }
}

const Index &CompilerCore::get_index() {
  return cpp_index;
}

const Index &CompilerCore::get_runtime_index() {
  return runtime_sources_index;
}

const Index &CompilerCore::get_runtime_core_index() {
  return runtime_common_sources_index;
}

const Index &CompilerCore::get_common_index() {
  return common_sources_index;
}

const Index &CompilerCore::get_unicode_index() {
  return unicode_sources_index;
}

File *CompilerCore::get_file_info(std::string &&file_name) {
  return cpp_index.insert_file(std::move(file_name));
}

void CompilerCore::del_extra_files() {
  cpp_index.del_extra_files();
}

void CompilerCore::init_dest_dir() {
  cpp_dir = settings().dest_cpp_dir.get();
  cpp_index.sync_with_dir(cpp_dir);
  cpp_dir = cpp_index.get_dir();
}

static std::vector<std::string> get_runtime_common_sources() {
#ifdef RUNTIME_LIGHT
  return split(RUNTIME_COMMON_SOURCES, ';');
#else
  return {};
#endif
}

static std::vector<std::string> get_runtime_sources() {
#ifdef RUNTIME_LIGHT
  return split(RUNTIME_SOURCES, ';');
#else
  return {};
#endif
}

static std::vector<std::string> get_common_sources() {
#ifdef RUNTIME_LIGHT
  return split(COMMON_SOURCES, ';');
#else
  return {};
#endif
}

static std::vector<std::string> get_unicode_sources() {
#ifdef RUNTIME_LIGHT
  return split(UNICODE_SOURCES, ';');
#else
  return {};
#endif
}

void CompilerCore::init_runtime_and_common_srcs_dir() {
  runtime_common_sources_dir = settings().runtime_and_common_src.get() + "runtime-common/";
  runtime_common_sources_index.sync_with_dir(runtime_common_sources_dir);
  runtime_common_sources_dir = runtime_common_sources_index.get_dir();
  runtime_common_sources_index.filter_with_whitelist(get_runtime_common_sources());

  runtime_sources_dir = settings().runtime_and_common_src.get() + "runtime-light/";
  runtime_sources_index.sync_with_dir(runtime_sources_dir);
  runtime_sources_dir = runtime_sources_index.get_dir(); // As in init_dest_dir, IDK what is it for
  runtime_sources_index.filter_with_whitelist(get_runtime_sources());

  common_sources_dir = settings().runtime_and_common_src.get() + "common/";
  common_sources_index.sync_with_dir(common_sources_dir);
  common_sources_dir = common_sources_index.get_dir(); // As in init_dest_dir, IDK what is it for
  common_sources_index.filter_with_whitelist(get_common_sources());

  unicode_sources_dir = settings().runtime_and_common_src.get() + "common/unicode/";
  unicode_sources_index.sync_with_dir(unicode_sources_dir);
  unicode_sources_dir = unicode_sources_index.get_dir(); // As in init_dest_dir, IDK what is it for
  unicode_sources_index.filter_with_whitelist(get_unicode_sources());
}

bool CompilerCore::try_require_file(SrcFilePtr file) {
  return __sync_bool_compare_and_swap(&file->is_required, false, true);
}

void CompilerCore::try_load_tl_classes() {
  if (!settings().tl_schema_file.get().empty()) {
    tl_classes.load_from(settings().tl_schema_file.get(), G->settings().gen_tl_internals.get());
  }
}

void CompilerCore::init_composer_class_loader() {
  if (!settings().is_composer_enabled()) {
    return;
  }

  composer_class_loader.set_use_dev(settings().composer_autoload_dev.get());
  composer_class_loader.load_root_file(settings().composer_root.get());

  // We could traverse the composer file and collect all "repositories"
  // and map them with "requirements" to get the dependency list,
  // but some projects may use composer plugins that change composer
  // files before "composer install" is invoked, so the final vendor
  // folder may be generated from files that differ from the composer
  // files that we can reach. To avoid that problem, we scan the vendor
  // folder in order to collect all dependencies (both direct and indirect).

  std::string vendor = settings().composer_root.get() + "vendor";
  bool vendor_folder_exists = access(vendor.c_str(), F_OK) == 0;
  if (vendor_folder_exists) {
    for (const std::string &composer_root : find_composer_folders(vendor)) {
      composer_class_loader.load_file(composer_root);
    }
  }
}

void CompilerCore::update_hash_tables_stats() {
  stats.ht_max_files = file_ht.max_size();
  stats.ht_max_dirs = dirs_ht.max_size();
  stats.ht_max_functions = functions_ht.max_size();
  stats.ht_max_classes = classes_ht.max_size();
  stats.ht_max_defines = defines_ht.max_size();
  stats.ht_max_constants = constants_ht.max_size();
  stats.ht_max_globals = globals_ht.max_size();
  stats.ht_max_libs = libs_ht.max_size();
  stats.ht_max_modulites = modulites_ht.max_size();
  stats.ht_max_composer_jsons = composer_json_ht.max_size();

  stats.ht_total_files = file_ht.get_size();
  stats.ht_total_dirs = dirs_ht.get_size();
  stats.ht_total_functions = functions_ht.get_size();
  stats.ht_total_classes = classes_ht.get_size();
  stats.ht_total_defines = defines_ht.get_size();
  stats.ht_total_constants = constants_ht.get_size();
  stats.ht_total_globals = globals_ht.get_size();
  stats.ht_total_libs = libs_ht.get_size();
  stats.ht_total_modulites = modulites_ht.get_size();
  stats.ht_total_composer_jsons = composer_json_ht.get_size();
}

CompilerCore *G;
