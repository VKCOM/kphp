// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once


/*** Core ***/
//Consists mostly of functions that require synchronization

#include "common/algorithms/hashes.h"

#include "compiler/data/data_ptr.h"
#include "compiler/compiler-settings.h"
#include "compiler/common.h"
#include "compiler/index.h"
#include "compiler/stats.h"
#include "compiler/threading/data-stream.h"
#include "compiler/threading/hash-table.h"
#include "compiler/tl-classes.h"
#include "compiler/composer.h"

class CompilerCore {
private:
  Index cpp_index;
  TSHashTable<SrcFilePtr> file_ht;
  TSHashTable<FunctionPtr> functions_ht;
  TSHashTable<DefinePtr> defines_ht;
  TSHashTable<VarPtr> global_vars_ht;
  TSHashTable<LibPtr> libs_ht;
  SrcFilePtr main_file;
  CompilerSettings *settings_;
  ComposerAutoloader composer_class_loader;
  TSHashTable<ClassPtr> classes_ht;
  ClassPtr memcache_class;
  TlClasses tl_classes;
  std::vector<std::string> kphp_runtime_opts;
  bool is_untyped_rpc_tl_used{false};

  inline bool try_require_file(SrcFilePtr file);

public:
  string cpp_dir;

  CompilerCore();
  void start();
  void finish();
  void register_settings(CompilerSettings *settings);
  const CompilerSettings &settings() const;
  const string &get_global_namespace() const;

  std::string search_required_file(const std::string &file_name) const;
  std::string search_file_in_include_dirs(const std::string &file_name, size_t *dir_index = nullptr) const;
  SrcFilePtr register_file(const string &file_name, LibPtr owner_lib);

  void register_main_file(const string &file_name, DataStream<SrcFilePtr> &os);
  SrcFilePtr require_file(const string &file_name, LibPtr owner_lib, DataStream<SrcFilePtr> &os, bool error_if_not_exists = true);

  void require_function(const string &name, DataStream<FunctionPtr> &os);
  void require_function(FunctionPtr function, DataStream<FunctionPtr> &os);

  template <class CallbackT>
  void operate_on_function_locking(const string &name, CallbackT callback) {
    static_assert(std::is_constructible<std::function<void(FunctionPtr&)>, CallbackT>::value, "invalid callback signature");

    TSHashTable<FunctionPtr>::HTNode *node = functions_ht.at(vk::std_hash(name));
    AutoLocker<Lockable *> locker(node);
    callback(node->data);
  }

  void register_function(FunctionPtr function);
  void register_and_require_function(FunctionPtr function, DataStream<FunctionPtr> &os, bool force_require = false);
  ClassPtr try_register_class(ClassPtr cur_class);
  bool register_class(ClassPtr cur_class);
  LibPtr register_lib(LibPtr lib);

  FunctionPtr get_function(const string &name);
  ClassPtr get_class(vk::string_view name);
  ClassPtr get_memcache_class();
  void set_memcache_class(ClassPtr klass);

  bool register_define(DefinePtr def_id);
  DefinePtr get_define(const string &name);

  VarPtr create_var(const string &name, VarData::Type type);
  VarPtr get_global_var(const string &name, VarData::Type type, VertexPtr init_val, bool *is_new_inserted = nullptr);
  VarPtr create_local_var(FunctionPtr function, const string &name, VarData::Type type);

  SrcFilePtr get_main_file() { return main_file; }
  vector<VarPtr> get_global_vars();
  vector<ClassPtr> get_classes();
  vector<DefinePtr> get_defines();
  vector<LibPtr> get_libs();
  const ComposerAutoloader &get_composer_autoloader() const;

  void load_index();
  void save_index();
  const Index &get_index();
  File *get_file_info(std::string &&file_name);
  void del_extra_files();
  void init_dest_dir();

  void try_load_tl_classes();
  void init_composer_class_loader();
  const TlClasses &get_tl_classes() const { return tl_classes; }

  void add_kphp_runtime_opt(std::string opt) { kphp_runtime_opts.emplace_back(std::move(opt)); }
  const std::vector<std::string> &get_kphp_runtime_opts() const { return kphp_runtime_opts; }

  void set_untyped_rpc_tl_used() {
    is_untyped_rpc_tl_used = true;
  }

  bool get_untyped_rpc_tl_used() const {
    return is_untyped_rpc_tl_used;
  }

  Stats stats;
};

extern CompilerCore *G;

/*** Misc functions ***/
bool try_optimize_var(VarPtr var);
string conv_to_func_ptr_name(VertexPtr call);
VertexPtr conv_to_func_ptr(VertexPtr call);

