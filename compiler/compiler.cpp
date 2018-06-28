#include "compiler/compiler.h"

#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/crc32.h"

#include "compiler/analyzer.h"
#include "compiler/bicycle.h"
#include "compiler/cfg.h"
#include "compiler/code-gen.h"
#include "compiler/compiler-core.h"
#include "compiler/data_ptr.h"
#include "compiler/function-pass.h"
#include "compiler/gentree.h"
#include "compiler/io.h"
#include "compiler/lexer.h"
#include "compiler/name-gen.h"
#include "compiler/pass-optimize.hpp"
#include "compiler/pass-register-vars.hpp"
#include "compiler/pass-rl.h"
#include "compiler/pass-split-switch.hpp"
#include "compiler/pass-ub.h"
#include "compiler/stage.h"
#include "compiler/type-inferer.h"
#include "compiler/const-manipulations.h"
#include "compiler/phpdoc.h"
#include "common/version-string.h"

template <class T>
class SyncPipeF {
  public:
    DataStreamRaw <T> tmp_stream;
    SyncPipeF() {
      tmp_stream.set_sink (true);
    }
    template <class OutputStreamT>
    void execute (T input, OutputStreamT &os __attribute__((unused))) {
      tmp_stream << input;
    }
    template <class OutputStreamT>
    void on_finish (OutputStreamT &os) {
      mem_info_t mem_info;
      get_mem_stats (getpid(), &mem_info);

      stage::die_if_global_errors();
      while (!tmp_stream.empty()) {
        os << tmp_stream.get();
      }
    }
};

/*** Load file ***/
class LoadFileF {
  public:
    DUMMY_ON_FINISH
    template <class OutputStream> void execute (SrcFilePtr file, OutputStream &os) {
      AUTO_PROF (load_files);
      stage::set_name ("Load file");
      stage::set_file (file);

      kphp_assert (!file->loaded);
      file->load();

      if (stage::has_error()) {
        return;
      }

      os << file;
    }
};

/*** Split file into tokens ***/
struct FileAndTokens {
  SrcFilePtr file;
  vector <Token *> *tokens;
  FileAndTokens() :
    file(),
    tokens (NULL) {
  }
};
class FileToTokensF {
  public:
    DUMMY_ON_FINISH
    template <class OutputStream> void execute (SrcFilePtr file, OutputStream &os) {
      AUTO_PROF (lexer);
      stage::set_name ("Split file to tokens");
      stage::set_file (file);
      kphp_assert (file.not_null());

      kphp_assert (file->loaded);
      FileAndTokens res;
      res.file = file;
      res.tokens = new vector <Token *>();
      php_text_to_tokens (
        &file->text[0], (int)file->text.length(),
        file->main_func_name, res.tokens
      );

      if (stage::has_error()) {
        return;
      }

      os << res;
    }
};

/*** Parse tokens into syntax tree ***/
template <class DataStream>
class GenTreeCallback : public GenTreeCallbackBase {
    DataStream &os;
  public:
    GenTreeCallback (DataStream &os) :
      os (os) {
    }
    FunctionPtr register_function (const FunctionInfo &info) {
      return G->register_function (info, os);
    }

    void require_function_set (FunctionPtr function) {
      G->require_function_set(fs_function, function->name, FunctionPtr(), os);
    }

    ClassPtr register_class (const ClassInfo &info) {
      return G->register_class(info, os);
    }

    ClassPtr get_class_by_name (string const &class_name) {
      return G->get_class(class_name);
    }
};

class ParseF {
  public:
    DUMMY_ON_FINISH
    template <class OutputStream> void execute (FileAndTokens file_and_tokens, OutputStream &os) {
      AUTO_PROF (gentree);
      stage::set_name ("Parse file");
      stage::set_file (file_and_tokens.file);
      kphp_assert (file_and_tokens.file.not_null());

      GenTreeCallback <OutputStream> callback (os);
      php_gen_tree (file_and_tokens.tokens, file_and_tokens.file->class_context, file_and_tokens.file->main_func_name, &callback);
    }
};

/*** Split main functions by #break_file ***/
bool split_by_break_file (VertexAdaptor <op_function> root, vector <VertexPtr> *res) {
  bool need_split = false;

  VertexAdaptor <op_seq> seq = root->cmd();
  for (VertexRange i = seq->args(); !i.empty(); i.next()) {
    if ((*i)->type() == op_break_file) {
      need_split = true;
      break;
    }
  }

  if (!need_split) {
    return false;
  }

  vector <VertexPtr> splitted;
  {
    vector <VertexPtr> cur_next;
    VertexRange i = seq->args();
    while (true) {
      if (i.empty() || (*i)->type() == op_break_file) {
        CREATE_VERTEX (new_seq, op_seq, cur_next);
        splitted.push_back (new_seq);
        cur_next.clear();
      } else {
        cur_next.push_back (*i);
      }
      if (i.empty()) {
        break;
      }
      i.next();
    }
  }

  int splitted_n = (int)splitted.size();
  VertexAdaptor <op_function> next_func;
  for (int i = splitted_n - 1; i >= 0; i--) {
    string func_name_str = gen_unique_name (stage::get_file()->short_file_name);
    CREATE_VERTEX (func_name, op_func_name);
    func_name->str_val = func_name_str;
    CREATE_VERTEX (func_params, op_func_param_list);
    CREATE_VERTEX (func, op_function, func_name, func_params, splitted[i]);
    func->extra_type = op_ex_func_global;

    if (next_func.not_null()) {
      CREATE_VERTEX (call, op_func_call);
      call->str_val = next_func->name()->get_string();
      GenTree::func_force_return (func, call);
    } else {
      GenTree::func_force_return (func);
    }
    next_func = func;

    res->push_back (func);
  }

  CREATE_VERTEX (func_call , op_func_call);
  func_call->str_val = next_func->name()->get_string();

  CREATE_VERTEX (ret, op_return, func_call);
  CREATE_VERTEX (new_seq, op_seq, ret);
  root->cmd() = new_seq;

  return true;
}

class ApplyBreakFileF {
  public:
    DUMMY_ON_FINISH
    template <class OutputStream> void execute (FunctionPtr function, OutputStream &os) {
      AUTO_PROF (apply_break_file);

      stage::set_name ("Apply #break_file");
      stage::set_function (function);
      kphp_assert (function.not_null());

      VertexPtr root = function->root;

      if (function->type() != FunctionData::func_global || root->type() != op_function) {
        os << function;
        return;
      }

      vector <VertexPtr> splitted;
      split_by_break_file (root, &splitted);

      if (stage::has_error()) {
        return;
      }

      for (int i = 0; i < (int)splitted.size(); i++) {
        G->register_function(FunctionInfo(
            splitted[i], function->namespace_name,
            function->class_name, function->class_context_name,
            function->namespace_uses, function->class_extends, set <string>(), false, false, access_nonmember
        ), os);
      }

      os << function;
    }
};

/*** Replace cases in big global functions with functions call ***/
class SplitSwitchF {
  public:
    DUMMY_ON_FINISH
    template <class OutputStream> void execute (FunctionPtr function, OutputStream &os) {
      SplitSwitchPass split_switch;
      run_function_pass (function, &split_switch);

      const vector <VertexPtr> &new_functions = split_switch.get_new_functions();
      for (int i = 0; i < (int)new_functions.size(); i++) {
        G->register_function (FunctionInfo(new_functions[i], function->namespace_name,
                                           function->class_name, function->class_context_name, function->namespace_uses,
                                           function->class_extends, set<string>(), false, false, access_nonmember), os);
      }

      if (stage::has_error()) {
        return;
      }

      os << function;
    }
};


/*** Collect files and functions used in function ***/
struct ReadyFunctionPtr {
  FunctionPtr function;
  ReadyFunctionPtr(){}
  ReadyFunctionPtr (FunctionPtr function) :
    function (function) {
  }
  operator FunctionPtr() const {
    return function;
  }
};

class CollectRequiredCallbackBase {
  public:
    virtual pair <SrcFilePtr, bool> require_file (const string &file_name, const string &class_context) = 0;
    virtual void require_function_set (
        function_set_t type,
        const string &name,
        FunctionPtr by_function) = 0;
    virtual ~CollectRequiredCallbackBase() {
    }
};

class CollectRequiredPass : public FunctionPassBase {
  private:
    AUTO_PROF (collect_required);
    bool force_func_ptr;
    CollectRequiredCallbackBase *callback;
  public:
    CollectRequiredPass (CollectRequiredCallbackBase *callback) :
      force_func_ptr (false),
      callback (callback) {
    }
    struct LocalT : public FunctionPassBase::LocalT {
      bool saved_force_func_ptr;
    };
    string get_description() {
      return "Collect required";
    }

    void require_class(string const &class_name, string const &context_name) {
      pair<SrcFilePtr, bool> res = callback->require_file(class_name + ".php", context_name);
      kphp_error(res.first.not_null(), dl_pstr("Class %s not found", class_name.c_str()));
      if (res.second) {
        res.first->req_id = current_function;
      }
    }

    template <class VisitT>
    bool user_recursion (VertexPtr v, LocalT *local __attribute__((unused)), VisitT &visit __attribute__((unused))) {
      if (v->type() == op_function && v.as<op_function>()->name().as<op_func_name>()->get_string() == current_function->name) {
        if (current_function->type() == FunctionData::func_global && !current_function->class_name.empty()) {
          if (!current_function->class_extends.empty()) {
            require_class(resolve_uses(current_function, current_function->class_extends, '/'), current_function->class_context_name);
          }
          if ((current_function->namespace_name + "\\" + current_function->class_name) != current_function->class_context_name) {
            return true;
          }
          if (!current_function->class_extends.empty()) {
            require_class(resolve_uses(current_function, current_function->class_extends, '/'), "");
          }
        }
      }
      return false;
    }

    string get_class_name_for(string const &name, char delim = '$') {
      size_t pos$$ = name.find("::");
      if (pos$$ == string::npos) {
        return "";
      }

      string class_name = name.substr(0, pos$$);
      kphp_assert(!class_name.empty());
      return resolve_uses(current_function, class_name, delim);
    }


    VertexPtr on_enter_vertex (VertexPtr root, LocalT *local) {
      bool new_force_func_ptr = false;
      if (root->type() == op_func_call || root->type() == op_func_name) {
        string name = get_full_static_member_name(current_function, root->get_string(), root->type() == op_func_call);
        callback->require_function_set (fs_function, name, current_function);
      }

      if (root->type() == op_func_call || root->type() == op_var || root->type() == op_func_name) {
        const string &name = root->get_string();
        const string &class_name = get_class_name_for(name, '/');
        if (!class_name.empty()) {
          require_class(class_name, "");
          string member_name = get_full_static_member_name(current_function, name, root->type() == op_func_call);
          root->set_string(member_name);
        }
      }
      if (root->type() == op_constructor_call) {
        if (likely(!root->type_help)) {     // type_help <=> Memcache | Exception
          const string &class_name = resolve_uses(current_function, root->get_string(), '/');
          require_class(class_name, "");
        } else {
          callback->require_function_set(fs_function, resolve_constructor_fname(current_function, root), current_function);
        }
      }

      if (root->type() == op_func_call) {
        new_force_func_ptr = true;
        const string &name = root->get_string();
        if (name == "func_get_args" || name == "func_get_arg" || name == "func_num_args") {
          current_function->varg_flag = true;
        }
      }

      local->saved_force_func_ptr = force_func_ptr;
      force_func_ptr = new_force_func_ptr;

      return root;
    }

    VertexPtr on_exit_vertex (VertexPtr root, LocalT *local) {
      force_func_ptr = local->saved_force_func_ptr;

      if (root->type() == op_require || root->type() == op_require_once) {
        VertexAdaptor <meta_op_require> require = root;
        for (VertexRange i = require->args(); !i.empty(); i.next()) {
          kphp_error_act ((*i)->type() == op_string, "Not a string in 'require' arguments", continue);
          VertexAdaptor <op_string> cur = *i;
          pair <SrcFilePtr, bool> tmp = callback->require_file (cur->str_val, "");
          SrcFilePtr file = tmp.first;
          bool required = tmp.second;
          if (required) {
            file->req_id = current_function;
          }

          CREATE_VERTEX (call, op_func_call);
          if (file.not_null()) {
            call->str_val = file->main_func_name;
            *i = call;
          } else {
            kphp_error (0, dl_pstr ("Cannot require [%s]\n", cur->str_val.c_str()));
          }
        }
      }

      return root;
    }
};

template <class DataStream>
class CollectRequiredCallback : public CollectRequiredCallbackBase {
  private:
    DataStream *os;
  public:
    CollectRequiredCallback (DataStream *os) :
      os (os) {
    }
    pair <SrcFilePtr, bool> require_file (const string &file_name, const string &class_context) {
      return G->require_file (file_name, class_context, *os);
    }
    void require_function_set (
        function_set_t type,
        const string &name,
        FunctionPtr by_function) {
      G->require_function_set (type, name, by_function, *os);
    }
};

class CollectRequiredF {
  public:
    DUMMY_ON_FINISH
    template <class OutputStream> void execute (FunctionPtr function, OutputStream &os) {
      CollectRequiredCallback <OutputStream> callback (&os);
      CollectRequiredPass pass (&callback);

      run_function_pass (function, &pass);

      if (stage::has_error()) {
        return;
      }

      if (function->type() == FunctionData::func_global && !function->class_name.empty() &&
        (function->namespace_name + "\\" + function->class_name) != function->class_context_name) {
        return;
      }
      os << ReadyFunctionPtr (function);
    }
};

/*** Create local variables for switches ***/
class CreateSwitchForeachVarsF : public FunctionPassBase {
  private:
    AUTO_PROF (create_switch_vars);
    VertexPtr process_switch (VertexPtr v){

      VertexAdaptor <op_switch> switch_v = v;

      CREATE_VERTEX (root_ss, op_var); 
      root_ss->str_val = gen_unique_name("ss");
      root_ss->extra_type = op_ex_var_superlocal;
      root_ss->type_help = tp_string; 
      switch_v->ss() = root_ss;

      CREATE_VERTEX (root_ss_hash, op_var); 
      root_ss_hash->str_val = gen_unique_name("ss_hash");
      root_ss_hash->extra_type = op_ex_var_superlocal;
      root_ss_hash->type_help = tp_int; 
      switch_v->ss_hash() = root_ss_hash;

      CREATE_VERTEX (root_switch_flag, op_var); 
      root_switch_flag->str_val = gen_unique_name("switch_flag");
      root_switch_flag->extra_type = op_ex_var_superlocal;
      root_switch_flag->type_help = tp_bool; 
      switch_v->switch_flag() = root_switch_flag;

      CREATE_VERTEX (root_switch_var, op_var); 
      root_switch_var->str_val = gen_unique_name("switch_var");
      root_switch_var->extra_type = op_ex_var_superlocal;
      root_switch_var->type_help = tp_var; 
      switch_v->switch_var() = root_switch_var;
      return switch_v;
    }

    VertexPtr process_foreach (VertexPtr v){

      VertexAdaptor <op_foreach> foreach_v = v;
      VertexAdaptor <op_foreach_param> foreach_param = foreach_v->params(); 
      VertexAdaptor <op_var> x = foreach_param->x();
      //VertexPtr xs = foreach_param->xs();

      if (!x->ref_flag) {
        /*CREATE_VERTEX(temp_var, op_var);
        temp_var->str_val = gen_unique_name ("tmp_expr");
        temp_var->extra_type = op_ex_var_superlocal;
        temp_var->needs_const_iterator_flag = true;
        VertexPtr xs_clone = clone_vertex(xs);
        CREATE_VERTEX(set_op, op_set, temp_var, xs_clone);


        CREATE_VERTEX(seq_op, op_seq, set_op);
        foreach_v->pre_cmd() = seq_op;*/

        CREATE_VERTEX(temp_var2, op_var);
        temp_var2->str_val = gen_unique_name ("tmp_expr");
        temp_var2->extra_type = op_ex_var_superlocal;
        temp_var2->needs_const_iterator_flag = true;
        foreach_param->temp_var() = temp_var2;

        foreach_v->params() = foreach_param;
      }
      return foreach_v;
    }


  public:
    string get_description() {
      return "create switch and foreach vars";
    }
    VertexPtr on_enter_vertex (VertexPtr v, LocalT *local __attribute__((unused))) {
      if (v->type() == op_switch) {
        return process_switch(v);
      }
      if (v->type() == op_foreach) {
        return process_foreach(v);
      }

      return v;
    }
};

/*** Calculate proper location field for each node ***/
class CalcLocationsPass : public FunctionPassBase {
  private:
    AUTO_PROF (calc_locations);
  public:
    string get_description() {
      return "Calc locations";
    }
    VertexPtr on_enter_vertex (VertexPtr v, LocalT *local __attribute__((unused))) {
      stage::set_line (v->location.line);
      v->location = stage::get_location();

      return v;
    }
};

class CollectDefinesToVectorPass : public FunctionPassBase {
  private:
    vector<VertexPtr> defines;
  public:
    string get_description() {
      return "Process defines concatenation";
    }
    VertexPtr on_exit_vertex (VertexPtr root, LocalT *local __attribute__((unused))) {
      if (root->type() == op_define) {
        defines.push_back(root);
      }
      return root;
    }
    const vector<VertexPtr>& get_vector() {
      return defines;
    }
};

class PreprocessDefinesConcatenationF {
  private:
    set<string> in_progress;
    set<string> done;
    map<string, VertexPtr> define_vertex;
    vector<string> stack;

    DataStreamRaw<VertexPtr> defines_stream;
    DataStreamRaw<FunctionPtr> all_fun;

    CheckConstWithDefines check_const;
    MakeConst make_const;

  public:

    PreprocessDefinesConcatenationF()
      : check_const(define_vertex)
      , make_const(define_vertex)
    {
      defines_stream.set_sink(true);
      all_fun.set_sink(true);

      CREATE_VERTEX(val, op_string);
      val->set_string(get_version_string());
      DefineData *data = new DefineData(val, DefineData::def_php);
      data->name = "KPHP_COMPILER_VERSION";
      DefinePtr def_id(data);

      G->register_define(def_id);
    }

    string get_description() {
      return "Process defines concatenation";
    }

    template <class OutputStreamT>
    void execute (FunctionPtr function, OutputStreamT &os __attribute__((unused))) {
      AUTO_PROF (preprocess_defines);
      CollectDefinesToVectorPass pass;
      run_function_pass (function, &pass);

      vector<VertexPtr> vs = pass.get_vector();
      for (size_t i = 0; i < vs.size(); i++) {
        defines_stream << vs[i];
      }
      all_fun << function;
    }

    template <class OutputStreamT>
    void on_finish (OutputStreamT &os) {
      AUTO_PROF (preprocess_defines_finish);
      stage::set_name ("Preprocess defines");
      stage::set_file (SrcFilePtr());

      stage::die_if_global_errors();

      vector<VertexPtr> all_defines = defines_stream.get_as_vector();
      FOREACH(all_defines, define) {
        VertexPtr name_v = define->as<op_define>()->name();
        stage::set_location(define->as<op_define>()->location);
        kphp_error_return (
          name_v->type() == op_string,
          "Define: first parameter must be a string"
        );

        string name = name_v.as<op_string>()->str_val;
        if (!define_vertex.insert(make_pair(name, *define)).second) {
          kphp_error_return(0, dl_pstr("Duplicate define declaration: %s", name.c_str()));
        }
      }

      FOREACH(all_defines, define) {
        process_define(*define);
      }

      vector<FunctionPtr> funs = all_fun.get_as_vector();
      for (size_t i = 0; i < funs.size(); i++) {
        os << funs[i];
      }
    }

    void process_define_recursive(VertexPtr root) {
      if (root->type() == op_func_name) {
        string name = root.as<op_func_name>()->str_val;
        name = resolve_define_name(name);
        map<string, VertexPtr>::iterator it = define_vertex.find(name);
        if (it != define_vertex.end()) {
          process_define(it->second);
        }
      }
      FOREACH(root, i) {
        process_define_recursive(*i);
      }
    }

    void process_define(VertexPtr root) {
      stage::set_location(root->location);
      VertexAdaptor <meta_op_define> define = root;
      VertexPtr name_v = define->name();
      VertexPtr val = define->value();

      kphp_error_return (
        name_v->type() == op_string,
        "Define: first parameter must be a string"
      );

      string name = name_v.as<op_string>()->str_val;

      if (done.find(name) != done.end()) {
        return;
      }

      if (in_progress.find(name) != in_progress.end()) {
        stringstream stream;
        int id = -1;
        for (size_t i = 0; i < stack.size(); i++) {
          if (stack[i] == name) {
            id = i;
            break;
          }
        }
        kphp_assert(id != -1);
        for (size_t i = id; i < stack.size(); i++) {
          stream << stack[i] << " -> ";
        }
        stream << name;
        kphp_error_return(0, dl_pstr("Recursive define dependency:\n%s\n", stream.str().c_str()));
      }

      in_progress.insert(name);
      stack.push_back(name);

      process_define_recursive(val);
      stage::set_location(root->location);

      in_progress.erase(name);
      stack.pop_back();
      done.insert(name);

      if (check_const.is_const(val)) {
        define->value() = make_const.make_const(val);
        kphp_assert(define->value().not_null());
      }
    }
};


/*** Collect defines declarations ***/
class CollectDefinesPass : public FunctionPassBase {
  private:
    AUTO_PROF (collect_defines);
  public:
    string get_description() {
      return "Collect defines";
    }

    VertexPtr on_exit_vertex (VertexPtr root, LocalT *local __attribute__((unused))) {
      if (root->type() == op_define || root->type() == op_define_raw) {
        VertexAdaptor <meta_op_define> define = root;
        VertexPtr name = define->name();
        VertexPtr val = define->value();

        kphp_error_act (
          name->type() == op_string,
          "Define: first parameter must be a string",
          return root
        );

        DefineData::DefineType def_type;
        if (!CheckConst::is_const(val)) {
          kphp_error(name->get_string().length() <= 1 || name->get_string().substr(0, 2) != "c#",
                     dl_pstr("Couldn't calculate value of %s", name->get_string().substr(2).c_str()));
          def_type = DefineData::def_var;

          CREATE_VERTEX (var, op_var);
          var->extra_type = op_ex_var_superglobal;
          var->str_val = name->get_string();
          set_location (var, name->get_location());
          name = var;

          define->value() = VertexPtr();
          CREATE_VERTEX (new_root, op_set, name, val);
          set_location (new_root, root->get_location());
          root = new_root;
        } else {
          def_type = DefineData::def_php;
          CREATE_VERTEX (new_root, op_empty);
          root = new_root;
        }

        DefineData *data = new DefineData (val, def_type);
        data->name = name->get_string();
        data->file_id = stage::get_file();
        DefinePtr def_id (data);

        if (def_type == DefineData::def_var) {
          name->set_string ("d$" + name->get_string());
        } else {
          current_function->define_ids.push_back (def_id);
        }

        G->register_define (def_id);
      }

      return root;
    }
};

class CheckReturnsPass : public FunctionPassBase {
  private:
    bool have_void;
    bool have_not_void;
    bool errored;
    AUTO_PROF (check_returns);
  public:
    string get_description() {
      return "Check returns";
    }
    void init() {
      have_void = have_not_void = errored = false;
    }
    VertexPtr on_exit_vertex (VertexPtr root, LocalT *local __attribute__((unused))) {

      if (root->type() == op_return) {
        if (root->void_flag) {
          have_void = true;
        } else {
          have_not_void = true;
        }
        if (have_void && have_not_void && !errored) {
          errored = true;
          FunctionPtr fun = stage::get_function();
          if (fun->type() != FunctionData::func_switch && fun->name != fun->file_id->main_func_name) {
            kphp_typed_warning("return", "Mixing void and not void returns in one function");
          }
        }
      }

      return root;
    }

    void on_finish() {
      if (!have_not_void && !stage::get_function()->is_extern) {
        stage::get_function()->root->void_flag = true;
      }
    }
};


/*** Apply function header ***/
void function_apply_header (FunctionPtr func, VertexAdaptor <meta_op_function> header) {
  VertexAdaptor <meta_op_function> root = func->root;
  func->used_in_source = true;

  kphp_assert (root.not_null() && header.not_null());
  kphp_error_return (
    func->header.is_null(),
    dl_pstr ("Function [%s]: multiple headers", func->name.c_str())
  );
  func->header = header;

  kphp_error_return (
    root->type_rule.is_null(),
    dl_pstr ("Function [%s]: type_rule is overided by header", func->name.c_str())
  );
  root->type_rule = header->type_rule;

  kphp_error_return (
    !(!header->varg_flag && func->varg_flag),
    dl_pstr ("Function [%s]: varg_flag mismatch with header", func->name.c_str())
  );
  func->varg_flag = header->varg_flag;

  if (!func->varg_flag) {
    VertexAdaptor <op_func_param_list> root_params_vertex = root->params();
    VertexAdaptor <op_func_param_list> header_params_vertex = header->params();
    VertexRange root_params = root_params_vertex->params();
    VertexRange header_params = header_params_vertex->params();

    kphp_error (
      root_params.size() == header_params.size(),
      dl_pstr ("Bad header for function [%s]", func->name.c_str())
    );
    int params_n = (int)root_params.size();
    for (int i = 0; i < params_n; i++) {
      kphp_error (
        root_params[i]->size() == header_params[i]->size(),
        dl_pstr (
          "Function [%s]: %dth param has problem with default value",
          func->name.c_str(), i + 1
        )
      );
      kphp_error (
        root_params[i]->type_help == tp_Unknown,
        dl_pstr ("Function [%s]: type_help is overrided by header", func->name.c_str())
      );
      root_params[i]->type_help = header_params[i]->type_help;
    }
  }
}

void prepare_function_misc (FunctionPtr func) {
  VertexAdaptor <meta_op_function> func_root = func->root;
  kphp_assert (func_root.not_null());
  VertexAdaptor <op_func_param_list> param_list = func_root->params();
  VertexRange params = param_list->args();
  int param_n = (int)params.size();
  bool was_default = false;
  func->min_argn = param_n;
  for (int i = 0; i < param_n; i++) {
    if (func->varg_flag) {
      kphp_error (params[i].as <meta_op_func_param>()->var()->ref_flag == false,
          "Reference arguments are not supported in varg functions");
    }
    if (params[i].as <meta_op_func_param>()->has_default()) {
      if (!was_default) {
        was_default = true;
        func->min_argn = i;
      }
      if (func->type() == FunctionData::func_local) {
        kphp_error (params[i].as <meta_op_func_param>()->var()->ref_flag == false,
        dl_pstr ("Default value in reference function argument [function = %s]", func->name.c_str()));
      }
    } else {
      kphp_error (!was_default,
          dl_pstr ("Default value expected [function = %s] [param_i = %d]",
            func->name.c_str(), i));
    }
  }
}

void prepare_function (FunctionPtr function) {
  prepare_function_misc (function);

  FunctionSetPtr function_set = function->function_set;
  VertexPtr header = function_set->header;
  if (header.not_null()) {
    function_apply_header (function, header);
  }
  if (function->root.not_null() && function->root->varg_flag) {
    function->varg_flag = true;
  }
}

class PrepareFunctionF  {
  public:
    DUMMY_ON_FINISH
    template <class OutputStream> void execute (FunctionPtr function, OutputStream &os) {
      stage::set_name ("Prepare function");
      stage::set_function (function);
      kphp_assert (function.not_null());

      prepare_function (function);

      if (stage::has_error()) {
        return;
      }

      os << function;
    }
};

/*** Replace defined and defines with values ***/
class RegisterDefinesPass : public FunctionPassBase {
  private:
    AUTO_PROF (register_defines);
  public:
    string get_description() {
      return "Register defines pass";
    }

    bool on_start (FunctionPtr function) {
      if (!FunctionPassBase::on_start(function)) {
        return false;
      }
      return true;
    }

    VertexPtr on_enter_vertex (VertexPtr root, LocalT *local __attribute__((unused))) {
      if (root->type() == op_defined) {
        bool is_defined = false;

        VertexAdaptor <op_defined> defined = root;

        kphp_error_act (
          (int)root->size() == 1 && defined->expr()->type() == op_string,
          "wrong arguments in 'defined'",
          return VertexPtr()
        );

        const string name = defined->expr().as <op_string>()->str_val;
        DefinePtr def = G->get_define (name);
        is_defined = def.not_null() && def->name == name;

        if (is_defined) {
          CREATE_VERTEX (true_val, op_true);
          root = true_val;
        } else {
          CREATE_VERTEX (false_val, op_false);
          root = false_val;
        }
      }

      if (root->type() == op_func_name) {
        string name = root->get_string();
        name = resolve_define_name(name);
        DefinePtr d = G->get_define (name);
        if (d.not_null()) {
          assert (d->name == name);
          if (d->type() == DefineData::def_var) {
            CREATE_VERTEX (var, op_var);
            var->extra_type = op_ex_var_superglobal;
            var->str_val = "d$" + name;
            root = var;
          } else if (d->type() == DefineData::def_raw || d->type() == DefineData::def_php) {
            CREATE_VERTEX (def, op_define_val);
            def->set_define_id (d);
            root = def;
          } else {
            assert (0 && "unreachable branch");
          }
        }
      }
      return root;
    }
};

/*** Hack for problems with ===, null and type inference ***/
class PreprocessEq3Pass : public FunctionPassBase {
  private:
    AUTO_PROF (preprocess_eq3);
  public:
    string get_description() {
      return "Preprocess eq3";
    }
    VertexPtr on_exit_vertex (VertexPtr root, LocalT *local __attribute__((unused))) {
      if (root->type() == op_eq3 || root->type() == op_neq3) {
        VertexAdaptor <meta_op_binary_op> eq_op = root;
        VertexPtr a = eq_op->lhs();
        VertexPtr b = eq_op->rhs();

        if (b->type() == op_var || b->type() == op_index) {
          std::swap (a, b);
        }

        if (a->type() == op_var || a->type() == op_index) {
          VertexPtr ra = a;
          while (ra->type() == op_index) {
            ra = ra.as <op_index>()->array();
          }
          bool ok = ra->type() == op_var;
          if (ok) {
            ok &= //(b->type() != op_true && b->type() != op_false) ||
              (ra->get_string() != "connection" &&
               ra->get_string().find ("MC") == string::npos);
          }

          if (ok) {
            if (b->type() == op_null) {
              VertexPtr check_cmd;
              CREATE_VERTEX (isset, op_isset, a);
              if (root->type() == op_neq3) {
                check_cmd = isset;
              } else {
                CREATE_VERTEX (not_isset, op_log_not, isset);
                check_cmd = not_isset;
              }
              root = check_cmd;
            } else if (b->type() == op_false ||
                (b->type() == op_string && b->get_string() == "") ||
                (b->type() == op_int_const && b->get_string() == "0") ||
                (b->type() == op_float_const/* && b->str_val == "0.0"*/) ||
                (b->type() == op_array && b->empty())) {
              VertexPtr check_cmd;

              VertexPtr isset;
              VertexPtr a_copy = clone_vertex (a);
              if (b->type() == op_true || b->type() == op_false) {
                CREATE_VERTEX(is_bool, op_func_call, a_copy);
                is_bool->str_val = "is_bool";
                isset = is_bool;
              } else if (b->type() == op_string) {
                CREATE_VERTEX(is_string, op_func_call, a_copy);
                is_string->str_val = "is_string";
                isset = is_string;
              } else if (b->type() == op_int_const) {
                CREATE_VERTEX(is_integer, op_func_call, a_copy);
                is_integer->str_val = "is_integer";
                isset = is_integer;
              } else if (b->type() == op_float_const) {
                CREATE_VERTEX(is_float, op_func_call, a_copy);
                is_float->str_val = "is_float";
                isset = is_float;
              } else if (b->type() == op_array) {
                CREATE_VERTEX(is_array, op_func_call, a_copy);
                is_array->str_val = "is_array";
                isset = is_array;
              } else {
                kphp_fail();
              }


              if (root->type() == op_neq3) {
                CREATE_VERTEX (not_isset, op_log_not, isset);
                CREATE_VERTEX (check, op_log_or, not_isset, root);
                check_cmd = check;
              } else {
                CREATE_VERTEX (check, op_log_and, isset, root);
                check_cmd = check;
              }
              root = check_cmd;
            }
          }
        }
      }

      return root;
    }
};

/*** Replace __FUNCTION__ ***/
/*** Set function_id for all function calls ***/
class PreprocessFunctionCPass : public FunctionPassBase {
  private:
    AUTO_PROF (preprocess_function_c);
  public:
    string get_description() {
      return "Preprocess function C";
    }
    bool check_function (FunctionPtr function) {
      return default_check_function (function) && function->type() != FunctionData::func_extern;
    }

    VertexPtr on_enter_vertex (VertexPtr root, LocalT *local __attribute__((unused))) {
      if (root->type() == op_function_c) {
        CREATE_VERTEX (new_root, op_string);
        if (stage::get_function_name() != stage::get_file()->main_func_name) {
          new_root->set_string(stage::get_function_name());
        }
        set_location (new_root, root->get_location());
        root = new_root;
      }

      if (root->type() == op_func_call || root->type() == op_func_ptr || root->type() == op_constructor_call) {
        root = try_set_func_id(root, current_function);
      }

      return root;
    }
};

/*** Preprocess 'break 5' and similar nodes. The will be replaced with goto ***/
class PreprocessBreakPass : public FunctionPassBase {
  private:
    AUTO_PROF (preprocess_break);
    vector <VertexPtr> cycles;

    int current_label_id;
    int get_label_id (VertexAdaptor <meta_op_cycle> cycle, Operation op) {
      int *val = NULL;
      if (op == op_break) {
        val = &cycle->break_label_id;
      } else if (op == op_continue) {
        val = &cycle->continue_label_id;
      } else {
        assert (0);
      }
      if (*val == 0) {
        *val = ++current_label_id;
      }
      return *val;
    }

  public:
    struct LocalT : public FunctionPassBase::LocalT {
      bool is_cycle;
    };

    PreprocessBreakPass() :
      current_label_id (0) {
      //cycles.reserve (1024);
    }

    VertexPtr on_enter_vertex (VertexPtr root, LocalT *local) {
      local->is_cycle = OpInfo::type (root->type()) == cycle_op;
      if (local->is_cycle) {
        cycles.push_back (root);
      }

      if (root->type() == op_break || root->type() == op_continue) {
        int val;
        VertexAdaptor <meta_op_goto> goto_op = root;
        kphp_error_act (
          goto_op->expr()->type() == op_int_const,
          "Break/continue parameter expected to be constant integer",
          return root
        );
        val = atoi (goto_op->expr()->get_string().c_str());
        kphp_error_act (
          1 <= val && val <= 10,
          "Break/continue parameter expected to be in [1;10] interval",
          return root
        );

        bool force_label = false;
        if (goto_op->type() == op_continue &&  val == 1 && !cycles.empty()
            && cycles.back()->type() == op_switch) {
          force_label = true;
        }


        int cycles_n = (int)cycles.size();
        kphp_error_act (
          val <= cycles_n,
          "Break/continue parameter is too big",
          return root
        );
        if (val != 1 || force_label) {
          goto_op->int_val = get_label_id (cycles[cycles_n - val], root->type());
        }
      }

      return root;
    }

    VertexPtr on_exit_vertex (VertexPtr root, LocalT *local) {
      if (local->is_cycle) {
        cycles.pop_back();
      }
      return root;
    }
};


class PreprocessVarargPass : public FunctionPassBase {
  private:
    AUTO_PROF (preprocess_break);

    VertexPtr create_va_list_var(Location loc) {
      CREATE_VERTEX(result, op_var);
      result->str_val = "$VA_LIST";
      set_location(result, loc);
      return result;
    }


  public:

    VertexPtr on_enter_vertex (VertexPtr root, LocalT *local __attribute__((unused))) {
      if (root->type() == op_func_call) {
        VertexPtr call = root.as<op_func_call>();
        string name = call->get_string();
        if (name == "func_get_args") {
          kphp_error(call->size() == 0, "Strange func_get_args with arguments");
          return create_va_list_var(call->location);
        } else if (name == "func_get_arg") {
          kphp_error(call->size() == 1, "Strange func_num_arg not one argument");
          VertexPtr arr = create_va_list_var(call->location);
          CREATE_VERTEX(index, op_index, arr, call->ith(0));
          return index;
        } else if (name == "func_num_args") {
          kphp_error(call->size() == 0, "Strange func_num_args with arguments");
          VertexPtr arr = create_va_list_var(call->location);
          CREATE_VERTEX(count_call, op_func_call, arr);
          count_call->str_val = "count";
          set_location(count_call, call->location);
          return count_call;
        }
        return root;
      }
      if (root->type() == op_function) {
        VertexPtr old_params = root.as<op_function>()->params();
        vector<VertexPtr> params_varg;
        VertexPtr va_list_var = create_va_list_var(root->location);
        CREATE_VERTEX(va_list_param, op_func_param, va_list_var);
        params_varg.push_back(va_list_param);
        CREATE_VERTEX (params_new, op_func_param_list, params_varg);

        root.as<op_function>()->params() = params_new;

        vector<VertexPtr> params_init;
        int ii = 0;
        FOREACH (old_params, i) {
          VertexAdaptor<op_func_param> arg = *i;
          kphp_error (!arg->ref_flag, "functions with reference arguments are not supported in vararg");
          VertexPtr var = arg->var();
          VertexPtr def;
          if (arg->has_default()) {
            def = arg->default_value();
          } else {
            CREATE_VERTEX(null, op_null);
            def = null;
          }

          CREATE_VERTEX(id0, op_int_const);
          id0->str_val = int_to_str(ii);
          CREATE_VERTEX(isset_value, op_index, create_va_list_var(root->location), id0);
          CREATE_VERTEX(isset, op_isset, isset_value);

          CREATE_VERTEX(id1, op_int_const);
          id1->str_val = int_to_str(ii);
          CREATE_VERTEX(result_value, op_index, create_va_list_var(root->location), id1);


          CREATE_VERTEX(expr, op_ternary, isset, result_value, def);
          CREATE_VERTEX(set, op_set, var, expr);
          params_init.push_back(set);
          ii++;
        }

        if (!params_init.empty()) {
          VertexPtr seq = root.as<op_function>()->cmd();
          kphp_assert(seq->type() == op_seq);
          FOREACH(seq, i) {
            params_init.push_back(*i);
          }
          CREATE_VERTEX(new_seq, op_seq, params_init);
          root.as<op_function>()->cmd() = new_seq;
        }
      }
      return root;
    }

    virtual bool check_function (FunctionPtr function) {
      return function->varg_flag && default_check_function (function);
    }
};


/**
 * Для обращений к свойствам $a->b определяем тип $a, убеждаемся в наличии свойства b у класса,
 * а также заполняем vertex<op_instance_prop>->var (понадобится для type inferring)
 */
class CheckInstanceProps : public FunctionPassBase {
private:
  AUTO_PROF (check_instance_props);
public:
  string get_description () {
    return "Check instance properties";
  }

  VertexPtr on_enter_vertex (VertexPtr v, LocalT *local __attribute__((unused))) {

    if (v->type() == op_instance_prop) {
      ClassPtr klass = resolve_expr_class(current_function, v);
      if (klass.not_null()) {   // если null, то ошибка доступа к непонятному свойству уже кинулась в resolve_expr_class()
        VarPtr var = klass->find_var(v->get_string());

        if (!kphp_error(var.not_null(),
            dl_pstr("Invalid property access ...->%s: does not exist in class %s", v->get_string().c_str(), klass->name.c_str()))) {
          v->set_var_id(var);
          init_class_instance_var(v, var, klass);
        }
      }
    }

    return v;
  }

  /*
   * Если при объявлении поля класса написано / ** @var int|false * / к примеру, делаем type_rule из phpdoc.
   * Это заставит type inferring принимать это во внимание, и если где-то выведется по-другому, будет ошибка.
   * С инстансами это тоже работает, т.е. / ** @var \AnotherClass * / будет тоже проверяться при выводе типов.
   */
  void init_class_instance_var (VertexPtr v, VarPtr var, ClassPtr klass) {
    kphp_assert(var->class_id.ptr == klass.ptr);

    // сейчас phpdoc у class var'а парсится каждый раз при ->обращении;
    // это уйдёт, когда вместо phpdoc_token будем хранить что-то более умное (что парсится 1 раз и хранит type_rule)
    if (var->phpdoc_token) {
      std::string var_name, type_str;
      if (PhpDocTypeRuleParser::find_tag_in_phpdoc(var->phpdoc_token->str_val, php_doc_tag::var, var_name, type_str)) {
        VertexPtr doc_type = phpdoc_parse_type(type_str, klass->init_function);
        if (!kphp_error(doc_type.not_null(),
            dl_pstr("Failed to parse phpdoc of %s::$%s", klass->name.c_str(), var->name.c_str()))) {
          doc_type->location = klass->root->location;
          CREATE_VERTEX(doc_type_check, op_lt_type_rule, doc_type);
          v->type_rule = doc_type_check;
        }
      }
    }
  }
};


/*** Calculate const_type for all nodes ***/
class CalcConstTypePass : public FunctionPassBase {
  private:
    AUTO_PROF (calc_const_type);
  public:
    struct LocalT : public FunctionPassBase::LocalT {
      bool has_nonconst;
      LocalT() :
        has_nonconst (false) {
       }
    };
    string get_description() {
      return "Calc const types";
    }

    void on_exit_edge (VertexPtr v __attribute__((unused)), 
                       LocalT *v_local,
                       VertexPtr from __attribute__((unused)), 
                       LocalT *from_local __attribute__((unused))) {
      v_local->has_nonconst |= from->const_type == cnst_nonconst_val;
    }

    VertexPtr on_exit_vertex (VertexPtr v, LocalT *local) {
      switch (OpInfo::cnst (v->type())) {
        case cnst_func:
          if (v->get_func_id().not_null()) {
            VertexPtr root = v->get_func_id()->root;
            if (root.is_null() || root->type_rule.is_null() || root->type_rule->extra_type != op_ex_rule_const) {
              v->const_type = cnst_nonconst_val;
              break;
            }
          }
          /* fallthrough */
        case cnst_const_func:
          v->const_type = local->has_nonconst ? cnst_nonconst_val : cnst_const_val;
          break;
        case cnst_nonconst_func:
          v->const_type = cnst_nonconst_val;
          break;
        case cnst_not_func:
          v->const_type = cnst_not_val;
          break;
        default:
          kphp_error (0, dl_pstr ("Unknown cnst-type for [op = %d]", v->type()));
          kphp_fail();
          break;
      }
      return v;
    }
};

/*** Throws flags calculcation ***/
class CalcThrowEdgesPass : public FunctionPassBase {
  private:
    vector <FunctionPtr> edges;
  public:
    string get_description() {
      return "Collect throw edges";
    }

    VertexPtr on_enter_vertex (VertexPtr v, LocalT *local __attribute__((unused))) {
      if (v->type() == op_throw) {
        current_function->root->throws_flag = true;
      }
      if (v->type() == op_func_call) {
        FunctionPtr from = v->get_func_id();
        kphp_assert (from.not_null());
        edges.push_back (from);
      }
      return v;
    }

    template <class VisitT>
    bool user_recursion (VertexPtr v, LocalT *local __attribute__((unused)), VisitT &visit) {
      if (v->type() == op_try) {
        VertexAdaptor <op_try> try_v = v;
        visit (try_v->catch_cmd());
        return true;
      }
      return false;
    }

    const vector <FunctionPtr> &get_edges() {
      return edges;
    }
};

struct FunctionAndEdges {
  FunctionPtr function;
  vector <FunctionPtr> *edges;
  FunctionAndEdges() :
   function(),
   edges (NULL) {
  }
  FunctionAndEdges (FunctionPtr function, vector <FunctionPtr> *edges) :
    function (function),
    edges (edges) {
  }

};
class CalcThrowEdgesF {
  public:
    DUMMY_ON_FINISH
    template <class OutputStreamT>
    void execute (FunctionPtr function, OutputStreamT &os) {
      AUTO_PROF (calc_throw_edges);
      CalcThrowEdgesPass pass;
      run_function_pass (function, &pass);

      if (stage::has_error()) {
        return;
      }

      os << FunctionAndEdges (function, new vector <FunctionPtr> (pass.get_edges()));
    }
};

static int throws_func_cnt = 0;
void calc_throws_dfs (FunctionPtr from, IdMap <vector <FunctionPtr> > &graph, vector <FunctionPtr> *bt) {
  throws_func_cnt++;
  //FIXME
  if (false && from->header.not_null()) {
    stringstream ss;
    ss << "Extern function [" << from->name << "] throws \n";
    for (int i = (int)bt->size() - 1; i >= 0; i--) {
      ss << "-->[" << bt->at (i)->name << "]";
    }
    ss << "\n";
    kphp_warning (ss.str().c_str());
  }
  bt->push_back (from);
  for (FunctionRange i = all (graph[from]); !i.empty(); i.next()) {
    FunctionPtr to = *i;
    if (!to->root->throws_flag) {
      to->root->throws_flag = true;
      calc_throws_dfs (to, graph, bt);

    }
  }
  bt->pop_back();
}

class CalcThrowsF {
  private:
    DataStreamRaw <FunctionAndEdges> tmp_stream;
  public:
    CalcThrowsF() {
      tmp_stream.set_sink (true);
    }

    template <class OutputStreamT>
    void execute (FunctionAndEdges input, OutputStreamT &os __attribute__((unused))) {
      tmp_stream << input;
    }

    template <class OutputStreamT>
    void on_finish (OutputStreamT &os) {

      mem_info_t mem_info;
      get_mem_stats (getpid(), &mem_info);

      stage::set_name ("Calc throw");
      stage::set_file (SrcFilePtr());

      stage::die_if_global_errors();

      AUTO_PROF (calc_throws);

      vector <FunctionPtr> from;

      vector <FunctionAndEdges> all = tmp_stream.get_as_vector();
      int cur_id = 0;
      for (int i = 0; i < (int)all.size(); i++) {
        set_index (&all[i].function, cur_id++);
        if (all[i].function->root->throws_flag) {
          from.push_back (all[i].function);
        }
      }

      IdMap < vector <FunctionPtr> > graph;
      graph.update_size (all.size());
      for (int i = 0; i < (int)all.size(); i++) {
        for (int j = 0; j < (int)all[i].edges->size(); j++) {
          graph[(*all[i].edges)[j]].push_back (all[i].function);
        }
      }

      for (int i = 0; i < (int)from.size(); i++) {
        vector <FunctionPtr> bt;
        calc_throws_dfs (from[i], graph, &bt);
      }


      if (stage::has_error()) {
        return;
      }

      for (int i = 0; i < (int)all.size(); i++) {
        os << all[i].function;
        delete all[i].edges;
      }
    }
};

/*** Check function calls ***/
class CheckFunctionCallsPass : public FunctionPassBase {
  private:
    AUTO_PROF (check_function_calls);
  public:
    string get_description() {
      return "Check function calls";
    }

    bool check_function (FunctionPtr function) {
      return default_check_function (function) && function->root->type() == op_function;
    }

    bool on_start (FunctionPtr function) {
      if (!FunctionPassBase::on_start (function)) {
        return false;
      }
      return true;
    }

    void check_func_call (VertexPtr call) {
      FunctionPtr f = call->get_func_id();
      kphp_assert (f.not_null());
      kphp_error_return (f->root.not_null(), dl_pstr ("Function [%s] undeclared", f->name.c_str()));

      if (call->type() == op_func_ptr) {
        return;
      }

      VertexRange func_params = f->root.as <meta_op_function>()->params().
        as <op_func_param_list>()->params();

      if (f->varg_flag) {
        return;
      }

      VertexRange call_params = call->type() == op_constructor_call ? call.as <op_constructor_call>()->args()
                                                                    : call.as <op_func_call>()->args();
      int func_params_n = (int)func_params.size(), call_params_n = (int)call_params.size();

      kphp_error_return (
        call_params_n >= f->min_argn,
        dl_pstr ("Not enough arguments in function [%s:%s] [found %d] [expected at least %d]",
          f->file_id->file_name.c_str(), f->name.c_str(), call_params_n, f->min_argn)
      );

      kphp_error (
        call_params.empty() || call_params[0]->type() != op_varg,
        dl_pstr (
          "call_user_func_array is used for function [%s:%s]",
          f->file_id->file_name.c_str(), f->name.c_str()
        )
      );

      kphp_error_return (
        func_params_n >= call_params_n,
        dl_pstr (
          "Too much arguments in function [%s:%s] [found %d] [expected %d]",
          f->file_id->file_name.c_str(), f->name.c_str(), call_params_n, func_params_n
        )
      );
      for (int i = 0; i < call_params_n; i++) {
        if (func_params[i]->type() == op_func_param_callback) {
          kphp_error_act (
            call_params[i]->type() != op_string,
            "Can't use a string as callback function's name",
            continue
          );
          kphp_error_act (
            call_params[i]->type() == op_func_ptr,
            "Function pointer expected",
            continue
          );

          FunctionPtr func_ptr = call_params[i]->get_func_id();

          kphp_error_act (
            func_ptr->root.not_null(),
            dl_pstr ("Unknown callback function [%s]", func_ptr->name.c_str()),
            continue
          );
          VertexRange cur_params = func_ptr->root.as <meta_op_function>()->params().
            as <op_func_param_list>()->params();
          kphp_error (
            (int)cur_params.size() == func_params[i].as <op_func_param_callback>()->param_cnt,
            "Wrong callback arguments count"
          );
          for (int j = 0; j < (int)cur_params.size(); j++) {
            kphp_error (cur_params[j]->type() == op_func_param,
                       "Callback function with callback parameter");
            kphp_error (cur_params[j].as <op_func_param>()->var()->ref_flag == 0,
                       "Callback function with reference parameter");
          }
        } else {
          kphp_error (call_params[i]->type() != op_func_ptr, "Unexpected function pointer");
        }
      }
    }

    VertexPtr on_enter_vertex (VertexPtr v, LocalT *local __attribute__((unused))) {
      if (v->type() == op_func_ptr || v->type() == op_func_call || v->type() == op_constructor_call) {
        check_func_call (v);
      }
      return v;
    }
};

/*** RL ***/
void rl_calc (VertexPtr root, RLValueType expected_rl_type);

class CalcRLF {
  public:
    DUMMY_ON_FINISH
    template <class OutputStream> void execute (FunctionPtr function, OutputStream &os) {
      AUTO_PROF (calc_rl);
      stage::set_name ("Calc RL");
      stage::set_function (function);

      rl_calc (function->root, val_none);

      if (stage::has_error()) {
        return;
      }

      os << function;
    }
};

/*** Control Flow Graph ***/
class CFGCallback : public cfg::CFGCallbackBase {
  private:
    FunctionPtr function;

    vector <VertexPtr> uninited_vars;
    vector <VarPtr> todo_var;
    vector <vector <vector <VertexPtr> > > todo_parts;
  public:
    void set_function (FunctionPtr new_function) {
      function = new_function;
    }

    void split_var (VarPtr var, vector < vector <VertexPtr> > &parts) {
      assert (var->type() == VarData::var_local_t || var->type() == VarData::var_param_t);
      int parts_size = (int)parts.size();
      if (parts_size == 0) {
        if (var->type() == VarData::var_local_t) {
          function->local_var_ids.erase (
              std::find (
                function->local_var_ids.begin(),
                function->local_var_ids.end(),
                var));
        }
        return;
      }
      assert (parts_size > 1);

      for (int i = 0; i < parts_size; i++) {
        string new_name = var->name + "$v_" + int_to_str (i);
        VarPtr new_var = G->create_var (new_name, var->type());

        for (int j = 0; j < (int)parts[i].size(); j++) {
          VertexPtr v = parts[i][j];
          v->set_var_id (new_var);
        }

        VertexRange params = function->root.
          as <meta_op_function>()->params().
          as <op_func_param_list>()->args();
        if (var->type() == VarData::var_local_t) {
          new_var->type() = VarData::var_local_t;
          function->local_var_ids.push_back (new_var);
        } else if (var->type() == VarData::var_param_t) {
          bool was_var = std::find (
              parts[i].begin(),
              parts[i].end(),
              params[var->param_i].as <op_func_param>()->var()
              ) != parts[i].end();

          if (was_var) { //union of part that contains function argument
            new_var->type() = VarData::var_param_t;
            new_var->param_i = var->param_i;
            new_var->init_val = var->init_val;
            function->param_ids[var->param_i] = new_var;
          } else {
            new_var->type() = VarData::var_local_t;
            function->local_var_ids.push_back (new_var);
          }
        } else {
          kphp_fail();
        }

      }

      if (var->type() == VarData::var_local_t) {
        vector <VarPtr>::iterator tmp = std::find (function->local_var_ids.begin(), function->local_var_ids.end(), var);
        if (function->local_var_ids.end() != tmp) {
          function->local_var_ids.erase (tmp);
        } else {
          kphp_fail();
        }
      }

      todo_var.push_back (var);

      //it could be simple std::move
      todo_parts.push_back (vector <vector <VertexPtr> > ());
      std::swap (todo_parts.back(), parts);
    }
    void unused_vertices (vector <VertexPtr *> &v) {
      for (__typeof (all (v)) i = all (v); !i.empty(); i.next()) {
        CREATE_VERTEX (empty, op_empty);
        **i = empty;
      }
    }
    FunctionPtr get_function() {
      return function;
    }
    void uninited (VertexPtr v) {
      if (v.not_null() && v->type() == op_var) {
        uninited_vars.push_back (v);
        v->get_var_id()->set_uninited_flag (true);
      }
    }

    void check_uninited() {
      for (int i = 0; i < (int)uninited_vars.size(); i++) {
        VertexPtr v = uninited_vars[i];
        VarPtr var = v->get_var_id();
        if (tinf::get_type (v)->ptype() == tp_var || v->extra_type == op_ex_var_superlocal) {
          continue;
        }

        stage::set_location (v->get_location());
        kphp_warning (dl_pstr ("Variable [%s] may be used uninitialized", var->name.c_str()));
      }
    }

    VarPtr merge_vars (vector <VarPtr> vars, const string &new_name) {
      VarPtr new_var = G->create_var (new_name, VarData::var_unknown_t);;
      //new_var->tinf = vars[0]->tinf; //hack, TODO: fix it
      new_var->tinf_node.copy_type_from (tinf::get_type (vars[0]));

      int param_i = -1;
      for (__typeof (all (vars)) i = all (vars); !i.empty(); i.next()) {
        if ((*i)->type() == VarData::var_param_t) {
          param_i = (*i)->param_i;
        } else if ((*i)->type() == VarData::var_local_t) {
          //FIXME: remember to remove all unused variables
          //func->local_var_ids.erase (*i);
          vector <VarPtr>::iterator tmp = std::find (function->local_var_ids.begin(), function->local_var_ids.end(), *i);
          if (function->local_var_ids.end() != tmp) {
            function->local_var_ids.erase (tmp);
          } else {
            kphp_fail();
          }

        } else {
          assert (0 && "unreachable");
        }
      }
      if (param_i != -1) {
        new_var->type() = VarData::var_param_t;
        function->param_ids[param_i] = new_var;
      } else {
        new_var->type() = VarData::var_local_t;
        function->local_var_ids.push_back (new_var);
      }

      return new_var;
    }


    struct MergeData {
      int id;
      VarPtr var;
      MergeData (int id, VarPtr var) :
        id (id),
        var (var) {
      }
    };

    static bool cmp_merge_data (const MergeData &a, const MergeData &b) {
      return type_out (tinf::get_type (a.var)) <
        type_out (tinf::get_type (b.var));
    }
    static bool eq_merge_data (const MergeData &a, const MergeData &b) {
      return type_out (tinf::get_type (a.var)) ==
        type_out (tinf::get_type (b.var));
    }

    void merge_same_type() {
      int todo_n = (int)todo_parts.size();
      for (int todo_i = 0; todo_i < todo_n; todo_i++) {
        vector <vector <VertexPtr> > &parts = todo_parts[todo_i];

        int n = (int)parts.size();
        vector <MergeData> to_merge;
        for (int i = 0; i < n; i++) {
          to_merge.push_back (MergeData (i, parts[i][0]->get_var_id()));
        }
        sort (to_merge.begin(), to_merge.end(), cmp_merge_data);

        vector <int> ids;
        int merge_id = 0;
        for (int i = 0; i <= n; i++) {
          if (i == n || (i > 0 && !eq_merge_data (to_merge[i - 1], to_merge[i]))) {
            vector <VarPtr> vars;
            for (int j = 0; j < (int)ids.size(); j++) {
              vars.push_back (parts[ids[j]][0]->get_var_id());
            }
            string new_name = vars[0]->name;
            int name_i = (int)new_name.size() - 1;
            while (new_name[name_i] != '$') {
              name_i--;
            }
            new_name.erase (name_i);
            new_name += "$v";
            new_name += int_to_str (merge_id++);

            VarPtr new_var = merge_vars (vars, new_name);
            for (int j = 0; j < (int)ids.size(); j++) {
              for (__typeof (all (parts[ids[j]])) v = all (parts[ids[j]]); !v.empty(); v.next()) {
                (*v)->set_var_id (new_var);
              }
            }

            ids.clear();
          }
          if (i == n) {
            break;
          }
          ids.push_back (to_merge[i].id);
        }
      }
    }
};

struct FunctionAndCFG {
  FunctionPtr function;
  CFGCallback *callback;
  FunctionAndCFG() :
    function(),
    callback (NULL) {
  }
  FunctionAndCFG (FunctionPtr function, CFGCallback *callback) :
    function (function),
    callback (callback) {
  }
};

class CheckReturnsF {
public:
  DUMMY_ON_FINISH
  template <class OutputStream>
  void execute (FunctionAndCFG function_and_cfg, OutputStream &os) {
    CheckReturnsPass pass;
    run_function_pass (function_and_cfg.function, &pass);
    if (stage::has_error()) {
      return;
    }
    os << function_and_cfg;
  }
};

class CFGBeginF {
  public:
    DUMMY_ON_FINISH
    template <class OutputStream> void execute (FunctionPtr function, OutputStream &os) {
      AUTO_PROF (CFG);
      stage::set_name ("Calc control flow graph");
      stage::set_function (function);

      cfg::CFG cfg;
      CFGCallback *callback = new CFGCallback();
      callback->set_function (function);
      cfg.run (callback);

      if (stage::has_error()) {
        return;
      }

      os << FunctionAndCFG (function, callback);
    }
};

/*** Type inference ***/
class CollectMainEdgesCallback : public CollectMainEdgesCallbackBase {
  private:
    tinf::TypeInferer *inferer_;

  public:
    CollectMainEdgesCallback (tinf::TypeInferer *inferer) :
      inferer_ (inferer) {
    }

    tinf::Node *node_from_rvalue (const RValue &rvalue) {
      if (rvalue.node == NULL) {
        kphp_assert (rvalue.type != NULL);
        return new tinf::TypeNode (rvalue.type);
      }

      return rvalue.node;
    }

    virtual void require_node (const RValue &rvalue) {
      if (rvalue.node != NULL) {
        inferer_->add_node (rvalue.node);
      }
    }
    virtual void create_set (const LValue &lvalue, const RValue &rvalue) {
      tinf::Edge *edge = new tinf::Edge();
      edge->from = lvalue.value;
      edge->from_at = lvalue.key;
      edge->to = node_from_rvalue (rvalue);
      inferer_->add_edge (edge);
      inferer_->add_node (edge->from);
    }
    virtual void create_less (const RValue &lhs, const RValue &rhs) {
      tinf::Node *a = node_from_rvalue (lhs);
      tinf::Node *b = node_from_rvalue (rhs);
      inferer_->add_node (a);
      inferer_->add_node (b);
      inferer_->add_restriction (new RestrictionLess (a, b));
    }
    virtual void create_isset_check (const RValue &rvalue) {
      tinf::Node *a = node_from_rvalue (rvalue);
      inferer_->add_node (a);
      inferer_->add_restriction (new RestrictionIsset (a));
    }
};

class TypeInfererF {
  //TODO: extract pattern
  private:
  public:
    TypeInfererF() {
      tinf::register_inferer (new tinf::TypeInferer());
    }

    template <class OutputStreamT>
    void execute (FunctionAndCFG input, OutputStreamT &os) {
      AUTO_PROF (tinf_infer_gen_dep);
      CollectMainEdgesCallback callback (tinf::get_inferer());
      CollectMainEdgesPass pass (&callback);
      run_function_pass (input.function, &pass);
      os << input;
    }

    template <class OutputStreamT>
    void on_finish (OutputStreamT &os __attribute__((unused))) {
      //FIXME: rebalance Queues
      vector <Task *> tasks = tinf::get_inferer()->get_tasks();
      for (int i = 0; i < (int)tasks.size(); i++) {
        register_async_task (tasks[i]);
      }
    }
};

class TypeInfererEndF {
  private:
    DataStreamRaw <FunctionAndCFG> tmp_stream;
  public:
    TypeInfererEndF() {
      tmp_stream.set_sink (true);
    }
    template <class OutputStreamT>
    void execute (FunctionAndCFG input, OutputStreamT &os __attribute__((unused))) {
      tmp_stream << input;
    }

    template <class OutputStreamT>
    void on_finish (OutputStreamT &os) {
      tinf::get_inferer()->check_restrictions();
      tinf::get_inferer()->finish();

      vector <FunctionAndCFG> all = tmp_stream.get_as_vector();
      for (int i = 0; i < (int)all.size(); i++) {
        os << all[i];
      }
    }
};

/*** Control flow graph. End ***/
class CFGEndF {
  public:
    DUMMY_ON_FINISH
    template <class OutputStream> void execute (FunctionAndCFG data, OutputStream &os) {
      AUTO_PROF (CFG_End);
      stage::set_name ("Control flow graph. End");
      stage::set_function (data.function);
      if (G->env().get_warnings_level() >= 1) {
        data.callback->check_uninited();
      }
      data.callback->merge_same_type();
      delete data.callback;

      if (stage::has_error()) {
        return;
      }

      os << data.function;
    }
};

/*** Calc val_ref_flag ***/
class CalcValRefPass : public FunctionPassBase {
  private:
    AUTO_PROF (calc_val_ref);
  public:
    string get_description() {
      return "Calc val ref";
    }
    bool check_function (FunctionPtr function) {
      return default_check_function (function) && function->type() != FunctionData::func_extern;
    }

    bool is_allowed_for_getting_val_or_ref(Operation op, bool is_last) {
      switch (op) {
        case op_push_back:
        case op_push_back_return:
        case op_set_value:
          return !is_last;

        case op_list:
          return is_last;

        case op_minus:
        case op_plus:

        case op_set_add:
        case op_set_sub:
        case op_set_mul:
        case op_set_div:
        case op_set_mod:
        case op_set_and:
        case op_set_or:
        case op_set_xor:
        case op_set_shr:
        case op_set_shl:

        case op_add:
        case op_sub:
        case op_mul:
        case op_div:
        case op_mod:
        case op_not:
        case op_log_not:

        case op_prefix_inc:
        case op_prefix_dec:
        case op_postfix_inc:
        case op_postfix_dec:

        case op_exit:
        case op_conv_int:
        case op_conv_int_l:
        case op_conv_float:
        case op_conv_array: // ?
        case op_conv_array_l: // ?
        case op_conv_bool:
        case op_conv_uint:
        case op_conv_long:
        case op_conv_ulong:

        case op_isset:
        case op_unset:
        case op_index:
        case op_switch:
          return true;

        default:
          return false;
      }
    }

    struct LocalT : public FunctionPassBase::LocalT {
      bool allowed;
    };

    void on_enter_edge (VertexPtr vertex __attribute__((unused)), LocalT *local, VertexPtr dest_vertex, LocalT *dest_local __attribute__((unused))) {
      if (local->allowed && dest_vertex->rl_type != val_none && dest_vertex->rl_type != val_error) {
        const TypeData *tp = tinf::get_type (dest_vertex);

        if (tp->ptype() != tp_Unknown && tp->use_or_false()) {
          dest_vertex->val_ref_flag = dest_vertex->rl_type;
        }
      }
    }

    template <class VisitT>
    bool user_recursion (VertexPtr v, LocalT *local, VisitT &visit) {
      for (int i = 0; i < v->size(); ++i) {
        bool is_last_elem = (i == v->size() - 1);

        local->allowed = is_allowed_for_getting_val_or_ref(v->type(), is_last_elem);
        visit (v->ith(i));
      }
      return true;
    }
};

class CalcBadVarsF {
  private:
    DataStreamRaw <pair <FunctionPtr, DepData *> > tmp_stream;
  public:
    CalcBadVarsF() {
      tmp_stream.set_sink (true);
    }

    template <class OutputStreamT>
    void execute (FunctionPtr function, OutputStreamT &os __attribute__((unused))) {
      CalcFuncDepPass pass;
      run_function_pass (function, &pass);
      DepData *data = new DepData();
      swap (*data, *pass.get_data_ptr());
      tmp_stream << std::make_pair (function, data);
    }


    template <class OutputStreamT>
    void on_finish (OutputStreamT &os) {
      stage::die_if_global_errors();

      AUTO_PROF (calc_bad_vars);
      stage::set_name ("Calc bad vars (for UB check)");
      vector <pair <FunctionPtr, DepData *> > tmp_vec = tmp_stream.get_as_vector();
      CalcBadVars calc_bad_vars;
      calc_bad_vars.run (tmp_vec);
      FOREACH (tmp_vec, i) {
        delete (*i).second;
        os << (*i).first;
      }
    }
};

/*** C++ undefined behaviour fixes ***/
class CheckUBF {
  public:
    DUMMY_ON_FINISH;
    template <class OutputStream> void execute (FunctionPtr function, OutputStream &os) {
      AUTO_PROF (check_ub);
      stage::set_name ("Check for undefined behaviour");
      stage::set_function (function);

      if (function->root->type() == op_function) {
        fix_undefined_behaviour (function);
      }

      if (stage::has_error()) {
        return;
      }

      os << function;
    }
};

/*** C++ undefined behaviour fixes ***/
class AnalyzerF {
public:
  DUMMY_ON_FINISH;
  template <class OutputStream> void execute (FunctionPtr function, OutputStream &os) {
    AUTO_PROF (check_ub);
    stage::set_name ("Try to detect common errors");
    stage::set_function (function);

    if (function->root->type() == op_function) {
      analyze_foreach (function);
      analyze_common (function);
    }

    if (stage::has_error()) {
      return;
    }

    os << function;
  }
};

class ExtractResumableCallsPass : public FunctionPassBase {
  private:
    AUTO_PROF (extract_resumable_calls);
    void skip_conv_and_sets(VertexPtr* &replace) {
      while (true) {
        if (!replace) break;
        Operation op = (*replace)->type();
        if (op == op_set_add || op == op_set_sub ||
            op == op_set_mul || op == op_set_div ||
            op == op_set_mod || op == op_set_and ||
            op == op_set_or  || op == op_set_xor ||
            op == op_set_dot || op == op_set     ||
            op == op_set_shr || op == op_set_shl) {
          replace = &((*replace).as<meta_op_binary_op>()->rhs());
        } else if (op == op_conv_int || op == op_conv_bool ||
                   op == op_conv_int_l || op == op_conv_float ||
                   op == op_conv_string || op == op_conv_array ||
                   op == op_conv_array_l || op == op_conv_var ||
                   op == op_conv_uint || op == op_conv_long ||
                   op == op_conv_ulong || op == op_conv_regexp ||
                   op == op_log_not) {
          replace = &((*replace).as <meta_op_unary_op>()->expr());
        } else {
          break;
        }
      }
    }

  public:
    string get_description() {
      return "Extract easy resumable calls";
    }
    bool check_function (FunctionPtr function) {
      return default_check_function (function) && function->type() != FunctionData::func_extern &&
        function->root->resumable_flag;
    }

    struct LocalT : public FunctionPassBase::LocalT {
      bool from_seq;
    };

    void on_enter_edge (VertexPtr vertex, LocalT *local __attribute__((unused)), VertexPtr dest_vertex __attribute__((unused)), LocalT *dest_local) {
      dest_local->from_seq = vertex->type() == op_seq;
    }

    VertexPtr on_enter_vertex (VertexPtr vertex, LocalT *local) {
      if (local->from_seq == false) {
        return vertex;
      }
      VertexPtr* replace = NULL;
      VertexAdaptor <op_func_call> func_call;
      Operation op = vertex->type();
      if (op == op_return) {
        replace = &vertex.as<op_return>()->expr();
      } else if (op == op_set_add || op == op_set_sub ||
                 op == op_set_mul || op == op_set_div ||
                 op == op_set_mod || op == op_set_and ||
                 op == op_set_or  || op == op_set_xor ||
                 op == op_set_dot || op == op_set     ||
                 op == op_set_shr || op == op_set_shl) {
        replace = &vertex.as<meta_op_binary_op>()->rhs();
        if ((*replace)->type() == op_func_call && op == op_set){
            return vertex;
        }
      } else if (op == op_list) {
        replace = &vertex.as<op_list>()->array();
      } else if (op == op_set_value) {
        replace = &vertex.as<op_set_value>()->value();
      } else if (op == op_push_back) {
        replace = &vertex.as<op_push_back>()->value();
      } else if (op == op_if) {
        replace = &vertex.as<op_if>()->cond();
      }
      skip_conv_and_sets(replace);
      if (replace) {
        func_call = *replace;
      }
      if (!replace || func_call.is_null() || func_call->type() != op_func_call) {
        return vertex;
      }
      FunctionPtr func = func_call->get_func_id();
      if (func->root->resumable_flag == false) {
        return vertex;
      }
      CREATE_VERTEX(temp_var, op_var);
      temp_var->str_val = gen_unique_name("resumable_temp_var");
      VarPtr var = G->create_local_var (stage::get_function(), temp_var->str_val, VarData::var_local_t);
      var->tinf_node.copy_type_from (tinf::get_type (func, -1));
      temp_var->set_var_id(var);
      CREATE_VERTEX(set_op, op_set, temp_var, func_call);

      CREATE_VERTEX(temp_var2, op_var);
      temp_var2->str_val = temp_var->str_val;
      temp_var2->set_var_id(var);
      *replace = temp_var2;

      CREATE_VERTEX(seq, op_seq, set_op, vertex);
      return seq;
    }
};


class ExtractAsyncPass : public FunctionPassBase {
  private:
    AUTO_PROF (extract_async);
  public:
    string get_description() {
      return "Extract async";
    }
    bool check_function (FunctionPtr function) {
      return default_check_function (function) && function->type() != FunctionData::func_extern &&
        function->root->resumable_flag;
    }

    struct LocalT : public FunctionPassBase::LocalT {
      bool from_seq;
    };

    void on_enter_edge (VertexPtr vertex, LocalT *local __attribute__((unused)), VertexPtr dest_vertex __attribute__((unused)), LocalT *dest_local) {
      dest_local->from_seq = vertex->type() == op_seq;
    }

    VertexPtr on_enter_vertex (VertexPtr vertex, LocalT *local) {
      if (local->from_seq == false) {
        return vertex;
      }
      VertexAdaptor <op_func_call> func_call;
      VertexPtr lhs;
      if (vertex->type() == op_func_call) {
        func_call = vertex;
      } else if (vertex->type() == op_set) {
        VertexAdaptor <op_set> set  = vertex;
        VertexPtr rhs = set->rhs();
        if (rhs->type() == op_func_call) {
          func_call = rhs;
          lhs = set->lhs();
        }
      }
      if (func_call.is_null()) {
        return vertex;
      }
      FunctionPtr func = func_call->get_func_id();
      if (func->root->resumable_flag == false) {
        return vertex;
      }
      if (lhs.is_null()) {
        CREATE_VERTEX (empty, op_empty);
        set_location (empty, vertex->get_location());
        lhs = empty;
      }
      CREATE_VERTEX (async, op_async, lhs, func_call);
      set_location (async, func_call->get_location());
      return async;
    }
};

class FinalCheckPass : public FunctionPassBase {
  private:
    AUTO_PROF (final_check);
    int from_return;
  public:

    string get_description() {
      return "Final check";
    }

    void init() {
      from_return = 0;
    }

    bool on_start (FunctionPtr function) {
      if (!FunctionPassBase::on_start (function)) {
        return false;
      }
      return function->type() != FunctionData::func_extern;
    }
    VertexPtr on_enter_vertex (VertexPtr vertex, LocalT *local __attribute__((unused))) {
      if (vertex->type() == op_return) {
        from_return++;
      }
      if (vertex->type() == op_func_name) {
        kphp_error (0, dl_pstr("Unexpected function name: '%s'", vertex.as<op_func_name>()->str_val.c_str()));
      }
      if (vertex->type() == op_addr) {
        kphp_error (0, "Getting references is unsupported");
      }
      if (vertex->type() == op_eq3) {
          const TypeData *type_left = tinf::get_type (VertexAdaptor<meta_op_binary_op>(vertex)->lhs());
          const TypeData *type_right = tinf::get_type (VertexAdaptor<meta_op_binary_op>(vertex)->rhs());
          if (type_left->ptype() == tp_float || type_right->ptype() == tp_float) {
            kphp_warning(dl_pstr("Using === with float operand"));
          }
          if (!can_be_same_type(type_left, type_right)) {
              kphp_warning(dl_pstr("=== with %s and %s as operands will be always false", 
                type_out(type_left).c_str(),
                type_out(type_right).c_str()));

          }
      }
      if (vertex->type() == op_add) {
        const TypeData *type_left = tinf::get_type (VertexAdaptor<meta_op_binary_op>(vertex)->lhs());
        const TypeData *type_right = tinf::get_type (VertexAdaptor<meta_op_binary_op>(vertex)->rhs());
        if ((type_left->ptype() == tp_array) ^ (type_right->ptype() == tp_array)) {
          if (type_left->ptype() != tp_var && type_right->ptype() != tp_var) {
            kphp_warning (dl_pstr ("%s + %s is strange operation",
                                   type_out (type_left).c_str (),
                                   type_out (type_right).c_str ()));
          }
        }
      }
      if (vertex->type() == op_sub || vertex->type() == op_mul || vertex->type() == op_div || vertex->type() == op_mod) {
        const TypeData *type_left = tinf::get_type (VertexAdaptor<meta_op_binary_op>(vertex)->lhs());
        const TypeData *type_right = tinf::get_type (VertexAdaptor<meta_op_binary_op>(vertex)->rhs());
        if ((type_left->ptype() == tp_array) || (type_right->ptype() == tp_array)) {
          kphp_warning (dl_pstr ("%s %s %s is strange operation",
                                 OpInfo::str (vertex->type()).c_str(),
                                 type_out(type_left).c_str(),
                                 type_out(type_right).c_str()));
        }
      }

      if (vertex->type() == op_foreach) {
        VertexPtr arr = vertex.as<op_foreach>()->params().as<op_foreach_param>()->xs();
        const TypeData* arrayType = tinf::get_type (arr);
        if (arrayType->ptype() == tp_array) {
          const TypeData* valueType = arrayType->lookup_at (Key::any_key ());
          if (valueType->ptype() == tp_Unknown && !valueType->or_false_flag ()) {
            kphp_error (0, "Can not compile foreach on array of Unknown type");
          }
        }
      }
      if (vertex->type() == op_list) {
        VertexPtr arr = vertex.as<op_list>()->array();
        const TypeData* arrayType = tinf::get_type (arr);
        if (arrayType->ptype() == tp_array) {
          const TypeData* valueType = arrayType->lookup_at (Key::any_key ());
          if (valueType->ptype() == tp_Unknown && !valueType->or_false_flag ()) {
            kphp_error (0, "Can not compile list with array of Unknown type");
          }
        }
      }
      if (vertex->type() == op_unset || vertex->type() == op_isset) {
        for (VertexRange i = vertex.as<meta_op_xset>()->args(); !i.empty(); i.next()) {
          VertexPtr varVertex = *i;
          if (varVertex->type() != op_var) {
            continue;
          }
          VarPtr var = varVertex.as<op_var>()->get_var_id();
          if (vertex->type() == op_unset) {
            kphp_error(!var->is_reference, "Unset of reference variables is not supported");
            if (var->type() == VarData::var_global_t) {
              FunctionPtr f = stage::get_function();
              if (f->type() != FunctionData::func_global && f->type() != FunctionData::func_switch) {
                kphp_error(0, "Unset of global variables in functions is not supported");
              }
            }
          } else {
            kphp_error(var->type() != VarData::var_const_t, "Can't use isset on const variable");
          }
        }
      }
      if (vertex->type() == op_func_call) {
        FunctionPtr fun = vertex.as<op_func_call>()->get_func_id();
        if (fun->root->void_flag && vertex->rl_type == val_r && from_return == 0) {
          if (fun->type() != FunctionData::func_switch && fun->file_id->main_func_name != fun->name) {
            kphp_warning(dl_pstr("Using result of void function %s\n", fun->name.c_str()));
          }
        }
      }
      //TODO: may be this should be moved to tinf_check
      return vertex;
    }
    template <class VisitT>
    bool user_recursion (VertexPtr v, LocalT *local __attribute__((unused)), VisitT &visit) {
      if (v->type() == op_function) {
        visit (v.as <op_function>()->cmd());
        return true;
      }

      if (v->type() == op_func_call || v->type() == op_var ||
          v->type() == op_index || v->type() == op_constructor_call) {
        if (v->rl_type == val_r) {
          const TypeData *type = tinf::get_type (v);
          // пока что, т.к. все методы всех классов считаются required, в реально неиспользуемых будет Unknown
          // (потом когда-нибудь можно убирать реально неиспользуемые из required-списка, и убрать дополнительное условие)
          if (type->ptype() == tp_Unknown && !type->use_or_false() && !current_function->is_instance_function()) {
            while (v->type() == op_index) {
              v = v.as <op_index>()->array();
            }
            string desc = "Using Unknown type : ";
            if (v->type() == op_var) {
              desc += "variable [$" + v.as <op_var>()->get_var_id()->name + "]";
            } else if (v->type() == op_func_call) {
              desc += "function [" + v.as <op_func_call>()->get_func_id()->name + "]";
            } else if (v->type() == op_constructor_call) {
              desc += "constructor [" + v.as <op_constructor_call>()->get_func_id()->name + "]";
            } else {
              desc += "...";
            }

            if (v->type() == op_var) {
              VarPtr var = v->get_var_id();
              if (var.not_null()) {
                FunctionPtr holder_func = var->holder_func;
                if (holder_func.not_null() && holder_func->is_required) {
                  desc += dl_pstr("\nMaybe because `@kphp-required` is set for function `%s` but it has never been used", holder_func->name.c_str());
                }
              }
            }

            kphp_error (0, desc.c_str());
            return true;
          }
        }
      }

      return false;
    }

    VertexPtr on_exit_vertex (VertexPtr vertex, LocalT *local __attribute__((unused))) {
      if (vertex->type() == op_return) {
        from_return--;
      }
      return vertex;
    }
};

// todo почему-то удалилось из server
void prepare_generate_class (ClassPtr klass __attribute__((unused))) {
}

template <class OutputStream>
class WriterCallback : public WriterCallbackBase {
  private:
    OutputStream &os;
  public:
    WriterCallback (OutputStream &os, const string dir __attribute__((unused)) = "./") :
      os (os) {
    }

    void on_end_write (WriterData *data) {
      if (stage::has_error()) {
        return;
      }

      WriterData *data_copy = new WriterData();
      data_copy->swap (*data);
      data_copy->calc_crc();
      os << data_copy;
    }
};

class CodeGenF {
  //TODO: extract pattern
  private:
    DataStreamRaw <FunctionPtr> tmp_stream;
    map <string, long long> subdir_hash;

  public:
    CodeGenF() {
      tmp_stream.set_sink (true);
    }

    template <class OutputStreamT>
    void execute (FunctionPtr input, OutputStreamT &os __attribute__((unused))) {
      tmp_stream << input;
    }

    int calc_count_of_parts(size_t cnt_global_vars) {
      return cnt_global_vars > 1000 ? 64 : 1;
    }

    template <class OutputStreamT>
    void on_finish (OutputStreamT &os) {
      AUTO_PROF (code_gen);

      stage::set_name ("GenerateCode");
      stage::set_file (SrcFilePtr());
      stage::die_if_global_errors();

      vector <FunctionPtr> xall = tmp_stream.get_as_vector();
      sort (xall.begin(), xall.end());
      const vector <ClassPtr> &all_classes = G->get_classes();

      //TODO: delete W_ptr
      CodeGenerator *W_ptr = new CodeGenerator();
      CodeGenerator &W = *W_ptr;

      if (G->env().get_use_safe_integer_arithmetic()) {
        W.use_safe_integer_arithmetic();
      }

      G->init_dest_dir();

      W.init (new WriterCallback <OutputStreamT> (os));

      //TODO: parallelize;
      FOREACH(xall, fun) {
        prepare_generate_function(*fun);
      }
      for (vector <ClassPtr>::const_iterator c = all_classes.begin(); c != all_classes.end(); ++c) {
        if (c->not_null() && !(*c)->is_fully_static())
          prepare_generate_class(*c);
      }

      vector <SrcFilePtr> main_files = G->get_main_files();
      vector <FunctionPtr> all_functions;
      vector <FunctionPtr> source_functions;
      for (int i = 0; i < (int)xall.size(); i++) {
        FunctionPtr function = xall[i];
        if (function->used_in_source) {
          source_functions.push_back (function);
        }
        if (function->type() == FunctionData::func_extern) {
          continue;
        }
        all_functions.push_back (function);
        W << Async (FunctionH (function));
        W << Async (FunctionCpp (function));
      }

      for (vector <ClassPtr>::const_iterator c = all_classes.begin(); c != all_classes.end(); ++c) {
        if (c->not_null() && !(*c)->is_fully_static()) {
          W << Async(ClassDeclaration(*c));
        }
      }

      //W << Async (XmainCpp());
      W << Async (InitScriptsH());
      FOREACH (main_files, j) {
        W << Async (DfsInit (*j));
      }
      W << Async (InitScriptsCpp (/*std::move*/main_files, source_functions, all_functions));

      vector <VarPtr> vars = G->get_global_vars();
      int parts_cnt = calc_count_of_parts(vars.size());
      W << Async (VarsCpp (vars, parts_cnt));

      write_hashes_of_subdirs_to_dep_files(W);

      write_tl_schema(W);
    }

  private:
    void prepare_generate_function (FunctionPtr func) {
      string file_name = func->name;
      std::replace(file_name.begin(), file_name.end(), '$', '@');

      string file_subdir = func->file_id->short_file_name;

      if (G->env().get_use_subdirs()) {
        func->header_name = file_name + ".h";
        func->subdir = get_subdir(file_subdir);

        recalc_hash_of_subdirectory(func->subdir, func->header_name);

        if (!func->root->inline_flag) {
          func->src_name = file_name + ".cpp";
          func->src_full_name = func->subdir + "/" + func->src_name;

          recalc_hash_of_subdirectory(func->subdir, func->src_name);
        }

        func->header_full_name = func->subdir + "/" + func->header_name;
      } else {
        string full_name = file_subdir + "." + file_name;
        func->src_name = full_name + ".cpp";
        func->header_name = full_name + ".h";
        func->subdir = "";
        func->src_full_name = func->src_name;
        func->header_full_name = func->header_name;
      }

      my_unique (&func->static_var_ids);
      my_unique (&func->global_var_ids);
      my_unique (&func->header_global_var_ids);
      my_unique (&func->local_var_ids);
    }

    string get_subdir (const string &base) {
      kphp_assert (G->env().get_use_subdirs());

      int func_hash = hash(base);
      int bucket = func_hash % 100;

      return string("o_") + int_to_str(bucket);
    }

    void recalc_hash_of_subdirectory(const string &subdir, const string &file_name) {
      long long &cur_hash = subdir_hash[subdir];
      cur_hash = cur_hash * 987654321 + hash(file_name);
    }

    void write_hashes_of_subdirs_to_dep_files(CodeGenerator &W) {
      FOREACH (subdir_hash, i) {
        string dir = i->first;
        long long hash = i->second;
        W << OpenFile ("_dep.cpp", dir, false);
        char tmp[100];
        sprintf (tmp, "%llx", hash);
        W << "//" << (const char *)tmp << NL;
        W << CloseFile();
      }
    }

    void write_tl_schema(CodeGenerator &W) {
      string schema;
      int schema_length = -1;
      if (G->env().get_tl_schema_file() != "") {
        FILE *f = fopen(G->env().get_tl_schema_file().c_str(), "r");
        const int MAX_SCHEMA_LEN = 1024 * 1024;
        static char buf[MAX_SCHEMA_LEN + 1];
        kphp_assert (f && "can't open tl schema file");
        schema_length = fread(buf, 1, sizeof(buf), f);
        kphp_assert (schema_length > 0 && schema_length < MAX_SCHEMA_LEN);
        schema.assign(buf, buf + schema_length);
        kphp_assert (!fclose(f));
      }
      W << OpenFile("_tl_schema.cpp", "", false);
      W << "extern \"C\" " << BEGIN;
      W << "const char *builtin_tl_schema = " << NL << Indent(2);
      compile_string_raw(schema, W);
      W << ";" << NL;
      W << "int builtin_tl_schema_length = ";
      char buf[100];
      sprintf(buf, "%d", schema_length);
      W << string(buf) << ";" << NL;
      W << END;
      W << CloseFile();
    }
};

class WriteFilesF {
  public:
    DUMMY_ON_FINISH;
    template <class OutputStreamT>
      void execute (WriterData *data, OutputStreamT &os __attribute__((unused))) {
        AUTO_PROF (end_write);
        stage::set_name ("Write files");
        string dir = G->cpp_dir;

        string cur_file_name = data->file_name;
        string cur_subdir = data->subdir;

        string full_file_name = dir;
        if (!cur_subdir.empty()) {
          full_file_name += cur_subdir;
          full_file_name += "/";
        }
        full_file_name += cur_file_name;

        File *file =  G->get_file_info (full_file_name);
        file->needed = true;
        file->includes = data->get_includes();
        file->compile_with_debug_info_flag = data->compile_with_debug_info();

        if (file->on_disk) {
          if (file->crc64 == (unsigned long long)-1) {
            FILE *old_file = fopen (full_file_name.c_str(), "r");
            dl_passert (old_file != NULL, 
                dl_pstr ("Failed to open [%s]", full_file_name.c_str()));
            unsigned long long old_crc = 0;
            unsigned long long old_crc_with_comments = static_cast<unsigned long long>(-1);

            if (fscanf (old_file, "//crc64:%Lx", &old_crc) != 1) {
              kphp_warning (dl_pstr ("can't read crc64 from [%s]\n", full_file_name.c_str()));
              old_crc = static_cast<unsigned long long>(-1);
            } else {
              if (fscanf (old_file, " //crc64_with_comments:%Lx", &old_crc_with_comments) != 1) {
                old_crc_with_comments = static_cast<unsigned long long>(-1);
              }
            }
            fclose (old_file);

            file->crc64 = old_crc;
            file->crc64_with_comments = old_crc_with_comments;
          }
        }

        bool need_del = false;
        bool need_fix = false;
        bool need_save_time = false;
        unsigned long long crc = data->calc_crc();
        string code_str;
        data->dump (code_str);
        unsigned long long crc_with_comments = compute_crc64 (code_str.c_str(), code_str.length());
        if (file->on_disk) {
          if (file->crc64 != crc) {
            need_fix = true;
            need_del = true;
          } else if (file->crc64_with_comments != crc_with_comments) {
            need_fix = true;
            need_del = true;
            need_save_time = true;
          }
        } else {
          need_fix = true;
        }

        if (need_fix) {
          long long mtime_before = 0;
          if (need_save_time) {
            int upd_res = file->upd_mtime();
            mtime_before = file->mtime;
            if (upd_res <= 0) {
              need_save_time = false;
            }
          }
          if (!need_save_time && G->env().get_verbosity() > 0) {
            fprintf (stderr, "File [%s] changed\n", full_file_name.c_str());
          }
          if (need_del) {
            int err = unlink (full_file_name.c_str());
            dl_passert (err == 0, dl_pstr ("Failed to unlink [%s]", full_file_name.c_str()));
          }
          FILE *dest_file = fopen (full_file_name.c_str(), "w");
          dl_passert (dest_file != NULL, 
              dl_pstr ("Failed to open [%s] for write\n", full_file_name.c_str()));

          dl_pcheck (fprintf (dest_file, "//crc64:%016Lx\n", ~crc));
          dl_pcheck (fprintf (dest_file, "//crc64_with_comments:%016Lx\n", ~crc_with_comments));
          dl_pcheck (fprintf (dest_file, "%s", code_str.c_str()));
          dl_pcheck (fflush (dest_file));
          dl_pcheck (fseek (dest_file, 0, SEEK_SET));
          dl_pcheck (fprintf (dest_file, "//crc64:%016Lx\n", crc));
          dl_pcheck (fprintf (dest_file, "//crc64_with_comments:%016Lx\n", crc_with_comments));

          dl_pcheck (fflush (dest_file));
          dl_pcheck (fclose (dest_file));

          file->crc64 =  crc;
          file->crc64_with_comments = crc_with_comments;
          file->on_disk = true;

          if (need_save_time) {
            file->set_mtime(mtime_before);
          }
          long long mtime = file->upd_mtime();
          dl_assert (mtime > 0, "Stat failed");
          kphp_error(!need_save_time || file->mtime == mtime_before, "Failed to set previous mtime\n");
        }
        delete data;
      }
};

class CollectClassF {
  public:
    DUMMY_ON_FINISH;
    template <class OutputStreamT>
    void execute (ReadyFunctionPtr ready_data, OutputStreamT &os) {
      stage::set_name("Collect classes");

      FunctionPtr data = ready_data.function;
      if (data->class_id.not_null() && data->class_id->init_function == data) {
        ClassPtr klass = data->class_id;
        if (!data->class_extends.empty()) {
          klass->extends = resolve_uses(data, data->class_extends, '\\');
        }
        if (!klass->extends.empty()) {
          klass->parent_class = G->get_class(klass->extends);
          kphp_assert(klass->parent_class.not_null());
          kphp_error(klass->is_fully_static() && klass->parent_class->is_fully_static(),
              dl_pstr("Invalid class extends %s and %s: extends is available only if classes are only-static", klass->name.c_str(), klass->parent_class->name.c_str()));
        } else {
          klass->parent_class = ClassPtr();
        }
      }
      os << data;
    }
};

/*
 * Для всех функций, всех переменных проверяем, что если делались предположения насчёт классов, то они совпали с выведенными.
 */
class CheckInferredInstances {
public:
  DUMMY_ON_FINISH

  template <class OutputStream>
  void execute (FunctionPtr function, OutputStream &os) {
    stage::set_name("Check inferred instances");
    stage::set_function(function);

    if (function->type() != FunctionData::func_extern && !function->assumptions.empty()) {
      analyze_function_vars(function);
    }

    if (stage::has_error()) {
      return;
    }

    os << function;
  }

  void analyze_function_vars (FunctionPtr function) {
    FOREACH(function->local_var_ids, var) {
      analyze_function_var(function, *var);
    }
    FOREACH(function->global_var_ids, var) {
      analyze_function_var(function, *var);
    }
    FOREACH(function->static_var_ids, var) {
      analyze_function_var(function, *var);
    }
  }

  inline void analyze_function_var (FunctionPtr function, VarPtr var) {
    ClassPtr klass;
    AssumType assum = assumption_get(function, var->name, klass);

    if (assum == assum_instance) {
      const TypeData *t = var->tinf_node.get_type();
      kphp_error((t->ptype() == tp_Class && klass.ptr == t->class_type().ptr)
                 || (t->ptype() == tp_Exception || t->ptype() == tp_MC),
          dl_pstr("var $%s assumed to be %s, but inferred %s", var->name.c_str(), klass->name.c_str(), type_out(t).c_str()));
    } else if (assum == assum_instance_array) {
      const TypeData *t = var->tinf_node.get_type()->lookup_at(Key::any_key());
      kphp_error(t != NULL && ((t->ptype() == tp_Class && klass.ptr == t->class_type().ptr)
                               || (t->ptype() == tp_Exception || t->ptype() == tp_MC)),
          dl_pstr("var $%s assumed to be %s[], but inferred %s", var->name.c_str(), klass->name.c_str(), type_out(var->tinf_node.get_type()).c_str()));
    }
  }
};

bool compiler_execute (KphpEnviroment *env) {
  double st = dl_time();
  G = new CompilerCore();
  G->register_env (env);
  G->start();
  if (!env->get_warnings_filename().empty()) {
    FILE *f = fopen (env->get_warnings_filename().c_str(), "w");
    if (!f) {
      fprintf (stderr, "Can't open warnings-file %s\n", env->get_warnings_filename().c_str());
      return false;
    }

    env->set_warnings_file (f);
  } else {
    env->set_warnings_file (NULL);
  }
  if (!env->get_stats_filename().empty()) {
    FILE *f = fopen (env->get_stats_filename().c_str(), "w");
    if (!f) {
      fprintf (stderr, "Can't open stats-file %s\n", env->get_stats_filename().c_str());
      return false;
    }
    env->set_stats_file (f);
  } else {
    env->set_stats_file (NULL);
  }

  //TODO: call it with pthread_once on need
  lexer_init();
  gen_tree_init();
  OpInfo::init_static();
  MultiKey::init_static();
  TypeData::init_static();
//  PhpDocTypeRuleParser::run_tipa_unit_tests_parsing_tags(); return true;

  DataStreamRaw <SrcFilePtr> file_stream;

  for (int i = 0; i < (int)env->get_main_files().size(); i++) {
    G->register_main_file (env->get_main_files()[i], file_stream);
  }

  {
    SchedulerBase *scheduler;
    if (G->env().get_threads_count() == 1) {
      scheduler = new OneThreadScheduler();
    } else {
      Scheduler *s = new Scheduler();
      s->set_threads_count (G->env().get_threads_count());
      scheduler = s;
    }

    Pipe <LoadFileF,
         DataStream <SrcFilePtr>,
         DataStream <SrcFilePtr> > load_file_pipe (true);
    Pipe <FileToTokensF,
         DataStream <SrcFilePtr>,
         DataStream <FileAndTokens> > file_to_tokens_pipe (true);
    Pipe <ParseF,
         DataStream <FileAndTokens>,
         DataStream <FunctionPtr> > parse_pipe (true);
    Pipe <ApplyBreakFileF,
         DataStream <FunctionPtr>,
         DataStream <FunctionPtr> > apply_break_file_pipe (true);
    Pipe <SplitSwitchF,
         DataStream <FunctionPtr>,
         DataStream <FunctionPtr> > split_switch_pipe (true);
    Pipe <FunctionPassF<CreateSwitchForeachVarsF>,
         DataStream <FunctionPtr>,
         DataStream <FunctionPtr> > create_switch_foreach_vars_pipe (true);
    Pipe <CollectRequiredF,
         DataStream <FunctionPtr>,
         DataStreamTriple <ReadyFunctionPtr, SrcFilePtr, FunctionPtr> > collect_required_pipe (true);
    Pipe <FunctionPassF <CalcLocationsPass>,
         DataStream <FunctionPtr>,
         DataStream <FunctionPtr> > calc_locations_pipe (true);
    Pipe <PreprocessDefinesConcatenationF,
        DataStream<FunctionPtr>,
        DataStream<FunctionPtr> > process_defines_concat (true);
    FunctionPassPipe <CollectDefinesPass>::Self collect_defines_pipe (true);

    Pipe <CheckReturnsF,
      DataStream <FunctionAndCFG>,
      DataStream <FunctionAndCFG> > check_returns_pipe (true);

    Pipe <SyncPipeF <FunctionPtr>,
         DataStream <FunctionPtr>,
         DataStream <FunctionPtr> > first_sync_pipe (true, true);
    Pipe <PrepareFunctionF,
         DataStream <FunctionPtr>,
         DataStream <FunctionPtr> > prepare_function_pipe (true);
    Pipe <SyncPipeF <FunctionPtr>,
         DataStream <FunctionPtr>,
         DataStream <FunctionPtr> > second_sync_pipe (true, true);
    Pipe <SyncPipeF <ReadyFunctionPtr>,
         DataStream <ReadyFunctionPtr>,
         DataStream <ReadyFunctionPtr> > first_class_sync_pipe (true, true);
    Pipe <CollectClassF,
         DataStream <ReadyFunctionPtr>,
         DataStream <FunctionPtr> > collect_classes_pipe (true);
    FunctionPassPipe <RegisterDefinesPass>::Self register_defines_pipe (true);
    FunctionPassPipe <PreprocessVarargPass>::Self preprocess_vararg_pipe (true);
    FunctionPassPipe <PreprocessEq3Pass>::Self preprocess_eq3_pipe (true);
    FunctionPassPipe <PreprocessFunctionCPass>::Self preprocess_function_c_pipe (true);
    FunctionPassPipe <PreprocessBreakPass>::Self preprocess_break_pipe (true);
    FunctionPassPipe <RegisterVariables>::Self register_variables_pipe (true);
    FunctionPassPipe <CheckInstanceProps>::Self check_instance_props_pipe (true);
    FunctionPassPipe <CalcConstTypePass>::Self calc_const_type_pipe (true);
    FunctionPassPipe <CollectConstVarsPass>::Self collect_const_vars_pipe (true);
    Pipe <CalcThrowEdgesF,
         DataStream <FunctionPtr>,
         DataStream <FunctionAndEdges> > calc_throw_edges_pipe (true);
    Pipe <CalcThrowsF,
         DataStream <FunctionAndEdges>,
         DataStream <FunctionPtr> > calc_throws_pipe (true, true);
    FunctionPassPipe <CheckFunctionCallsPass>::Self check_func_calls_pipe (true);
    Pipe <CalcRLF,
         DataStream <FunctionPtr>,
         DataStream <FunctionPtr> > calc_rl_pipe (true);
    Pipe <CFGBeginF,
         DataStream <FunctionPtr>,
         DataStream <FunctionAndCFG> > cfg_begin_pipe (true);
    //tmp
    Pipe <SyncPipeF <FunctionAndCFG>,
         DataStream <FunctionAndCFG>,
         DataStream <FunctionAndCFG> > tmp_sync_pipe (true, true);

    Pipe <TypeInfererF,
         DataStream <FunctionAndCFG>,
         DataStream <FunctionAndCFG> > type_inferer_pipe (true);
    Pipe <TypeInfererEndF,
         DataStream <FunctionAndCFG>,
         DataStream <FunctionAndCFG> > type_inferer_end_pipe (true);
    Pipe <CFGEndF,
         DataStream <FunctionAndCFG>,
         DataStream <FunctionPtr> > cfg_end_pipe (true);
    Pipe <CheckInferredInstances,
         DataStream <FunctionPtr>,
         DataStream <FunctionPtr> > check_inferred_instances_pipe(true);
    FunctionPassPipe <OptimizationPass>::Self optimization_pipe (true);
    FunctionPassPipe <CalcValRefPass>::Self calc_val_ref_pipe (true);
    Pipe <CalcBadVarsF,
         DataStream <FunctionPtr>,
         DataStream <FunctionPtr> > calc_bad_vars_pipe (true);
    Pipe <CheckUBF,
         DataStream <FunctionPtr>,
         DataStream <FunctionPtr> > check_ub_pipe (true);
    FunctionPassPipe <ExtractAsyncPass>::Self extract_async_pipe (true);
    FunctionPassPipe <ExtractResumableCallsPass>::Self extract_resumable_calls_pipe (true);
    Pipe <SyncPipeF <FunctionPtr>,
         DataStream <FunctionPtr>,
         DataStream <FunctionPtr> > third_sync_pipe (true, true);
    Pipe <SyncPipeF <FunctionPtr>,
         DataStream <FunctionPtr>,
         DataStream <FunctionPtr> > forth_sync_pipe (true, true);
    Pipe <AnalyzerF,
      DataStream <FunctionPtr>,
      DataStream <FunctionPtr> > analyzer_pipe (true);
    FunctionPassPipe <CheckAccessModifiers>::Self check_access_modifiers_pass(true);
    FunctionPassPipe <FinalCheckPass>::Self final_check_pass (true);
    Pipe <CodeGenF,
         DataStream <FunctionPtr>,
         DataStream <WriterData *> > code_gen_pipe;
    Pipe <WriteFilesF,
         DataStream <WriterData *>,
         EmptyStream> write_files_pipe (false);


    pipe_input (load_file_pipe).set_stream (&file_stream);

    scheduler_constructor(*scheduler, load_file_pipe)
        >>
        file_to_tokens_pipe >>
        parse_pipe >>
        apply_break_file_pipe >>
        split_switch_pipe >>
        create_switch_foreach_vars_pipe >>
        collect_required_pipe >> use_first_output() >>
        first_class_sync_pipe >> sync_node() >>
        collect_classes_pipe >>
        calc_locations_pipe >>
        process_defines_concat >> sync_node() >>
        collect_defines_pipe >>
        first_sync_pipe >> sync_node() >>
        prepare_function_pipe >>
        second_sync_pipe >> sync_node() >>
        register_defines_pipe >>
        preprocess_vararg_pipe >>
        preprocess_eq3_pipe >>
        preprocess_function_c_pipe >>
        preprocess_break_pipe >>
        calc_const_type_pipe >>
        collect_const_vars_pipe >>
        check_instance_props_pipe >>
        register_variables_pipe >>
        calc_throw_edges_pipe >>
        calc_throws_pipe >> sync_node() >>
        check_func_calls_pipe >>
        calc_rl_pipe >>
        cfg_begin_pipe >>
        check_returns_pipe >>
        tmp_sync_pipe >> sync_node() >>
        type_inferer_pipe >> sync_node() >>
        type_inferer_end_pipe >> sync_node() >>
        cfg_end_pipe >>
        check_inferred_instances_pipe >>
        optimization_pipe >>
        calc_val_ref_pipe >>
        calc_bad_vars_pipe >> sync_node() >>
        check_ub_pipe >>
        extract_resumable_calls_pipe >>
        extract_async_pipe >>
        analyzer_pipe >>
        check_access_modifiers_pass >>
        final_check_pass >>
        code_gen_pipe >> sync_node() >>
        write_files_pipe;

    scheduler_constructor (*scheduler, collect_required_pipe) >> use_second_output() >>
      load_file_pipe;
    scheduler_constructor (*scheduler, collect_required_pipe) >> use_third_output() >>
      apply_break_file_pipe;

    get_scheduler()->execute();
  }

  stage::die_if_global_errors();
  int verbosity = G->env().get_verbosity();
  if (G->env().get_use_make()) {
    fprintf (stderr, "start make\n");
    G->make();
  }
  G->finish();
  if (verbosity > 1) {
    profiler_print_all();
    double en = dl_time();
    double passed = en - st;
    fprintf (stderr, "PASSED: %lf\n", passed);
    mem_info_t mem_info;
    get_mem_stats (getpid(), &mem_info);
    fprintf (stderr, "RSS: %lluKb\n", mem_info.rss);
  }
  return true;
}
