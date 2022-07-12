// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/compiler-core.h"

#include "common/algorithms/contains.h"
#include "common/wrappers/mkdir_recursive.h"

#include "compiler/const-manipulations.h"
#include "compiler/data/ffi-data.h"
#include "compiler/data/define-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/lib-data.h"
#include "compiler/data/src-file.h"
#include "compiler/name-gen.h"

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
  kphp_assert (settings_ == nullptr);
  settings_ = settings;
}

const CompilerSettings &CompilerCore::settings() const {
  kphp_assert (settings_ != nullptr);
  return *settings_;
}

const std::string &CompilerCore::get_global_namespace() const {
  return settings().static_lib_name.get();
}

FunctionPtr CompilerCore::get_function(const std::string &name) {
  TSHashTable<FunctionPtr>::HTNode *node = functions_ht.at(vk::std_hash(name));
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
      vk::string_view relative_file_name{new_file->file_name};
      if (relative_file_name.starts_with(settings().base_dir.get())) {
        relative_file_name.remove_prefix(settings().base_dir.get().size());
      } else if (settings().is_composer_enabled() && relative_file_name.starts_with(settings().composer_root.get())) {
        relative_file_name.remove_prefix(settings().composer_root.get().size());
      } else if (builtin) {
        relative_file_name = short_file_name;
      }
      new_file->relative_file_name = static_cast<std::string>(relative_file_name);
      size_t last_slash = new_file->relative_file_name.rfind('/');
      new_file->relative_dir_name = last_slash == std::string::npos ? "" : new_file->relative_file_name.substr(0, last_slash);

      std::string func_name = "src_" + new_file->short_file_name + fmt_format("{:x}", vk::std_hash(relative_file_name));
      new_file->main_func_name = replace_non_alphanum(std::move(func_name));
      node->data = new_file;
    }
  }
  SrcFilePtr file = node->data;
  return file;
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
  TSHashTable<LibPtr>::HTNode *node = libs_ht.at(vk::std_hash(lib->lib_namespace()));
  AutoLocker<Lockable *> locker(node);
  if (!node->data) {
    node->data = lib;
  }
  return node->data;
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

DefinePtr CompilerCore::get_define(const std::string &name) {
  return defines_ht.at(vk::std_hash(name))->data;
}

VarPtr CompilerCore::create_var(const std::string &name, VarData::Type type) {
  VarPtr var = VarPtr(new VarData(type));
  var->name = name;
  var->tinf_node.init_as_variable(var);
  stats.on_var_inserting(type);
  return var;
}

VarPtr CompilerCore::get_global_var(const std::string &name, VarData::Type type,
                                    VertexPtr init_val, bool *is_new_inserted) {
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

  if (init_val) {
    AutoLocker<Lockable *> locker(node);
    // to avoid of unstable locations, order them
    if (node->data->init_val && node->data->init_val->get_location() < init_val->get_location()) {
      std::swap(node->data->init_val, init_val);
    }
  }

  VarPtr var = node->data;
  if (is_new_inserted) {
    *is_new_inserted = static_cast<bool>(new_var);
  }
  if (!new_var) {
    kphp_assert_msg(var->name == name, fmt_format("bug in compiler (hash collision) {} {}", var->name, name));
    if (init_val) {
      kphp_assert(var->init_val->type() == init_val->type());
      switch (init_val->type()) {
        case op_string:
          kphp_assert(var->init_val->get_string() == init_val->get_string());
          break;
        case op_conv_regexp: {
          std::string &new_regexp = init_val.as<op_conv_regexp>()->expr().as<op_string>()->str_val;
          std::string &hashed_regexp = var->init_val.as<op_conv_regexp>()->expr().as<op_string>()->str_val;
          std::string msg = "hash collision: " + new_regexp + "; " + hashed_regexp;

          kphp_assert_msg(hashed_regexp == new_regexp, msg.c_str());
          break;
        }
        case op_array: {
          std::string new_array_repr = VertexPtrFormatter::to_string(init_val);
          std::string hashed_array_repr = VertexPtrFormatter::to_string(var->init_val);

          std::string msg = "hash collision: " + new_array_repr + "; " + hashed_array_repr;

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
  // static class variables are registered as globals, but if they're unused,
  // then their types were never calculated; we don't need to export them to vars.cpp
  return global_vars_ht.get_all_if([](VarPtr v) {
    return v->tinf_node.was_recalc_finished_at_least_once();
  });
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
  composer_class_loader.load(settings().composer_root.get());
}


CompilerCore *G;
