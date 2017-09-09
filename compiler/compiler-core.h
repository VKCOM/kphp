#pragma once
#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "compiler/bicycle.h"
#include "compiler/compiler.h"
#include "compiler/data.h"
#include "compiler/data_ptr.h"
#include "compiler/function-pass.h"
#include "compiler/index.h"
#include "compiler/io.h"
#include "compiler/name-gen.h"
#include "compiler/stage.h"

/*** Core ***/
//Consists mostly of functions that require synchronization
typedef enum {fs_function, fs_member_function} function_set_t;
class CompilerCore {
  private:
    Index cpp_index;
    HT <SrcFilePtr> file_ht;
    HT <FunctionSetPtr> function_set_ht;
    HT <DefinePtr> defines_ht;
    HT <VarPtr> global_vars_ht;
    vector <SrcFilePtr> main_files;
    KphpEnviroment *env_;
    HT <ClassPtr> classes_ht;

    bool add_to_function_set (FunctionSetPtr function_set, FunctionPtr function,
                              bool req = false);
    FunctionPtr create_function (const FunctionInfo &info);
    ClassPtr create_class (const ClassInfo &info);

    inline bool try_require_file (SrcFilePtr file) {
      return __sync_bool_compare_and_swap (&file->is_required, false, true);
    }
  public:
    string cpp_dir;

    CompilerCore();
    void start();
    void make();
    void finish();
    void register_env (KphpEnviroment *env);
    const KphpEnviroment &env();
    string unify_file_name (const string &file_name);
    SrcFilePtr register_file (const string &file_name, const string &context);

    template <class DataStream>
    void register_main_file (const string &file_name, DataStream &os);
    template <class DataStream>
    pair <SrcFilePtr, bool> require_file (const string &file_name, const string &context, DataStream &os);

    template <class DataStream>
    void require_function_set (FunctionSetPtr function_set, FunctionPtr by_function,
                               DataStream &os);

    template <class DataStream>
    void require_function_set (function_set_t type, const string &name,
                               FunctionPtr by_function, DataStream &os);
    template <class DataStream>
    void register_function_header (VertexAdaptor <meta_op_function> function_header, DataStream &os);
    template <class DataStream>
    FunctionPtr register_function (const FunctionInfo &info, DataStream &os);
    template <class DataStream>
    ClassPtr register_class(const ClassInfo &info, DataStream &os);
    FunctionSetPtr get_function_set (function_set_t type, const string &name, bool force);
    FunctionPtr get_function_unsafe (const string &name);
    ClassPtr get_class(const string &name);

    bool register_define (DefinePtr def_id);
    DefinePtr get_define (const string &name);

    VarPtr create_var (const string &name, VarData::Type type);
    VarPtr get_global_var (const string &name, VarData::Type type, VertexPtr init_val);
    VarPtr create_local_var (FunctionPtr function, const string &name, VarData::Type type);

    const vector <SrcFilePtr> &get_main_files();
    vector <VarPtr> get_global_vars();
    vector <ClassPtr> get_classes();

    void load_index();
    void save_index();
    File *get_file_info (const string &file_name);
    void del_extra_files();
    void init_dest_dir();
};

extern CompilerCore *G;

/*** Misc functions ***/
bool try_optimize_var (VarPtr var);
VertexPtr conv_to_func_ptr (VertexPtr call);
VertexPtr set_func_id (VertexPtr call, FunctionPtr func);
VertexPtr try_set_func_id (VertexPtr call);

#include "compiler-core.hpp"
