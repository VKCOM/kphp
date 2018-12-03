#pragma once

#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <functional>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <forward_list>

#include "compiler/compiler.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/function-info.h"
#include "compiler/data/lib-data.h"
#include "compiler/data/var-data.h"
#include "compiler/function-pass.h"
#include "compiler/index.h"
#include "compiler/io.h"
#include "compiler/name-gen.h"
#include "compiler/stage.h"
#include "compiler/threading/data-stream.h"
#include "compiler/threading/hash-table.h"

/*** Core ***/
//Consists mostly of functions that require synchronization

class CompilerCore {
private:
  Index cpp_index;
  TSHashTable<SrcFilePtr> file_ht;
  TSHashTable<FunctionPtr> functions_ht;
  TSHashTable<DefinePtr> defines_ht;
  TSHashTable<VarPtr> global_vars_ht;
  TSHashTable<LibPtr> libs_ht;
  vector<SrcFilePtr> main_files;
  KphpEnviroment *env_;
  TSHashTable<ClassPtr> classes_ht;
  // это костыль, который должен уйти, когда мы перепишем часть php-кода
  TSHashTable<VertexPtr> extern_func_headers_ht;

  void create_builtin_classes();

  inline bool try_require_file(SrcFilePtr file);

public:
  string cpp_dir;

  CompilerCore();
  void start();
  void make();
  void finish();
  void register_env(KphpEnviroment *env);
  const KphpEnviroment &env() const;
  const string &get_global_namespace() const;
  string unify_file_name(const string &file_name);
  SrcFilePtr register_file(const string &file_name, ClassPtr context_class, LibPtr owner_lib);


  void register_main_file(const string &file_name, DataStream<SrcFilePtr> &os);
  pair<SrcFilePtr, bool> require_file(const string &file_name, ClassPtr context_class, LibPtr owner_lib, DataStream<SrcFilePtr> &os);

  void require_function(const string &name, DataStream<FunctionPtr> &os);

  template <class CallbackT>
  void operate_on_function_locking(const string &name, CallbackT callback) {
    static_assert(std::is_constructible<std::function<void(FunctionPtr&)>, CallbackT>::value, "invalid callback signature");

    TSHashTable<FunctionPtr>::HTNode *node = functions_ht.at(hash_ll(name));
    AutoLocker<Lockable *> locker(node);
    callback(node->data);
  }

  FunctionPtr register_function(const FunctionInfo &info, DataStream<FunctionPtr> &os);
  ClassPtr register_class(ClassPtr cur_class);
  LibPtr register_lib(LibPtr lib);

  FunctionPtr get_function(const string &name);
  ClassPtr get_class(const string &name);

  VertexPtr get_extern_func_header(const string &name);
  void save_extern_func_header(const string &name, VertexPtr header);

  bool register_define(DefinePtr def_id);
  DefinePtr get_define(const string &name);

  VarPtr create_var(const string &name, VarData::Type type);
  VarPtr get_global_var(const string &name, VarData::Type type, VertexPtr init_val);
  VarPtr create_local_var(FunctionPtr function, const string &name, VarData::Type type);

  const vector<SrcFilePtr> &get_main_files();
  vector<VarPtr> get_global_vars();
  vector<ClassPtr> get_classes();
  vector<DefinePtr> get_defines();
  vector<LibPtr> get_libs();

  void load_index();
  void save_index();
  File *get_file_info(const string &file_name);
  void del_extra_files();
  void init_dest_dir();
  std::string get_subdir_name() const;

private:
  void copy_static_lib_to_out_dir(const File &static_archive, bool show_copy_cmd) const;
  std::forward_list<File> collect_external_libs();
};

extern CompilerCore *G;

/*** Misc functions ***/
bool try_optimize_var(VarPtr var);
string conv_to_func_ptr_name(VertexPtr call);
VertexPtr conv_to_func_ptr(VertexPtr call, FunctionPtr current_function);

