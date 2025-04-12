// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once


/*** Core ***/
//Consists mostly of functions that require synchronization

#include <string>
#include <vector>

#include "common/algorithms/hashes.h"

#include "compiler/compiler-settings.h"
#include "compiler/composer.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/ffi-data.h"
#include "compiler/function-colors.h"
#include "compiler/index.h"
#include "compiler/stats.h"
#include "compiler/threading/data-stream.h"
#include "compiler/threading/hash-table.h"
#include "compiler/tl-classes.h"

enum class OutputMode {
  server,       // -M server
  cli,          // -M cli
  lib,          // -M lib
  k2_cli,       // -M k2-cli
  k2_server,    // -M k2-server
  k2_oneshot,   // -M k2-oneshot
  k2_multishot, // -M k2-multishot
};

class CompilerCore {
private:
  Index cpp_index;
  Index runtime_common_sources_index;
  Index runtime_sources_index;
  Index common_sources_index;
  Index unicode_sources_index;
  TSHashTable<SrcFilePtr> file_ht;
  TSHashTable<SrcDirPtr> dirs_ht;
  TSHashTable<FunctionPtr, 2'000'000> functions_ht;
  TSHashTable<ClassPtr> classes_ht;
  TSHashTable<DefinePtr> defines_ht;
  TSHashTable<VarPtr,2'000'000> constants_ht;   // auto-collected constants (const strings / arrays / regexps / pure func calls); are inited once in a master process
  TSHashTable<VarPtr> globals_ht;               // mutable globals (vars in global scope, class static fields); are reset for each php script inside worker processes
  TSHashTable<LibPtr, 1000> libs_ht;
  TSHashTable<ModulitePtr, 10000> modulites_ht;
  TSHashTable<ComposerJsonPtr, 10000> composer_json_ht;
  SrcFilePtr main_file;
  CompilerSettings *settings_;
  ComposerAutoloader composer_class_loader;
  OutputMode output_mode;
  FFIRoot ffi;
  ClassPtr memcache_class;
  TlClasses tl_classes;
  std::vector<std::string> kphp_runtime_opts;
  std::vector<std::string> exclude_namespaces;
  bool is_untyped_rpc_tl_used{false};
  bool is_functions_txt_parsed{false};
  function_palette::Palette function_palette;

  inline bool try_require_file(SrcFilePtr file);

public:
  std::string cpp_dir;

  // Don't like that, handle in another way
  std::string runtime_common_sources_dir;
  std::string runtime_sources_dir;
  std::string common_sources_dir;
  std::string unicode_sources_dir;

  CompilerCore();
  void start();
  void finish();
  void register_settings(CompilerSettings *settings);
  const CompilerSettings &settings() const;
  const std::string &get_global_namespace() const;

  vk::string_view calc_relative_name(SrcFilePtr file, bool builtin) const;
  std::string search_required_file(const std::string &file_name) const;
  std::string search_file_in_include_dirs(const std::string &file_name, size_t *dir_index = nullptr) const;
  SrcFilePtr register_file(const std::string &file_name, LibPtr owner_lib, bool builtin = false);
  SrcDirPtr register_dir(vk::string_view full_dir_name);

  FFIRoot &get_ffi_root();
  OutputMode get_output_mode() const;

  void register_main_file(const std::string &file_name, DataStream<SrcFilePtr> &os);
  SrcFilePtr require_file(const std::string &file_name, LibPtr owner_lib, DataStream<SrcFilePtr> &os, bool error_if_not_exists = true, bool builtin = false);

  void require_function(const std::string &name, DataStream<FunctionPtr> &os);
  void require_function(FunctionPtr function, DataStream<FunctionPtr> &os);

  template <class CallbackT>
  void operate_on_function_locking(const std::string &name, CallbackT callback) {
    static_assert(std::is_constructible<std::function<void(FunctionPtr&)>, CallbackT>::value, "invalid callback signature");

    auto *node = functions_ht.at(vk::std_hash(name));
    AutoLocker<Lockable *> locker(node);
    callback(node->data);
  }

  void register_function(FunctionPtr function);
  void register_and_require_function(FunctionPtr function, DataStream<FunctionPtr> &os, bool force_require = false);
  ClassPtr try_register_class(ClassPtr cur_class);
  bool register_class(ClassPtr cur_class);
  LibPtr register_lib(LibPtr lib);
  ModulitePtr register_modulite(ModulitePtr modulite);
  ComposerJsonPtr register_composer_json(ComposerJsonPtr composer_json);

  FunctionPtr get_function(const std::string &name);
  ModulitePtr get_modulite(vk::string_view name);
  ComposerJsonPtr get_composer_json(vk::string_view name);
  ComposerJsonPtr get_composer_json_at_dir(SrcDirPtr dir);
  ClassPtr get_class(vk::string_view name);
  ClassPtr get_memcache_class();
  void set_memcache_class(ClassPtr klass);

  bool register_define(DefinePtr def_id);
  DefinePtr get_define(std::string_view name);

  VarPtr create_var(const std::string &name, VarData::Type type);
  VarPtr get_global_var(const std::string &name, VertexPtr init_val);
  VarPtr get_constant_var(const std::string &name, VertexPtr init_val, bool *is_new_inserted = nullptr);
  VarPtr create_local_var(FunctionPtr function, const std::string &name, VarData::Type type);

  SrcFilePtr get_main_file() { return main_file; }
  std::vector<VarPtr> get_global_vars();
  std::vector<VarPtr> get_constants_vars();
  std::vector<ClassPtr> get_classes();
  std::vector<InterfacePtr> get_interfaces();
  std::vector<DefinePtr> get_defines();
  std::vector<LibPtr> get_libs();
  std::vector<SrcDirPtr> get_dirs();
  std::vector<ModulitePtr> get_modulites();
  const ComposerAutoloader &get_composer_autoloader() const;

  void load_index();
  void save_index();
  const Index &get_index();
  const Index &get_runtime_core_index();
  const Index &get_runtime_index();
  const Index &get_common_index();
  const Index &get_unicode_index();
  File *get_file_info(std::string &&file_name);
  void del_extra_files();
  void init_dest_dir();
  void init_runtime_and_common_srcs_dir();

  void try_load_tl_classes();
  void init_composer_class_loader();
  const TlClasses &get_tl_classes() const { return tl_classes; }

  void add_kphp_runtime_opt(std::string opt) { kphp_runtime_opts.emplace_back(std::move(opt)); }
  const std::vector<std::string> &get_kphp_runtime_opts() const { return kphp_runtime_opts; }

  void set_function_palette(function_palette::Palette &&palette) {
    function_palette = palette;
  }

  function_palette::Palette &get_function_palette() {
    return function_palette;
  }

  void set_untyped_rpc_tl_used() {
    is_untyped_rpc_tl_used = true;
  }

  bool get_untyped_rpc_tl_used() const {
    return is_untyped_rpc_tl_used;
  }

  void set_functions_txt_parsed() {
    is_functions_txt_parsed = true;
  }

  bool get_functions_txt_parsed() const {
    return is_functions_txt_parsed;
  }

  bool is_output_mode_server() const {
    return output_mode == OutputMode::server;
  }

  bool is_output_mode_cli() const {
    return output_mode == OutputMode::cli;
  }

  bool is_output_mode_lib() const {
    return output_mode == OutputMode::lib;
  }

  bool is_output_mode_k2_cli() const {
    return output_mode == OutputMode::k2_cli;
  }

  bool is_output_mode_k2_server() const {
    return output_mode == OutputMode::k2_server;
  }

  bool is_output_mode_k2_oneshot() const {
    return output_mode == OutputMode::k2_oneshot;
  }

  bool is_output_mode_k2_multishot() const {
    return output_mode == OutputMode::k2_multishot;
  }

  bool is_output_mode_k2() const {
    return is_output_mode_k2_cli() || is_output_mode_k2_server() || is_output_mode_k2_oneshot() || is_output_mode_k2_multishot();
  }

  void set_exclude_namespaces(std::vector<std::string> &&excludes) {
    exclude_namespaces = std::move(excludes);
  }

  const std::vector<std::string> &get_exclude_namespaces() const {
    return exclude_namespaces;
  }

  void update_hash_tables_stats();

  Stats stats;
};

extern CompilerCore *G;

