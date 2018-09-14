#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <functional>
#include <mutex>

#include "compiler/analyzer.h"
#include "compiler/bicycle.h"
#include "compiler/cfg.h"
#include "compiler/code-gen.h"
#include "compiler/compiler-core.h"
#include "compiler/compiler.h"
#include "compiler/const-manipulations.h"
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
#include "compiler/phpdoc.h"
#include "compiler/stage.h"
#include "compiler/type-inferer.h"
#include "compiler/utils.h"
#include "common/crc32.h"
#include "common/version-string.h"

/*** Load file ***/
class LoadFileF {
public:
  DUMMY_ON_FINISH

  template<class OutputStream>
  void execute(SrcFilePtr file, OutputStream &os) {
    AUTO_PROF (load_files);
    stage::set_name("Load file");
    stage::set_file(file);

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
  vector<Token *> *tokens;

  FileAndTokens() :
    file(),
    tokens(nullptr) {
  }
};

class FileToTokensF {
public:
  DUMMY_ON_FINISH

  template<class OutputStream>
  void execute(SrcFilePtr file, OutputStream &os) {
    AUTO_PROF (lexer);
    stage::set_name("Split file to tokens");
    stage::set_file(file);
    kphp_assert (file.not_null());

    kphp_assert (file->loaded);
    FileAndTokens res;
    res.file = file;
    res.tokens = new vector<Token *>();
    php_text_to_tokens(
      &file->text[0], (int)file->text.length(),
      file->main_func_name, res.tokens
    );

    if (stage::has_error()) {
      return;
    }

    os << res;
  }
};

class ParseF {
public:
  DUMMY_ON_FINISH

  template<class OutputStream>
  void execute(FileAndTokens file_and_tokens, OutputStream &os) {
    AUTO_PROF (gentree);
    stage::set_name("Parse file");
    stage::set_file(file_and_tokens.file);
    kphp_assert (file_and_tokens.file.not_null());

    GenTreeCallback callback(os);
    php_gen_tree(file_and_tokens.tokens, file_and_tokens.file->class_context, file_and_tokens.file->main_func_name, callback);
  }
};

/*** Split main functions by #break_file ***/
bool split_by_break_file(VertexAdaptor<op_function> root, vector<VertexPtr> *res) {
  bool need_split = false;

  VertexAdaptor<op_seq> seq = root->cmd();
  for (auto i : seq->args()) {
    if (i->type() == op_break_file) {
      need_split = true;
      break;
    }
  }

  if (!need_split) {
    return false;
  }

  vector<VertexPtr> splitted;
  {
    vector<VertexPtr> cur_next;
    VertexRange args = seq->args();
    auto cur_arg = args.begin();
    while (true) {
      if ((cur_arg == args.end()) || (*cur_arg)->type() == op_break_file) {
        CREATE_VERTEX (new_seq, op_seq, cur_next);
        splitted.push_back(new_seq);
        cur_next.clear();
      } else {
        cur_next.push_back(*cur_arg);
      }
      if (cur_arg == args.end()) {
        break;
      }
      ++cur_arg;
    }
  }

  int splitted_n = (int)splitted.size();
  VertexAdaptor<op_function> next_func;
  for (int i = splitted_n - 1; i >= 0; i--) {
    string func_name_str = gen_unique_name(stage::get_file()->short_file_name);
    CREATE_VERTEX (func_name, op_func_name);
    func_name->str_val = func_name_str;
    CREATE_VERTEX (func_params, op_func_param_list);
    CREATE_VERTEX (func, op_function, func_name, func_params, splitted[i]);
    func->extra_type = op_ex_func_global;

    if (next_func.not_null()) {
      CREATE_VERTEX (call, op_func_call);
      call->str_val = next_func->name()->get_string();
      GenTree::func_force_return(func, call);
    } else {
      GenTree::func_force_return(func);
    }
    next_func = func;

    res->push_back(func);
  }

  CREATE_VERTEX (func_call, op_func_call);
  func_call->str_val = next_func->name()->get_string();

  CREATE_VERTEX (ret, op_return, func_call);
  CREATE_VERTEX (new_seq, op_seq, ret);
  root->cmd() = new_seq;

  return true;
}

class ApplyBreakFileF {
public:
  DUMMY_ON_FINISH

  template<class OutputStream>
  void execute(FunctionPtr function, OutputStream &os) {
    AUTO_PROF (apply_break_file);

    stage::set_name("Apply #break_file");
    stage::set_function(function);
    kphp_assert (function.not_null());

    VertexPtr root = function->root;

    if (function->type() != FunctionData::func_global || root->type() != op_function) {
      os << function;
      return;
    }

    vector<VertexPtr> splitted;
    split_by_break_file(root, &splitted);

    if (stage::has_error()) {
      return;
    }

    for (int i = 0; i < (int)splitted.size(); i++) {
      G->register_function(FunctionInfo(
        splitted[i], function->namespace_name,
        function->class_name, function->class_context_name,
        function->namespace_uses, function->class_extends, false, access_nonmember
      ), os);
    }

    os << function;
  }
};

/*** Replace cases in big global functions with functions call ***/
class SplitSwitchF {
public:
  DUMMY_ON_FINISH

  template<class OutputStream>
  void execute(FunctionPtr function, OutputStream &os) {
    SplitSwitchPass split_switch;
    run_function_pass(function, &split_switch);

    for (VertexPtr new_function : split_switch.get_new_functions()) {
      G->register_function(FunctionInfo(new_function, function->namespace_name,
                                        function->class_name, function->class_context_name, function->namespace_uses,
                                        function->class_extends, false, access_nonmember), os);
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

  ReadyFunctionPtr() {}

  explicit ReadyFunctionPtr(FunctionPtr function) :
    function(function) {
  }

  operator FunctionPtr() const {
    return function;
  }
};

class CollectRequiredCallbackBase {
public:
  virtual pair<SrcFilePtr, bool> require_file(const string &file_name, const string &class_context) = 0;
  virtual void require_function_set(
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
  CollectRequiredPass(CollectRequiredCallbackBase *callback) :
    force_func_ptr(false),
    callback(callback) {
  }

  struct LocalT : public FunctionPassBase::LocalT {
    bool saved_force_func_ptr;
  };

  string get_description() {
    return "Collect required";
  }

  void require_class(const string &class_name, const string &context_name) {
    pair<SrcFilePtr, bool> res = callback->require_file(class_name + ".php", context_name);
    kphp_error(res.first.not_null(), dl_pstr("Class %s not found", class_name.c_str()));
    if (res.second) {
      res.first->req_id = current_function;
    }
  }

  template<class VisitT>
  bool user_recursion(VertexPtr v, LocalT *local __attribute__((unused)), VisitT &visit __attribute__((unused))) {
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

  string get_class_name_for(const string &name, char delim = '$') {
    size_t pos$$ = name.find("::");
    if (pos$$ == string::npos) {
      return "";
    }

    string class_name = name.substr(0, pos$$);
    kphp_assert(!class_name.empty());
    return resolve_uses(current_function, class_name, delim);
  }


  VertexPtr on_enter_vertex(VertexPtr root, LocalT *local) {
    bool new_force_func_ptr = false;
    if (root->type() == op_func_call || root->type() == op_func_name) {
      string name = get_full_static_member_name(current_function, root->get_string(), root->type() == op_func_call);
      callback->require_function_set(fs_function, name, current_function);
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
        callback->require_function_set(fs_function, resolve_constructor_func_name(current_function, root), current_function);
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

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *local) {
    force_func_ptr = local->saved_force_func_ptr;

    if (root->type() == op_require || root->type() == op_require_once) {
      VertexAdaptor<meta_op_require> require = root;
      for (auto &cur : require->args()) {
        kphp_error_act (cur->type() == op_string, "Not a string in 'require' arguments", continue);
        pair<SrcFilePtr, bool> tmp = callback->require_file(cur->get_string(), "");
        SrcFilePtr file = tmp.first;
        bool required = tmp.second;
        if (required) {
          file->req_id = current_function;
        }

        CREATE_VERTEX (call, op_func_call);
        if (file.not_null()) {
          call->str_val = file->main_func_name;
          cur = call;
        } else {
          kphp_error (0, dl_pstr("Cannot require [%s]\n", cur->get_string().c_str()));
        }
      }
    }

    return root;
  }
};

template<class DataStream>
class CollectRequiredCallback : public CollectRequiredCallbackBase {
private:
  DataStream *os;
public:
  CollectRequiredCallback(DataStream *os) :
    os(os) {
  }

  pair<SrcFilePtr, bool> require_file(const string &file_name, const string &class_context) {
    return G->require_file(file_name, class_context, *os);
  }

  void require_function_set(
    function_set_t type,
    const string &name,
    FunctionPtr by_function) {
    G->require_function_set(type, name, by_function, *os);
  }
};

class CollectRequiredF {
public:
  DUMMY_ON_FINISH

  template<class OutputStream>
  void execute(FunctionPtr function, OutputStream &os) {
    CollectRequiredCallback<OutputStream> callback(&os);
    CollectRequiredPass pass(&callback);

    run_function_pass(function, &pass);

    if (stage::has_error()) {
      return;
    }

    if (function->type() == FunctionData::func_global && !function->class_name.empty() &&
        (function->namespace_name + "\\" + function->class_name) != function->class_context_name) {
      return;
    }
    os << ReadyFunctionPtr(function);
  }
};

/*** Create local variables for switches ***/
class CreateSwitchForeachVarsF : public FunctionPassBase {
private:
  AUTO_PROF (create_switch_vars);

  VertexPtr process_switch(VertexPtr v) {

    VertexAdaptor<op_switch> switch_v = v;

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

  VertexPtr process_foreach(VertexPtr v) {

    VertexAdaptor<op_foreach> foreach_v = v;
    VertexAdaptor<op_foreach_param> foreach_param = foreach_v->params();
    VertexAdaptor<op_var> x = foreach_param->x();
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
      temp_var2->str_val = gen_unique_name("tmp_expr");
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

  VertexPtr on_enter_vertex(VertexPtr v, LocalT *local __attribute__((unused))) {
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

  VertexPtr on_enter_vertex(VertexPtr v, LocalT *local __attribute__((unused))) {
    stage::set_line(v->location.line);
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

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *local __attribute__((unused))) {
    if (root->type() == op_define) {
      defines.push_back(root);
    }
    return root;
  }

  const vector<VertexPtr> &get_vector() {
    return defines;
  }
};

class PreprocessDefinesConcatenationF {
private:
  set<string> in_progress;
  set<string> done;
  map<string, VertexPtr> define_vertex;
  vector<string> stack;

  DataStream<VertexPtr> defines_stream;
  DataStream<FunctionPtr> all_fun;

  CheckConstWithDefines check_const;
  MakeConst make_const;

public:

  PreprocessDefinesConcatenationF() :
    check_const(define_vertex),
    make_const(define_vertex) {
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

  template<class OutputStreamT>
  void execute(FunctionPtr function, OutputStreamT &os __attribute__((unused))) {
    AUTO_PROF (preprocess_defines);
    CollectDefinesToVectorPass pass;
    run_function_pass(function, &pass);

    vector<VertexPtr> vs = pass.get_vector();
    for (size_t i = 0; i < vs.size(); i++) {
      defines_stream << vs[i];
    }
    all_fun << function;
  }

  template<class OutputStreamT>
  void on_finish(OutputStreamT &os) {
    AUTO_PROF (preprocess_defines_finish);
    stage::set_name("Preprocess defines");
    stage::set_file(SrcFilePtr());

    stage::die_if_global_errors();

    vector<VertexPtr> all_defines = defines_stream.get_as_vector();
    for (auto define : all_defines) {
      VertexPtr name_v = define.as<op_define>()->name();
      stage::set_location(define.as<op_define>()->location);
      kphp_error_return (
        name_v->type() == op_string,
        "Define: first parameter must be a string"
      );

      string name = name_v.as<op_string>()->str_val;
      if (!define_vertex.insert(make_pair(name, define)).second) {
        kphp_error_return(0, dl_pstr("Duplicate define declaration: %s", name.c_str()));
      }
    }

    for (auto define : all_defines) {
      process_define(define);
    }

    for (auto fun : all_fun.get_as_vector()) {
      os << fun;
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
    for (auto i : *root) {
      process_define_recursive(i);
    }
  }

  void process_define(VertexPtr root) {
    stage::set_location(root->location);
    VertexAdaptor<meta_op_define> define = root;
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

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *local __attribute__((unused))) {
    if (root->type() == op_define || root->type() == op_define_raw) {
      VertexAdaptor<meta_op_define> define = root;
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
        set_location(var, name->get_location());
        name = var;

        define->value() = VertexPtr();
        CREATE_VERTEX (new_root, op_set, name, val);
        set_location(new_root, root->get_location());
        root = new_root;
      } else {
        def_type = DefineData::def_php;
        CREATE_VERTEX (new_root, op_empty);
        root = new_root;
      }

      DefineData *data = new DefineData(val, def_type);
      data->name = name->get_string();
      data->file_id = stage::get_file();
      DefinePtr def_id(data);

      if (def_type == DefineData::def_var) {
        name->set_string("d$" + name->get_string());
      } else {
        current_function->define_ids.push_back(def_id);
      }

      G->register_define(def_id);
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

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *local __attribute__((unused))) {

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
void function_apply_header(FunctionPtr func, VertexAdaptor<meta_op_function> header) {
  VertexAdaptor<meta_op_function> root = func->root;
  func->used_in_source = true;

  kphp_assert (root.not_null() && header.not_null());
  kphp_error_return (
    func->header.is_null(),
    dl_pstr("Function [%s]: multiple headers", func->name.c_str())
  );
  func->header = header;

  kphp_error_return (
    root->type_rule.is_null(),
    dl_pstr("Function [%s]: type_rule is overided by header", func->name.c_str())
  );
  root->type_rule = header->type_rule;

  kphp_error_return (
    !(!header->varg_flag && func->varg_flag),
    dl_pstr("Function [%s]: varg_flag mismatch with header", func->name.c_str())
  );
  func->varg_flag = header->varg_flag;

  if (!func->varg_flag) {
    VertexAdaptor<op_func_param_list> root_params_vertex = root->params();
    VertexAdaptor<op_func_param_list> header_params_vertex = header->params();
    VertexRange root_params = root_params_vertex->params();
    VertexRange header_params = header_params_vertex->params();

    kphp_error (
      root_params.size() == header_params.size(),
      dl_pstr("Bad header for function [%s]", func->name.c_str())
    );
    int params_n = (int)root_params.size();
    for (int i = 0; i < params_n; i++) {
      kphp_error (
        root_params[i]->size() == header_params[i]->size(),
        dl_pstr(
          "Function [%s]: %dth param has problem with default value",
          func->name.c_str(), i + 1
        )
      );
      kphp_error (
        root_params[i]->type_help == tp_Unknown,
        dl_pstr("Function [%s]: type_help is overrided by header", func->name.c_str())
      );
      root_params[i]->type_help = header_params[i]->type_help;
    }
  }
}

void prepare_function_misc(FunctionPtr func) {
  VertexAdaptor<meta_op_function> func_root = func->root;
  kphp_assert (func_root.not_null());
  VertexAdaptor<op_func_param_list> param_list = func_root->params();
  VertexRange params = param_list->args();
  int param_n = (int)params.size();
  bool was_default = false;
  func->min_argn = param_n;
  for (int i = 0; i < param_n; i++) {
    if (func->varg_flag) {
      kphp_error (params[i].as<meta_op_func_param>()->var()->ref_flag == false,
                  "Reference arguments are not supported in varg functions");
    }
    if (params[i].as<meta_op_func_param>()->has_default()) {
      if (!was_default) {
        was_default = true;
        func->min_argn = i;
      }
      if (func->type() == FunctionData::func_local) {
        kphp_error (params[i].as<meta_op_func_param>()->var()->ref_flag == false,
                    dl_pstr("Default value in reference function argument [function = %s]", func->name.c_str()));
      }
    } else {
      kphp_error (!was_default,
                  dl_pstr("Default value expected [function = %s] [param_i = %d]",
                          func->name.c_str(), i));
    }
  }
}

/*
 * Анализ @kphp-infer, @kphp-inline и других @kphp-* внутри phpdoc над функцией f
 * Сейчас пока что есть костыль: @kphp-required анализируется всё равно в gentree
 */
void parse_and_apply_function_kphp_phpdoc(FunctionPtr f) {
  if (f->phpdoc_token == nullptr || f->phpdoc_token->type() != tok_phpdoc_kphp) {
    return;   // обычный phpdoc, без @kphp нотаций, тут не парсим; если там инстансы, распарсится по требованию
  }

  int infer_type = 0;
  vector<VertexPtr> prepend_cmd;
  VertexPtr func_params = f->root.as<op_function>()->params();

  std::unordered_map<std::string, VertexAdaptor<op_func_param>> name_to_function_param;
  for (auto param_ptr : *func_params) {
    VertexAdaptor<op_func_param> param = param_ptr.as<op_func_param>();
    name_to_function_param.emplace("$" + param->var()->get_string(), param);
  }

  std::size_t id_of_kphp_template = 0;
  stage::set_location(f->root->get_location());
  for (auto &tag : parse_php_doc(f->phpdoc_token->str_val.str())) {
    stage::set_line(tag.line_num);
    switch (tag.type) {
      case php_doc_tag::kphp_inline: {
        f->root->inline_flag = true;
        continue;
      }

      case php_doc_tag::kphp_sync: {
        f->should_be_sync = true;
        continue;
      }

      case php_doc_tag::kphp_infer: {
        kphp_error(infer_type == 0, "Double kphp-infer tag found");
        std::istringstream is(tag.value);
        string token;
        while (is >> token) {
          if (token == "check") {
            infer_type |= 1;
          } else if (token == "hint") {
            infer_type |= 2;
          } else if (token == "cast") {
            infer_type |= 4;
          } else {
            kphp_error(0, dl_pstr("Unknown kphp-infer tag type '%s'", token.c_str()));
          }
        }
        break;
      }

      case php_doc_tag::kphp_disable_warnings: {
        std::istringstream is(tag.value);
        string token;
        while (is >> token) {
          if (!f->disabled_warnings.insert(token).second) {
            kphp_warning(dl_pstr("Warning '%s' has been disabled twice", token.c_str()));
          }
        }
        break;
      }

      case php_doc_tag::kphp_required: {
        f->kphp_required = true;
        break;
      }

      case php_doc_tag::param: {
        if (infer_type) {
          kphp_error_act(!name_to_function_param.empty(), "Too many @param tags", return);
          std::istringstream is(tag.value);
          string type_help, var_name;
          kphp_error(is >> type_help, "Failed to parse @param tag");
          kphp_error(is >> var_name, "Failed to parse @param tag");

          auto func_param_it = name_to_function_param.find(var_name);

          kphp_error_act(func_param_it != name_to_function_param.end(), dl_pstr("@param tag var name mismatch. found %s.", var_name.c_str()), return);

          VertexAdaptor<op_func_param> cur_func_param = func_param_it->second;
          VertexAdaptor<op_var> var = cur_func_param->var().as<op_var>();
          name_to_function_param.erase(func_param_it);

          kphp_error(var.not_null(), "Something strange happened during @param parsing");

          VertexPtr doc_type = phpdoc_parse_type(type_help, f);
          kphp_error(doc_type.not_null(),
                     dl_pstr("Failed to parse type '%s'", type_help.c_str()));
          if (infer_type & 1) {
            CREATE_VERTEX(doc_type_check, op_lt_type_rule, doc_type);
            CREATE_VERTEX(doc_rule_var, op_var);
            doc_rule_var->str_val = var->str_val;
            doc_rule_var->type_rule = doc_type_check;
            set_location(doc_rule_var, f->root->location);
            prepend_cmd.push_back(doc_rule_var);
          }
          if (infer_type & 2) {
            CREATE_VERTEX(doc_type_check, op_common_type_rule, doc_type);
            CREATE_VERTEX(doc_rule_var, op_var);
            doc_rule_var->str_val = var->str_val;
            doc_rule_var->type_rule = doc_type_check;
            set_location(doc_rule_var, f->root->location);
            prepend_cmd.push_back(doc_rule_var);
          }
          if (infer_type & 4) {
            kphp_error(doc_type->type() == op_type_rule,
                       dl_pstr("Too hard rule '%s' for cast", type_help.c_str()));
            kphp_error(doc_type.as<op_type_rule>()->args().empty(),
                       dl_pstr("Too hard rule '%s' for cast", type_help.c_str()));
            kphp_error(cur_func_param->type_help == tp_Unknown,
                       dl_pstr("Duplicate type rule for argument '%s'", var_name.c_str()));
            cur_func_param->type_help = doc_type.as<op_type_rule>()->type_help;
          }
        }
        break;
      }

      case php_doc_tag::kphp_template: {
        f->is_template = true;
        for (const auto &var_name : split_skipping_delimeters(tag.value, ", ")) {
          if (var_name[0] != '$') {
            break;
          }

          auto func_param_it = name_to_function_param.find(var_name);
          kphp_error_return(func_param_it != name_to_function_param.end(), dl_pstr("@kphp-template tag var name mismatch. found %s.", var_name.c_str()));

          VertexAdaptor<op_func_param> cur_func_param = func_param_it->second;
          name_to_function_param.erase(func_param_it);

          cur_func_param->template_type_id = id_of_kphp_template;
        }
        id_of_kphp_template++;

        break;
      }

      default:
        break;
    }
  }
  size_t cnt_left_params_without_tag = f->is_instance_function() && !f->is_constructor() ? 1 : 0;  // пропускаем неявный this
  if (infer_type && name_to_function_param.size() != cnt_left_params_without_tag) {
    std::string err_msg = "Not enough @param tags. Need tags for function arguments:\n";
    for (auto name_and_function_param : name_to_function_param) {
      err_msg += name_and_function_param.first;
      err_msg += "\n";
    }
    stage::set_location(f->root->get_location());
    kphp_error(false, err_msg.c_str());
  }

  // из { cmd } делаем { prepend_cmd; cmd }
  if (!prepend_cmd.empty()) {
    for (auto i : *f->root.as<op_function>()->cmd()) {
      prepend_cmd.push_back(i);
    }
    CREATE_VERTEX(new_cmd, op_seq, prepend_cmd);
    ::set_location(new_cmd, f->root->location);
    f->root.as<op_function>()->cmd() = new_cmd;
  }
}

void prepare_function(FunctionPtr function) {
  parse_and_apply_function_kphp_phpdoc(function);
  prepare_function_misc(function);

  FunctionSetPtr function_set = function->function_set;
  VertexPtr header = function_set->header;
  if (header.not_null()) {
    function_apply_header(function, header);
  }
  if (function->root.not_null() && function->root->varg_flag) {
    function->varg_flag = true;
  }
}

class PrepareFunctionF {
public:
  DUMMY_ON_FINISH

  template<class OutputStream>
  void execute(FunctionPtr function, OutputStream &os) {
    stage::set_name("Prepare function");
    stage::set_function(function);
    kphp_assert (function.not_null());

    prepare_function(function);

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

  bool on_start(FunctionPtr function) {
    if (!FunctionPassBase::on_start(function)) {
      return false;
    }
    return true;
  }

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *local __attribute__((unused))) {
    if (root->type() == op_defined) {
      bool is_defined = false;

      VertexAdaptor<op_defined> defined = root;

      kphp_error_act (
        (int)root->size() == 1 && defined->expr()->type() == op_string,
        "wrong arguments in 'defined'",
        return VertexPtr()
      );

      const string name = defined->expr().as<op_string>()->str_val;
      DefinePtr def = G->get_define(name);
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
      DefinePtr d = G->get_define(name);
      if (d.not_null()) {
        assert (d->name == name);
        if (d->type() == DefineData::def_var) {
          CREATE_VERTEX (var, op_var);
          var->extra_type = op_ex_var_superglobal;
          var->str_val = "d$" + name;
          root = var;
        } else if (d->type() == DefineData::def_raw || d->type() == DefineData::def_php) {
          CREATE_VERTEX (def, op_define_val);
          def->set_define_id(d);
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

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *local __attribute__((unused))) {
    if (root->type() == op_eq3 || root->type() == op_neq3) {
      VertexAdaptor<meta_op_binary_op> eq_op = root;
      VertexPtr a = eq_op->lhs();
      VertexPtr b = eq_op->rhs();

      if (b->type() == op_var || b->type() == op_index) {
        std::swap(a, b);
      }

      if (a->type() == op_var || a->type() == op_index) {
        VertexPtr ra = a;
        while (ra->type() == op_index) {
          ra = ra.as<op_index>()->array();
        }
        bool ok = ra->type() == op_var;
        if (ok) {
          ok &= //(b->type() != op_true && b->type() != op_false) ||
            (ra->get_string() != "connection" &&
             ra->get_string().find("MC") == string::npos);
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
            VertexPtr a_copy = clone_vertex(a);
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
  using InstanceOfFunctionTemplatePtr = FunctionPtr;
  using OStreamT = MultipleDataStreams<FunctionPtr, InstanceOfFunctionTemplatePtr>;
  OStreamT &os;

  explicit PreprocessFunctionCPass(OStreamT &os) :
    os(os) {}

  std::string get_description() override {
    return "Preprocess function C";
  }

  bool check_function(FunctionPtr function) override {
    return default_check_function(function) && function->type() != FunctionData::func_extern && !function->is_template;
  }

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *) {
    if (root->type() == op_function_c) {
      CREATE_VERTEX (new_root, op_string);
      if (stage::get_function_name() != stage::get_file()->main_func_name) {
        new_root->set_string(stage::get_function_name());
      }
      set_location(new_root, root->get_location());
      root = new_root;
    }

    if (root->type() == op_func_call || root->type() == op_func_ptr || root->type() == op_constructor_call) {
      root = try_set_func_id(root);
    }

    return root;
  }

private:

  VertexPtr set_func_id(VertexPtr call, FunctionPtr func) {
    kphp_assert (call->type() == op_func_ptr || call->type() == op_func_call || call->type() == op_constructor_call);
    kphp_assert (func.not_null());
    kphp_assert (call->get_func_id().is_null() || call->get_func_id() == func);
    if (call->get_func_id() == func) {
      return call;
    }
    //fprintf (stderr, "%s\n", func->name.c_str());

    call->set_func_id(func);
    if (call->type() == op_func_ptr) {
      func->is_callback = true;
      return call;
    }

    if (func->root.is_null()) {
      kphp_fail();
      return call;
    }

    VertexAdaptor<meta_op_function> func_root = func->root;
    VertexAdaptor<op_func_param_list> param_list = func_root->params();
    VertexRange call_args = call.as<op_func_call>()->args();
    VertexRange func_args = param_list->params();
    int call_args_n = (int)call_args.size();
    int func_args_n = (int)func_args.size();

    if (func->varg_flag) {
      for (int i = 0; i < call_args_n; i++) {
        kphp_error_act (
          call_args[i]->type() != op_func_name,
          "Unexpected function pointer",
          return VertexPtr()
        );
      }
      VertexPtr args;
      if (call_args_n == 1 && call_args[0]->type() == op_varg) {
        args = call_args[0].as<op_varg>()->expr();
      } else {
        CREATE_VERTEX (new_args, op_array, call->get_next());
        new_args->location = call->get_location();
        args = new_args;
      }
      vector<VertexPtr> tmp(1, GenTree::conv_to<tp_array>(args));
      COPY_CREATE_VERTEX (new_call, call, op_func_call, tmp);
      return new_call;
    }

    std::map<int, std::pair<AssumType, ClassPtr>> template_type_id_to_ClassPtr;
    std::string name_of_function_instance = func->name + "$instance";
    for (int i = 0; i < call_args_n; i++) {
      if (i < func_args_n) {
        if (func_args[i]->type() == op_func_param) {
          if (call_args[i]->type() == op_func_name) {
            string msg = "Unexpected function pointer: " + call_args[i]->get_string();
            kphp_error(false, msg.c_str());
            continue;
          } else if (call_args[i]->type() == op_varg) {
            string msg = "function: `" + func->name + "` must takes variable-length argument list";
            kphp_error_act(false, msg.c_str(), break);
          }
          VertexAdaptor<op_func_param> param = func_args[i];
          if (param->type_help != tp_Unknown) {
            call_args[i] = GenTree::conv_to(call_args[i], param->type_help, param->var()->ref_flag);
          }

          if (param->template_type_id >= 0) {
            kphp_assert(func->is_template);
            ClassPtr class_corresponding_to_parameter;
            AssumType assum = infer_class_of_expr(stage::get_function(), call_args[i], class_corresponding_to_parameter);

            {
              std::string error_msg = "function templates support only instances as argument value; " + func->name + "; " + param->var()
                                                                                                                                 ->get_string() + " argument";
              kphp_error_act(vk::any_of_equal(assum, assum_instance, assum_instance_array), error_msg.c_str(), return {});
            }

            auto insertion_result = template_type_id_to_ClassPtr.emplace(param->template_type_id, std::make_pair(assum, class_corresponding_to_parameter));
            if (!insertion_result.second) {
              const std::pair<AssumType, ClassPtr> &previous_assum_and_class = insertion_result.first->second;
              auto wrap_if_array = [](const std::string &s, AssumType assum) {
                return assum == assum_instance_array ? s + "[]" : s;
              };

              std::string error_msg =
                "argument $" + param->var()->get_string() + " of " + func->name +
                " has a type: `" + wrap_if_array(class_corresponding_to_parameter->name, assum) +
                "` but expected type: `" + wrap_if_array(previous_assum_and_class.second->name, previous_assum_and_class.first) + "`";

              kphp_error_act(previous_assum_and_class.second->name == class_corresponding_to_parameter->name, error_msg.c_str(), return {});
              kphp_error_act(previous_assum_and_class.first == assum, error_msg.c_str(), return {});
            }

            if (assum == assum_instance_array) {
              name_of_function_instance += "$arr";
            }

            name_of_function_instance += "$" + replace_backslashes(class_corresponding_to_parameter->name);
          }
        } else if (func_args[i]->type() == op_func_param_callback) {
          call_args[i] = conv_to_func_ptr(call_args[i], stage::get_function());
          kphp_error (call_args[i]->type() == op_func_ptr, "Function pointer expected");
        } else {
          kphp_fail();
        }
      }
    }

    if (func->is_template) {
      FunctionSetPtr function_set = G->get_function_set(fs_function, name_of_function_instance, true);
      call->set_string(name_of_function_instance);
      call->set_func_id({});

      FunctionPtr new_function;
      {
        std::lock_guard<Lockable> guard{*function_set};

        if (function_set->size() == 0) {
          new_function = generate_instance_of_template_function(template_type_id_to_ClassPtr, func, name_of_function_instance);
          bool added = function_set->add_function(new_function);
          kphp_assert(added);

          function_set->is_required = true;
          new_function->function_set = function_set;
        }
      }

      kphp_assert(function_set->size() == 1);
      set_func_id(call, function_set[0]);

      if (new_function.not_null()) {
        (*os.project_to_nth_data_stream<1>()) << new_function;
      }
    }

    return call;
  }

  FunctionPtr generate_instance_of_template_function(const std::map<int, std::pair<AssumType, ClassPtr>> &template_type_id_to_ClassPtr,
                                                     FunctionPtr func,
                                                     const std::string &name_of_function_instance) {
    VertexAdaptor<op_func_param_list> param_list = func->root.as<meta_op_function>()->params();
    VertexRange func_args = param_list->params();
    size_t func_args_n = func_args.size();

    FunctionPtr new_function(new FunctionData());
    CLONE_VERTEX(new_func_root, op_function, func->root.as<op_function>());

    for (auto id_classPtr_it : template_type_id_to_ClassPtr) {
      const std::pair<AssumType, ClassPtr> &assum_and_class = id_classPtr_it.second;
      for (size_t i = 0; i < func_args_n; ++i) {
        VertexAdaptor<op_func_param> param = func_args[i];
        if (param->template_type_id == id_classPtr_it.first) {
          new_function->assumptions.emplace_back(assum_and_class.first, param->var()->get_string(), assum_and_class.second);
          new_function->assumptions_inited_args = 2;

          new_func_root->params()->ith(i).as<op_func_param>()->template_type_id = -1;
        }
      }
    }

    new_func_root->name()->set_string(name_of_function_instance);

    new_function->root = new_func_root;
    new_function->root->set_func_id(new_function);
    new_function->is_required = true;
    new_function->type() = func->type();
    new_function->file_id = func->file_id;
    new_function->req_id = func->req_id;
    new_function->class_id = func->class_id;
    new_function->varg_flag = func->varg_flag;
    new_function->tinf_state = func->tinf_state;
    new_function->const_data = func->const_data;
    new_function->phpdoc_token = func->phpdoc_token;
    new_function->min_argn = func->min_argn;
    new_function->is_extern = func->is_extern;
    new_function->used_in_source = func->used_in_source;
    new_function->kphp_required = true;
    new_function->namespace_name = func->namespace_name;
    new_function->class_name = func->class_name;
    new_function->class_context_name = func->class_context_name;
    new_function->class_extends = func->class_extends;
    new_function->access_type = func->access_type;
    new_function->namespace_uses = func->namespace_uses;
    new_function->is_template = false;
    new_function->name = name_of_function_instance;

    std::function<void(VertexPtr, FunctionPtr)> set_location_for_all = [&set_location_for_all](VertexPtr root, FunctionPtr function_location) {
      root->location.function = function_location;
      for (VertexPtr &v : *root) {
        set_location_for_all(v, function_location);
      }
    };
    set_location_for_all(new_func_root, new_function);

    return new_function;
  }

  /**
   * Имея vertex вида 'fn(...)' или 'new A(...)', сопоставить этому vertex реальную FunctionPtr
   *  (он будет доступен через vertex->get_func_id()).
   * Вызовы instance-методов вида $a->fn(...) были на уровне gentree преобразованы в op_func_call fn($a, ...),
   * со спец. extra_type, поэтому для таких можно определить FunctionPtr по первому аргументу.
   */
  VertexPtr try_set_func_id(VertexPtr call) {
    if (call->get_func_id().not_null()) {
      return call;
    }

    const string &name =
      call->type() == op_constructor_call
      ? resolve_constructor_func_name(current_function, call)
      : call->type() == op_func_call && call->extra_type == op_ex_func_member
        ? resolve_instance_func_name(current_function, call)
        : call->get_string();

    FunctionSetPtr function_set = G->get_function_set(fs_function, name, true);

    switch (function_set->size()) {
      case 1: {
        if (!function_set->is_required) {
          kphp_error(false, dl_pstr("Function is not required. Maybe you want to use `@kphp-required` for this function [%s]\n%s\n", name.c_str(), stage::get_function_history()
            .c_str()));
          break;
        }
        call = set_func_id(call, function_set[0]);
        break;
      }

      case 0: {
        if (call->type() == op_constructor_call) {
          kphp_error(0, dl_pstr("Calling 'new %s()', but this class does not have fields and constructor\n%s\n",
                                call->get_string().c_str(), stage::get_function_history().c_str()));
        } else {
          kphp_error(0, dl_pstr("Unknown function [%s]\n%s\n",
                                name.c_str(), stage::get_function_history().c_str()));
        }
        break;
      }

      default: {
        kphp_error(false, dl_pstr("Function overloading is not supported properly [%s]", name.c_str()));
        break;
      }
    }

    return call;
  }
};

class PreprocessFunctionF {
public:
  DUMMY_ON_FINISH

  void execute(FunctionPtr function, PreprocessFunctionCPass::OStreamT &os) {
    PreprocessFunctionCPass pass(os);

    pass.init();
    if (pass.on_start(function)) {
      PreprocessFunctionCPass::LocalT local;
      function->root = run_function_pass(function->root, &pass, &local);
      pass.on_finish();
    }

    if (!stage::has_error() && !function->is_template) {
      (*os.project_to_nth_data_stream<0>()) << function;
    }
  }
};

/*** Preprocess 'break 5' and similar nodes. The will be replaced with goto ***/
class PreprocessBreakPass : public FunctionPassBase {
private:
  AUTO_PROF (preprocess_break);
  vector<VertexPtr> cycles;

  int current_label_id;

  int get_label_id(VertexAdaptor<meta_op_cycle> cycle, Operation op) {
    int *val = nullptr;
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
    current_label_id(0) {
    //cycles.reserve (1024);
  }

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *local) {
    local->is_cycle = OpInfo::type(root->type()) == cycle_op;
    if (local->is_cycle) {
      cycles.push_back(root);
    }

    if (root->type() == op_break || root->type() == op_continue) {
      int val;
      VertexAdaptor<meta_op_goto> goto_op = root;
      kphp_error_act (
        goto_op->expr()->type() == op_int_const,
        "Break/continue parameter expected to be constant integer",
        return root
      );
      val = atoi(goto_op->expr()->get_string().c_str());
      kphp_error_act (
        1 <= val && val <= 10,
        "Break/continue parameter expected to be in [1;10] interval",
        return root
      );

      bool force_label = false;
      if (goto_op->type() == op_continue && val == 1 && !cycles.empty()
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
        goto_op->int_val = get_label_id(cycles[cycles_n - val], root->type());
      }
    }

    return root;
  }

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *local) {
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

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *local __attribute__((unused))) {
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
      for (auto i : *old_params) {
        VertexAdaptor<op_func_param> arg = i;
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
        for (auto i : *seq) {
          params_init.push_back(i);
        }
        CREATE_VERTEX(new_seq, op_seq, params_init);
        root.as<op_function>()->cmd() = new_seq;
      }
    }
    return root;
  }

  virtual bool check_function(FunctionPtr function) {
    return function->varg_flag && default_check_function(function);
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
  string get_description() {
    return "Check instance properties";
  }

  VertexPtr on_enter_vertex(VertexPtr v, LocalT *local __attribute__((unused))) {

    if (v->type() == op_instance_prop) {
      ClassPtr klass = resolve_class_of_arrow_access(current_function, v);
      if (klass.not_null()) {   // если null, то ошибка доступа к непонятному свойству уже кинулась в resolve_class_of_arrow_access()
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
  void init_class_instance_var(VertexPtr v, VarPtr var, ClassPtr klass) {
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
      has_nonconst(false) {
    }
  };

  string get_description() {
    return "Calc const types";
  }

  void on_exit_edge(VertexPtr v __attribute__((unused)),
                    LocalT *v_local,
                    VertexPtr from __attribute__((unused)),
                    LocalT *from_local __attribute__((unused))) {
    v_local->has_nonconst |= from->const_type == cnst_nonconst_val;
  }

  VertexPtr on_exit_vertex(VertexPtr v, LocalT *local) {
    switch (OpInfo::cnst(v->type())) {
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
        kphp_error (0, dl_pstr("Unknown cnst-type for [op = %d]", v->type()));
        kphp_fail();
        break;
    }
    return v;
  }
};

/*** Throws flags calculcation ***/
class CalcThrowEdgesPass : public FunctionPassBase {
private:
  vector<FunctionPtr> edges;
public:
  string get_description() {
    return "Collect throw edges";
  }

  VertexPtr on_enter_vertex(VertexPtr v, LocalT *local __attribute__((unused))) {
    if (v->type() == op_throw) {
      current_function->root->throws_flag = true;
    }
    if (v->type() == op_func_call) {
      FunctionPtr from = v->get_func_id();
      kphp_assert (from.not_null());
      edges.push_back(from);
    }
    return v;
  }

  template<class VisitT>
  bool user_recursion(VertexPtr v, LocalT *local __attribute__((unused)), VisitT &visit) {
    if (v->type() == op_try) {
      VertexAdaptor<op_try> try_v = v;
      visit(try_v->catch_cmd());
      return true;
    }
    return false;
  }

  const vector<FunctionPtr> &get_edges() {
    return edges;
  }
};

struct FunctionAndEdges {
  FunctionPtr function;
  vector<FunctionPtr> *edges;

  FunctionAndEdges() :
    function(),
    edges(nullptr) {
  }

  FunctionAndEdges(FunctionPtr function, vector<FunctionPtr> *edges) :
    function(function),
    edges(edges) {
  }

};

class CalcThrowEdgesF {
public:
  DUMMY_ON_FINISH

  template<class OutputStreamT>
  void execute(FunctionPtr function, OutputStreamT &os) {
    AUTO_PROF (calc_throw_edges);
    CalcThrowEdgesPass pass;
    run_function_pass(function, &pass);

    if (stage::has_error()) {
      return;
    }

    os << FunctionAndEdges(function, new vector<FunctionPtr>(pass.get_edges()));
  }
};

static int throws_func_cnt = 0;

void calc_throws_dfs(FunctionPtr from, IdMap<vector<FunctionPtr>> &graph, vector<FunctionPtr> *bt) {
  throws_func_cnt++;
  //FIXME
  if (false && from->header.not_null()) {
    stringstream ss;
    ss << "Extern function [" << from->name << "] throws \n";
    for (int i = (int)bt->size() - 1; i >= 0; i--) {
      ss << "-->[" << bt->at(i)->name << "]";
    }
    ss << "\n";
    kphp_warning (ss.str().c_str());
  }
  bt->push_back(from);
  for (FunctionPtr to : graph[from]) {
    if (!to->root->throws_flag) {
      to->root->throws_flag = true;
      calc_throws_dfs(to, graph, bt);

    }
  }
  bt->pop_back();
}

class CalcThrowsF {
private:
  DataStream<FunctionAndEdges> tmp_stream;
public:
  CalcThrowsF() {
    tmp_stream.set_sink(true);
  }

  template<class OutputStreamT>
  void execute(FunctionAndEdges input, OutputStreamT &os __attribute__((unused))) {
    tmp_stream << input;
  }

  template<class OutputStreamT>
  void on_finish(OutputStreamT &os) {

    mem_info_t mem_info;
    get_mem_stats(getpid(), &mem_info);

    stage::set_name("Calc throw");
    stage::set_file(SrcFilePtr());

    stage::die_if_global_errors();

    AUTO_PROF (calc_throws);

    vector<FunctionPtr> from;

    vector<FunctionAndEdges> all = tmp_stream.get_as_vector();
    int cur_id = 0;
    for (int i = 0; i < (int)all.size(); i++) {
      set_index(&all[i].function, cur_id++);
      if (all[i].function->root->throws_flag) {
        from.push_back(all[i].function);
      }
    }

    IdMap<vector<FunctionPtr>> graph;
    graph.update_size(all.size());
    for (int i = 0; i < (int)all.size(); i++) {
      for (int j = 0; j < (int)all[i].edges->size(); j++) {
        graph[(*all[i].edges)[j]].push_back(all[i].function);
      }
    }

    for (int i = 0; i < (int)from.size(); i++) {
      vector<FunctionPtr> bt;
      calc_throws_dfs(from[i], graph, &bt);
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

  bool check_function(FunctionPtr function) {
    return default_check_function(function) && function->root->type() == op_function;
  }

  bool on_start(FunctionPtr function) {
    if (!FunctionPassBase::on_start(function)) {
      return false;
    }
    return true;
  }

  void check_func_call(VertexPtr call) {
    FunctionPtr f = call->get_func_id();
    kphp_assert (f.not_null());
    kphp_error_return (f->root.not_null(), dl_pstr("Function [%s] undeclared", f->name.c_str()));

    if (call->type() == op_func_ptr) {
      return;
    }

    VertexRange func_params = f->root.as<meta_op_function>()->params().
      as<op_func_param_list>()->params();

    if (f->varg_flag) {
      return;
    }

    VertexRange call_params = call->type() == op_constructor_call ? call.as<op_constructor_call>()->args()
                                                                  : call.as<op_func_call>()->args();
    int func_params_n = (int)func_params.size(), call_params_n = (int)call_params.size();

    kphp_error_return (
      call_params_n >= f->min_argn,
      dl_pstr("Not enough arguments in function [%s:%s] [found %d] [expected at least %d]",
              f->file_id->file_name.c_str(), f->name.c_str(), call_params_n, f->min_argn)
    );

    kphp_error (
      call_params.begin() == call_params.end() || call_params[0]->type() != op_varg,
      dl_pstr(
        "call_user_func_array is used for function [%s:%s]",
        f->file_id->file_name.c_str(), f->name.c_str()
      )
    );

    kphp_error_return (
      func_params_n >= call_params_n,
      dl_pstr(
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
          dl_pstr("Unknown callback function [%s]", func_ptr->name.c_str()),
          continue
        );
        VertexRange cur_params = func_ptr->root.as<meta_op_function>()->params().
          as<op_func_param_list>()->params();
        kphp_error (
          (int)cur_params.size() == func_params[i].as<op_func_param_callback>()->param_cnt,
          "Wrong callback arguments count"
        );
        for (int j = 0; j < (int)cur_params.size(); j++) {
          kphp_error (cur_params[j]->type() == op_func_param,
                      "Callback function with callback parameter");
          kphp_error (cur_params[j].as<op_func_param>()->var()->ref_flag == 0,
                      "Callback function with reference parameter");
        }
      } else {
        kphp_error (call_params[i]->type() != op_func_ptr, "Unexpected function pointer");
      }
    }
  }

  VertexPtr on_enter_vertex(VertexPtr v, LocalT *local __attribute__((unused))) {
    if (v->type() == op_func_ptr || v->type() == op_func_call || v->type() == op_constructor_call) {
      check_func_call(v);
    }
    return v;
  }
};

/*** RL ***/
void rl_calc(VertexPtr root, RLValueType expected_rl_type);

class CalcRLF {
public:
  DUMMY_ON_FINISH

  template<class OutputStream>
  void execute(FunctionPtr function, OutputStream &os) {
    AUTO_PROF (calc_rl);
    stage::set_name("Calc RL");
    stage::set_function(function);

    rl_calc(function->root, val_none);

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

  vector<VertexPtr> uninited_vars;
  vector<VarPtr> todo_var;
  vector<vector<vector<VertexPtr>>> todo_parts;
public:
  void set_function(FunctionPtr new_function) {
    function = new_function;
  }

  void split_var(VarPtr var, vector<vector<VertexPtr>> &parts) {
    assert (var->type() == VarData::var_local_t || var->type() == VarData::var_param_t);
    int parts_size = (int)parts.size();
    if (parts_size == 0) {
      if (var->type() == VarData::var_local_t) {
        function->local_var_ids.erase(
          std::find(
            function->local_var_ids.begin(),
            function->local_var_ids.end(),
            var));
      }
      return;
    }
    assert (parts_size > 1);

    for (int i = 0; i < parts_size; i++) {
      string new_name = var->name + "$v_" + int_to_str(i);
      VarPtr new_var = G->create_var(new_name, var->type());

      for (int j = 0; j < (int)parts[i].size(); j++) {
        VertexPtr v = parts[i][j];
        v->set_var_id(new_var);
      }

      VertexRange params = function->root.
        as<meta_op_function>()->params().
                                     as<op_func_param_list>()->args();
      if (var->type() == VarData::var_local_t) {
        new_var->type() = VarData::var_local_t;
        function->local_var_ids.push_back(new_var);
      } else if (var->type() == VarData::var_param_t) {
        bool was_var = std::find(
          parts[i].begin(),
          parts[i].end(),
          params[var->param_i].as<op_func_param>()->var()
        ) != parts[i].end();

        if (was_var) { //union of part that contains function argument
          new_var->type() = VarData::var_param_t;
          new_var->param_i = var->param_i;
          new_var->init_val = var->init_val;
          function->param_ids[var->param_i] = new_var;
        } else {
          new_var->type() = VarData::var_local_t;
          function->local_var_ids.push_back(new_var);
        }
      } else {
        kphp_fail();
      }

    }

    if (var->type() == VarData::var_local_t) {
      vector<VarPtr>::iterator tmp = std::find(function->local_var_ids.begin(), function->local_var_ids.end(), var);
      if (function->local_var_ids.end() != tmp) {
        function->local_var_ids.erase(tmp);
      } else {
        kphp_fail();
      }
    }

    todo_var.push_back(var);

    //it could be simple std::move
    todo_parts.push_back(vector<vector<VertexPtr>>());
    std::swap(todo_parts.back(), parts);
  }

  void unused_vertices(vector<VertexPtr *> &v) {
    for (auto i: v) {
      CREATE_VERTEX (empty, op_empty);
      *i = empty;
    }
  }

  FunctionPtr get_function() {
    return function;
  }

  void uninited(VertexPtr v) {
    if (v.not_null() && v->type() == op_var && v->extra_type != op_ex_var_superlocal && v->extra_type != op_ex_var_this) {
      uninited_vars.push_back(v);
      v->get_var_id()->set_uninited_flag(true);
    }
  }

  void check_uninited() {
    for (int i = 0; i < (int)uninited_vars.size(); i++) {
      VertexPtr v = uninited_vars[i];
      VarPtr var = v->get_var_id();
      if (tinf::get_type(v)->ptype() == tp_var) {
        continue;
      }

      stage::set_location(v->get_location());
      kphp_warning (dl_pstr("Variable [%s] may be used uninitialized", var->name.c_str()));
    }
  }

  VarPtr merge_vars(vector<VarPtr> vars, const string &new_name) {
    VarPtr new_var = G->create_var(new_name, VarData::var_unknown_t);;
    //new_var->tinf = vars[0]->tinf; //hack, TODO: fix it
    new_var->tinf_node.copy_type_from(tinf::get_type(vars[0]));

    int param_i = -1;
    for (VarPtr var : vars) {
      if (var->type() == VarData::var_param_t) {
        param_i = var->param_i;
      } else if (var->type() == VarData::var_local_t) {
        //FIXME: remember to remove all unused variables
        //func->local_var_ids.erase (*i);
        vector<VarPtr>::iterator tmp = std::find(function->local_var_ids.begin(), function->local_var_ids.end(), var);
        if (function->local_var_ids.end() != tmp) {
          function->local_var_ids.erase(tmp);
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
      function->local_var_ids.push_back(new_var);
    }

    return new_var;
  }


  struct MergeData {
    int id;
    VarPtr var;

    MergeData(int id, VarPtr var) :
      id(id),
      var(var) {
    }
  };

  static bool cmp_merge_data(const MergeData &a, const MergeData &b) {
    return type_out(tinf::get_type(a.var)) <
           type_out(tinf::get_type(b.var));
  }

  static bool eq_merge_data(const MergeData &a, const MergeData &b) {
    return type_out(tinf::get_type(a.var)) ==
           type_out(tinf::get_type(b.var));
  }

  void merge_same_type() {
    int todo_n = (int)todo_parts.size();
    for (int todo_i = 0; todo_i < todo_n; todo_i++) {
      vector<vector<VertexPtr>> &parts = todo_parts[todo_i];

      int n = (int)parts.size();
      vector<MergeData> to_merge;
      for (int i = 0; i < n; i++) {
        to_merge.push_back(MergeData(i, parts[i][0]->get_var_id()));
      }
      sort(to_merge.begin(), to_merge.end(), cmp_merge_data);

      vector<int> ids;
      int merge_id = 0;
      for (int i = 0; i <= n; i++) {
        if (i == n || (i > 0 && !eq_merge_data(to_merge[i - 1], to_merge[i]))) {
          vector<VarPtr> vars;
          for (int id : ids) {
            vars.push_back(parts[id][0]->get_var_id());
          }
          string new_name = vars[0]->name;
          int name_i = (int)new_name.size() - 1;
          while (new_name[name_i] != '$') {
            name_i--;
          }
          new_name.erase(name_i);
          new_name += "$v";
          new_name += int_to_str(merge_id++);

          VarPtr new_var = merge_vars(vars, new_name);
          for (int id : ids) {
            for (VertexPtr v : parts[id]) {
              v->set_var_id(new_var);
            }
          }

          ids.clear();
        }
        if (i == n) {
          break;
        }
        ids.push_back(to_merge[i].id);
      }
    }
  }
};

struct FunctionAndCFG {
  FunctionPtr function;
  CFGCallback *callback;

  FunctionAndCFG() :
    function(),
    callback(nullptr) {
  }

  FunctionAndCFG(FunctionPtr function, CFGCallback *callback) :
    function(function),
    callback(callback) {
  }
};

class CheckReturnsF {
public:
  DUMMY_ON_FINISH

  template<class OutputStream>
  void execute(FunctionAndCFG function_and_cfg, OutputStream &os) {
    CheckReturnsPass pass;
    run_function_pass(function_and_cfg.function, &pass);
    if (stage::has_error()) {
      return;
    }
    os << function_and_cfg;
  }
};

class CFGBeginF {
public:
  DUMMY_ON_FINISH

  template<class OutputStream>
  void execute(FunctionPtr function, OutputStream &os) {
    AUTO_PROF (CFG);
    stage::set_name("Calc control flow graph");
    stage::set_function(function);

    cfg::CFG cfg;
    CFGCallback *callback = new CFGCallback();
    callback->set_function(function);
    cfg.run(callback);

    if (stage::has_error()) {
      return;
    }

    os << FunctionAndCFG(function, callback);
  }
};

/*** Type inference ***/
class CollectMainEdgesCallback : public CollectMainEdgesCallbackBase {
private:
  tinf::TypeInferer *inferer_;

public:
  CollectMainEdgesCallback(tinf::TypeInferer *inferer) :
    inferer_(inferer) {
  }

  tinf::Node *node_from_rvalue(const RValue &rvalue) {
    if (rvalue.node == nullptr) {
      kphp_assert (rvalue.type != nullptr);
      return new tinf::TypeNode(rvalue.type);
    }

    return rvalue.node;
  }

  virtual void require_node(const RValue &rvalue) {
    if (rvalue.node != nullptr) {
      inferer_->add_node(rvalue.node);
    }
  }

  virtual void create_set(const LValue &lvalue, const RValue &rvalue) {
    tinf::Edge *edge = new tinf::Edge();
    edge->from = lvalue.value;
    edge->from_at = lvalue.key;
    edge->to = node_from_rvalue(rvalue);
    inferer_->add_edge(edge);
    inferer_->add_node(edge->from);
  }

  virtual void create_less(const RValue &lhs, const RValue &rhs) {
    tinf::Node *a = node_from_rvalue(lhs);
    tinf::Node *b = node_from_rvalue(rhs);
    inferer_->add_node(a);
    inferer_->add_node(b);
    inferer_->add_restriction(new RestrictionLess(a, b));
  }

  virtual void create_isset_check(const RValue &rvalue) {
    tinf::Node *a = node_from_rvalue(rvalue);
    inferer_->add_node(a);
    inferer_->add_restriction(new RestrictionIsset(a));
  }
};

class TypeInfererF {
  //TODO: extract pattern
private:
public:
  TypeInfererF() {
    tinf::register_inferer(new tinf::TypeInferer());
  }

  template<class OutputStreamT>
  void execute(FunctionAndCFG input, OutputStreamT &os) {
    AUTO_PROF (tinf_infer_gen_dep);
    CollectMainEdgesCallback callback(tinf::get_inferer());
    CollectMainEdgesPass pass(&callback);
    run_function_pass(input.function, &pass);
    os << input;
  }

  template<class OutputStreamT>
  void on_finish(OutputStreamT &os __attribute__((unused))) {
    //FIXME: rebalance Queues
    vector<Task *> tasks = tinf::get_inferer()->get_tasks();
    for (int i = 0; i < (int)tasks.size(); i++) {
      register_async_task(tasks[i]);
    }
  }
};

class TypeInfererEndF {
private:
  DataStream<FunctionAndCFG> tmp_stream;
public:
  TypeInfererEndF() {
    tmp_stream.set_sink(true);
  }

  template<class OutputStreamT>
  void execute(FunctionAndCFG input, OutputStreamT &os __attribute__((unused))) {
    tmp_stream << input;
  }

  template<class OutputStreamT>
  void on_finish(OutputStreamT &os) {
    tinf::get_inferer()->check_restrictions();
    tinf::get_inferer()->finish();

    for (auto &f_and_cfg : tmp_stream.get_as_vector()) {
      os << f_and_cfg;
    }
  }
};

/*** Control flow graph. End ***/
class CFGEndF {
public:
  DUMMY_ON_FINISH

  template<class OutputStream>
  void execute(FunctionAndCFG data, OutputStream &os) {
    AUTO_PROF (CFG_End);
    stage::set_name("Control flow graph. End");
    stage::set_function(data.function);
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

  bool check_function(FunctionPtr function) {
    return default_check_function(function) && function->type() != FunctionData::func_extern;
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
      case op_conv_uint:
      case op_conv_long:
      case op_conv_ulong:
        // case op_conv_bool нет намеренно, чтобы f$boolval(OrFalse<T>) не превращался в дефолтный T

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

  void on_enter_edge(VertexPtr vertex __attribute__((unused)), LocalT *local, VertexPtr dest_vertex, LocalT *dest_local __attribute__((unused))) {
    if (local->allowed && dest_vertex->rl_type != val_none && dest_vertex->rl_type != val_error) {
      const TypeData *tp = tinf::get_type(dest_vertex);

      if (tp->ptype() != tp_Unknown && tp->use_or_false()) {
        dest_vertex->val_ref_flag = dest_vertex->rl_type;
      }
    }
  }

  template<class VisitT>
  bool user_recursion(VertexPtr v, LocalT *local, VisitT &visit) {
    for (int i = 0; i < v->size(); ++i) {
      bool is_last_elem = (i == v->size() - 1);

      local->allowed = is_allowed_for_getting_val_or_ref(v->type(), is_last_elem);
      visit(v->ith(i));
    }
    return true;
  }
};

class CalcBadVarsF {
private:
  DataStream<pair<FunctionPtr, DepData *>> tmp_stream;
public:
  CalcBadVarsF() {
    tmp_stream.set_sink(true);
  }

  template<class OutputStreamT>
  void execute(FunctionPtr function, OutputStreamT &os __attribute__((unused))) {
    CalcFuncDepPass pass;
    run_function_pass(function, &pass);
    DepData *data = new DepData();
    swap(*data, *pass.get_data_ptr());
    tmp_stream << std::make_pair(function, data);
  }


  template<class OutputStreamT>
  void on_finish(OutputStreamT &os) {
    stage::die_if_global_errors();

    AUTO_PROF (calc_bad_vars);
    stage::set_name("Calc bad vars (for UB check)");
    vector<pair<FunctionPtr, DepData *>> tmp_vec = tmp_stream.get_as_vector();
    CalcBadVars{}.run(tmp_vec);
    for (const auto &fun_dep : tmp_vec) {
      delete fun_dep.second;
      os << fun_dep.first;
    }
  }
};

/*** C++ undefined behaviour fixes ***/
class CheckUBF {
public:
  DUMMY_ON_FINISH;

  template<class OutputStream>
  void execute(FunctionPtr function, OutputStream &os) {
    AUTO_PROF (check_ub);
    stage::set_name("Check for undefined behaviour");
    stage::set_function(function);

    if (function->root->type() == op_function) {
      fix_undefined_behaviour(function);
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

  template<class OutputStream>
  void execute(FunctionPtr function, OutputStream &os) {
    AUTO_PROF (check_ub);
    stage::set_name("Try to detect common errors");
    stage::set_function(function);

    if (function->root->type() == op_function) {
      analyze_foreach(function);
      analyze_common(function);
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

  void skip_conv_and_sets(VertexPtr *&replace) {
    while (true) {
      if (!replace) {
        break;
      }
      Operation op = (*replace)->type();
      if (op == op_set_add || op == op_set_sub ||
          op == op_set_mul || op == op_set_div ||
          op == op_set_mod || op == op_set_and ||
          op == op_set_or || op == op_set_xor ||
          op == op_set_dot || op == op_set ||
          op == op_set_shr || op == op_set_shl) {
        replace = &((*replace).as<meta_op_binary_op>()->rhs());
      } else if (op == op_conv_int || op == op_conv_bool ||
                 op == op_conv_int_l || op == op_conv_float ||
                 op == op_conv_string || op == op_conv_array ||
                 op == op_conv_array_l || op == op_conv_var ||
                 op == op_conv_uint || op == op_conv_long ||
                 op == op_conv_ulong || op == op_conv_regexp ||
                 op == op_log_not) {
        replace = &((*replace).as<meta_op_unary_op>()->expr());
      } else {
        break;
      }
    }
  }

public:
  string get_description() {
    return "Extract easy resumable calls";
  }

  bool check_function(FunctionPtr function) {
    return default_check_function(function) && function->type() != FunctionData::func_extern &&
           function->root->resumable_flag;
  }

  struct LocalT : public FunctionPassBase::LocalT {
    bool from_seq;
  };

  void on_enter_edge(VertexPtr vertex, LocalT *local __attribute__((unused)), VertexPtr dest_vertex __attribute__((unused)), LocalT *dest_local) {
    dest_local->from_seq = vertex->type() == op_seq || vertex->type() == op_seq_rval;
  }

  VertexPtr on_enter_vertex(VertexPtr vertex, LocalT *local) {
    if (local->from_seq == false) {
      return vertex;
    }
    VertexPtr *replace = nullptr;
    VertexAdaptor<op_func_call> func_call;
    Operation op = vertex->type();
    if (op == op_return) {
      replace = &vertex.as<op_return>()->expr();
    } else if (op == op_set_add || op == op_set_sub ||
               op == op_set_mul || op == op_set_div ||
               op == op_set_mod || op == op_set_and ||
               op == op_set_or || op == op_set_xor ||
               op == op_set_dot || op == op_set ||
               op == op_set_shr || op == op_set_shl) {
      replace = &vertex.as<meta_op_binary_op>()->rhs();
      if ((*replace)->type() == op_func_call && op == op_set) {
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
    VarPtr var = G->create_local_var(stage::get_function(), temp_var->str_val, VarData::var_local_t);
    var->tinf_node.copy_type_from(tinf::get_type(func, -1));
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

  bool check_function(FunctionPtr function) {
    return default_check_function(function) && function->type() != FunctionData::func_extern &&
           function->root->resumable_flag;
  }

  struct LocalT : public FunctionPassBase::LocalT {
    bool from_seq;
  };

  void on_enter_edge(VertexPtr vertex, LocalT *local __attribute__((unused)), VertexPtr dest_vertex __attribute__((unused)), LocalT *dest_local) {
    dest_local->from_seq = vertex->type() == op_seq || vertex->type() == op_seq_rval;
  }

  VertexPtr on_enter_vertex(VertexPtr vertex, LocalT *local) {
    if (local->from_seq == false) {
      return vertex;
    }
    VertexAdaptor<op_func_call> func_call;
    VertexPtr lhs;
    if (vertex->type() == op_func_call) {
      func_call = vertex;
    } else if (vertex->type() == op_set) {
      VertexAdaptor<op_set> set = vertex;
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
      set_location(empty, vertex->get_location());
      lhs = empty;
    }
    CREATE_VERTEX (async, op_async, lhs, func_call);
    set_location(async, func_call->get_location());
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

  bool on_start(FunctionPtr function) {
    if (!FunctionPassBase::on_start(function)) {
      return false;
    }
    return function->type() != FunctionData::func_extern;
  }

  VertexPtr on_enter_vertex(VertexPtr vertex, LocalT *local __attribute__((unused))) {
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
      const TypeData *type_left = tinf::get_type(VertexAdaptor<meta_op_binary_op>(vertex)->lhs());
      const TypeData *type_right = tinf::get_type(VertexAdaptor<meta_op_binary_op>(vertex)->rhs());
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
      const TypeData *type_left = tinf::get_type(VertexAdaptor<meta_op_binary_op>(vertex)->lhs());
      const TypeData *type_right = tinf::get_type(VertexAdaptor<meta_op_binary_op>(vertex)->rhs());
      if ((type_left->ptype() == tp_array) ^ (type_right->ptype() == tp_array)) {
        if (type_left->ptype() != tp_var && type_right->ptype() != tp_var) {
          kphp_warning (dl_pstr("%s + %s is strange operation",
                                type_out(type_left).c_str(),
                                type_out(type_right).c_str()));
        }
      }
    }
    if (vertex->type() == op_sub || vertex->type() == op_mul || vertex->type() == op_div || vertex->type() == op_mod) {
      const TypeData *type_left = tinf::get_type(VertexAdaptor<meta_op_binary_op>(vertex)->lhs());
      const TypeData *type_right = tinf::get_type(VertexAdaptor<meta_op_binary_op>(vertex)->rhs());
      if ((type_left->ptype() == tp_array) || (type_right->ptype() == tp_array)) {
        kphp_warning (dl_pstr("%s %s %s is strange operation",
                              OpInfo::str(vertex->type()).c_str(),
                              type_out(type_left).c_str(),
                              type_out(type_right).c_str()));
      }
    }

    if (vertex->type() == op_foreach) {
      VertexPtr arr = vertex.as<op_foreach>()->params().as<op_foreach_param>()->xs();
      const TypeData *arrayType = tinf::get_type(arr);
      if (arrayType->ptype() == tp_array) {
        const TypeData *valueType = arrayType->lookup_at(Key::any_key());
        if (valueType->get_real_ptype() == tp_Unknown) {
          kphp_error (0, "Can not compile foreach on array of Unknown type");
        }
      }
    }
    if (vertex->type() == op_list) {
      VertexPtr arr = vertex.as<op_list>()->array();
      const TypeData *arrayType = tinf::get_type(arr);
      if (arrayType->ptype() == tp_array) {
        const TypeData *valueType = arrayType->lookup_at(Key::any_key());
        if (valueType->get_real_ptype() == tp_Unknown) {
          kphp_error (0, "Can not compile list with array of Unknown type");
        }
      }
    }
    if (vertex->type() == op_unset || vertex->type() == op_isset) {
      for (auto v : vertex.as<meta_op_xset>()->args()) {
        if (v->type() == op_var) {    // isset($var), unset($var)
          VarPtr var = v.as<op_var>()->get_var_id();
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
        } else if (v->type() == op_index) {   // isset($arr[index]), unset($arr[index])
          const TypeData *arrayType = tinf::get_type(v.as<op_index>()->array());
          PrimitiveType ptype = arrayType->get_real_ptype();
          kphp_error(ptype == tp_array || ptype == tp_var, "Can't use isset/unset by[idx] for not an array");
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

    if (G->env().get_warnings_level() >= 1 && vk::any_of_equal(vertex->type(), op_require, op_require_once)) {
      FunctionPtr function_where_require = stage::get_function();

      if (function_where_require.not_null() && function_where_require->type() == FunctionData::func_local) {
        for (auto v : *vertex) {
          FunctionPtr function_which_required = v->get_func_id();

          kphp_assert(function_which_required.not_null());
          kphp_assert(function_which_required->type() == FunctionData::func_global);

          for (VarPtr global_var : function_which_required->global_var_ids) {
            if (!global_var->marked_as_global) {
              kphp_warning(("require file with global variable not marked as global: " + global_var->name).c_str());
            }
          }
        }
      }
    }
    //TODO: may be this should be moved to tinf_check
    return vertex;
  }

  template<class VisitT>
  bool user_recursion(VertexPtr v, LocalT *local __attribute__((unused)), VisitT &visit) {
    if (v->type() == op_function) {
      visit(v.as<op_function>()->cmd());
      return true;
    }

    if (v->type() == op_func_call || v->type() == op_var ||
        v->type() == op_index || v->type() == op_constructor_call) {
      if (v->rl_type == val_r) {
        const TypeData *type = tinf::get_type(v);
        // пока что, т.к. все методы всех классов считаются required, в реально неиспользуемых будет Unknown
        // (потом когда-нибудь можно убирать реально неиспользуемые из required-списка, и убрать дополнительное условие)
        if (type->get_real_ptype() == tp_Unknown && !current_function->is_instance_function()) {
          string index_depth = "";
          while (v->type() == op_index) {
            v = v.as<op_index>()->array();
            index_depth += "[.]";
          }
          string desc = "Using Unknown type : ";
          if (v->type() == op_var) {
            VarPtr var = v->get_var_id();
            desc += "variable [$" + var->name + "]" + index_depth;

            FunctionPtr holder_func = var->holder_func;
            if (holder_func.not_null() && holder_func->is_required && holder_func->kphp_required) {
              desc += dl_pstr("\nMaybe because `@kphp-required` is set for function `%s` but it has never been used", holder_func->name.c_str());
            }

          } else if (v->type() == op_func_call) {
            desc += "function [" + v.as<op_func_call>()->get_func_id()->name + "]" + index_depth;
          } else if (v->type() == op_constructor_call) {
            desc += "constructor [" + v.as<op_constructor_call>()->get_func_id()->name + "]" + index_depth;
          } else {
            desc += "...";
          }

          kphp_error (0, desc.c_str());
          return true;
        }
      }
    }

    return false;
  }

  VertexPtr on_exit_vertex(VertexPtr vertex, LocalT *local __attribute__((unused))) {
    if (vertex->type() == op_return) {
      from_return--;
    }
    return vertex;
  }
};

// todo почему-то удалилось из server
void prepare_generate_class(ClassPtr klass __attribute__((unused))) {
}

template<class OutputStream>
class WriterCallback : public WriterCallbackBase {
private:
  OutputStream &os;
public:
  WriterCallback(OutputStream &os, const string dir __attribute__((unused)) = "./") :
    os(os) {
  }

  void on_end_write(WriterData *data) {
    if (stage::has_error()) {
      return;
    }

    WriterData *data_copy = new WriterData();
    data_copy->swap(*data);
    data_copy->calc_crc();
    os << data_copy;
  }
};

class CodeGenF {
  //TODO: extract pattern
private:
  DataStream<FunctionPtr> tmp_stream;
  map<string, long long> subdir_hash;

public:
  CodeGenF() {
    tmp_stream.set_sink(true);
  }

  template<class OutputStreamT>
  void execute(FunctionPtr input, OutputStreamT &os __attribute__((unused))) {
    tmp_stream << input;
  }

  int calc_count_of_parts(size_t cnt_global_vars) {
    return cnt_global_vars > 1000 ? 64 : 1;
  }

  template<class OutputStreamT>
  void on_finish(OutputStreamT &os) {
    AUTO_PROF (code_gen);

    stage::set_name("GenerateCode");
    stage::set_file(SrcFilePtr());
    stage::die_if_global_errors();

    vector<FunctionPtr> xall = tmp_stream.get_as_vector();
    sort(xall.begin(), xall.end());
    const vector<ClassPtr> &all_classes = G->get_classes();

    //TODO: delete W_ptr
    CodeGenerator *W_ptr = new CodeGenerator();
    CodeGenerator &W = *W_ptr;

    if (G->env().get_use_safe_integer_arithmetic()) {
      W.use_safe_integer_arithmetic();
    }

    G->init_dest_dir();
    G->load_index();

    W.init(new WriterCallback<OutputStreamT>(os));

    //TODO: parallelize;
    for (const auto &fun : xall) {
      prepare_generate_function(fun);
    }
    for (vector<ClassPtr>::const_iterator c = all_classes.begin(); c != all_classes.end(); ++c) {
      if (c->not_null() && !(*c)->is_fully_static()) {
        prepare_generate_class(*c);
      }
    }

    vector<SrcFilePtr> main_files = G->get_main_files();
    vector<FunctionPtr> all_functions;
    vector<FunctionPtr> source_functions;
    for (int i = 0; i < (int)xall.size(); i++) {
      FunctionPtr function = xall[i];
      if (function->used_in_source) {
        source_functions.push_back(function);
      }
      if (function->type() == FunctionData::func_extern) {
        continue;
      }
      all_functions.push_back(function);
      W << Async(FunctionH(function));
      W << Async(FunctionCpp(function));
    }

    for (vector<ClassPtr>::const_iterator c = all_classes.begin(); c != all_classes.end(); ++c) {
      if (c->not_null() && !(*c)->is_fully_static()) {
        W << Async(ClassDeclaration(*c));
      }
    }

    //W << Async (XmainCpp());
    W << Async(InitScriptsH());
    for (const auto &main_file : main_files) {
      W << Async(DfsInit(main_file));
    }
    W << Async(InitScriptsCpp(/*std::move*/main_files, source_functions, all_functions));

    vector<VarPtr> vars = G->get_global_vars();
    int parts_cnt = calc_count_of_parts(vars.size());
    W << Async(VarsCpp(vars, parts_cnt));

    write_hashes_of_subdirs_to_dep_files(W);

    write_tl_schema(W);
  }

private:
  void prepare_generate_function(FunctionPtr func) {
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

    my_unique(&func->static_var_ids);
    my_unique(&func->global_var_ids);
    my_unique(&func->header_global_var_ids);
    my_unique(&func->local_var_ids);
  }

  string get_subdir(const string &base) {
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
    for (const auto &dir_and_hash : subdir_hash) {
      W << OpenFile("_dep.cpp", dir_and_hash.first, false);
      char tmp[100];
      sprintf(tmp, "%llx", dir_and_hash.second);
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

  template<class OutputStreamT>
  void execute(WriterData *data, OutputStreamT &os __attribute__((unused))) {
    AUTO_PROF (end_write);
    stage::set_name("Write files");
    string dir = G->cpp_dir;

    string cur_file_name = data->file_name;
    string cur_subdir = data->subdir;

    string full_file_name = dir;
    if (!cur_subdir.empty()) {
      full_file_name += cur_subdir;
      full_file_name += "/";
    }
    full_file_name += cur_file_name;

    File *file = G->get_file_info(full_file_name);
    file->needed = true;
    file->includes = data->get_includes();
    file->compile_with_debug_info_flag = data->compile_with_debug_info();

    if (file->on_disk) {
      if (file->crc64 == (unsigned long long)-1) {
        FILE *old_file = fopen(full_file_name.c_str(), "r");
        dl_passert (old_file != nullptr,
                    dl_pstr("Failed to open [%s]", full_file_name.c_str()));
        unsigned long long old_crc = 0;
        unsigned long long old_crc_with_comments = static_cast<unsigned long long>(-1);

        if (fscanf(old_file, "//crc64:%Lx", &old_crc) != 1) {
          kphp_warning (dl_pstr("can't read crc64 from [%s]\n", full_file_name.c_str()));
          old_crc = static_cast<unsigned long long>(-1);
        } else {
          if (fscanf(old_file, " //crc64_with_comments:%Lx", &old_crc_with_comments) != 1) {
            old_crc_with_comments = static_cast<unsigned long long>(-1);
          }
        }
        fclose(old_file);

        file->crc64 = old_crc;
        file->crc64_with_comments = old_crc_with_comments;
      }
    }

    bool need_del = false;
    bool need_fix = false;
    bool need_save_time = false;
    unsigned long long crc = data->calc_crc();
    string code_str;
    data->dump(code_str);
    unsigned long long crc_with_comments = compute_crc64(code_str.c_str(), code_str.length());
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
        fprintf(stderr, "File [%s] changed\n", full_file_name.c_str());
      }
      if (need_del) {
        int err = unlink(full_file_name.c_str());
        dl_passert (err == 0, dl_pstr("Failed to unlink [%s]", full_file_name.c_str()));
      }
      FILE *dest_file = fopen(full_file_name.c_str(), "w");
      dl_passert (dest_file != nullptr,
                  dl_pstr("Failed to open [%s] for write\n", full_file_name.c_str()));

      dl_pcheck (fprintf(dest_file, "//crc64:%016Lx\n", ~crc));
      dl_pcheck (fprintf(dest_file, "//crc64_with_comments:%016Lx\n", ~crc_with_comments));
      dl_pcheck (fprintf(dest_file, "%s", code_str.c_str()));
      dl_pcheck (fflush(dest_file));
      dl_pcheck (fseek(dest_file, 0, SEEK_SET));
      dl_pcheck (fprintf(dest_file, "//crc64:%016Lx\n", crc));
      dl_pcheck (fprintf(dest_file, "//crc64_with_comments:%016Lx\n", crc_with_comments));

      dl_pcheck (fflush(dest_file));
      dl_pcheck (fclose(dest_file));

      file->crc64 = crc;
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

  template<class OutputStreamT>
  void execute(ReadyFunctionPtr ready_data, OutputStreamT &os) {
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
                   dl_pstr("Invalid class extends %s and %s: extends is available only if classes are only-static", klass->name.c_str(), klass->parent_class
                                                                                                                                              ->name
                                                                                                                                              .c_str()));
      } else {
        klass->parent_class = ClassPtr();
      }
    }
    os << data;
  }
};

/*
 * Для всех функций, всех переменных проверяем, что если делались предположения насчёт классов, то они совпали с выведенными.
 * Также анализируем переменные-члены инстансов, как они вывелись.
 */
class CheckInferredInstances {
public:
  DUMMY_ON_FINISH

  template<class OutputStream>
  void execute(FunctionPtr function, OutputStream &os) {
    stage::set_name("Check inferred instances");
    stage::set_function(function);

    if (function->type() != FunctionData::func_extern && !function->assumptions.empty()) {
      analyze_function_vars(function);
    }
    if (function->class_id.not_null() && function->class_id->init_function.ptr == function.ptr) {
      analyze_class(function->class_id);
    }

    if (stage::has_error()) {
      return;
    }

    os << function;
  }

  void analyze_function_vars(FunctionPtr function) {
    auto analyze_vars = [this, function](const std::vector<VarPtr> &vars) {
      for (const auto &var: vars) {
        analyze_function_var(function, var);
      }
    };

    analyze_vars(function->local_var_ids);
    analyze_vars(function->global_var_ids);
    analyze_vars(function->static_var_ids);
  }

  inline void analyze_function_var(FunctionPtr function, VarPtr var) {
    ClassPtr klass;
    AssumType assum = assumption_get(function, var->name, klass);

    if (assum == assum_instance) {
      const TypeData *t = var->tinf_node.get_type();
      kphp_error((t->ptype() == tp_Class && klass.ptr == t->class_type().ptr)
                 || (t->ptype() == tp_Exception || t->ptype() == tp_MC),
                 dl_pstr("var $%s assumed to be %s, but inferred %s", var->name.c_str(), klass->name.c_str(), type_out(t).c_str()));
    } else if (assum == assum_instance_array) {
      const TypeData *t = var->tinf_node.get_type()->lookup_at(Key::any_key());
      kphp_error(t != nullptr && ((t->ptype() == tp_Class && klass.ptr == t->class_type().ptr)
                               || (t->ptype() == tp_Exception || t->ptype() == tp_MC)),
                 dl_pstr("var $%s assumed to be %s[], but inferred %s", var->name.c_str(), klass->name.c_str(), type_out(var->tinf_node.get_type()).c_str()));
    }
  }

  void analyze_class(ClassPtr klass) {
    // при несовпадении phpdoc и выведенного типов — ошибка уже кинулась раньше, на этапе type inferring
    // а здесь мы проверим переменные классов, которые объявлены, но никогда не записывались и не имеют дефолтного значения
    for (auto var : klass->vars) {
      PrimitiveType ptype = var->tinf_node.get_type()->get_real_ptype();
      kphp_error(ptype != tp_Unknown,
                 dl_pstr("var %s::$%s is declared but never written", klass->name.c_str(), var->name.c_str()));
    }
  }
};

class lockf_wrapper {
  std::string locked_filename_;
  int fd_ = -1;
  bool locked_ = false;

  void close_and_unlink() {
    if (close(fd_) == -1) {
      perror(("Can't close file: " + locked_filename_).c_str());
      return;
    }

    if (unlink(locked_filename_.c_str()) == -1) {
      perror(("Can't unlink file: " + locked_filename_).c_str());
      return;
    }
  }

public:
  bool lock() {
    std::string dest_path = G->env().get_dest_dir();
    if (G->env().get_use_auto_dest()) {
      dest_path += G->get_subdir_name();
    }

    std::stringstream ss;
    ss << std::hex << hash(dest_path) << "_kphp_temp_lock";
    locked_filename_ = ss.str();

    fd_ = open(locked_filename_.c_str(), O_RDWR | O_CREAT, 0666);
    assert(fd_ != -1);

    locked_ = true;
    if (lockf(fd_, F_TLOCK, 0) == -1) {
      perror("\nCan't lock file, maybe you have already run kphp2cpp");
      fprintf(stderr, "\n");

      close(fd_);
      locked_ = false;
    }

    return locked_;
  }

  ~lockf_wrapper() {
    if (locked_) {
      if (lockf(fd_, F_ULOCK, 0) == -1) {
        perror(("Can't unlock file: " + locked_filename_).c_str());
      }

      close_and_unlink();
      locked_ = false;
    }
  }
};

template<class PipeFunctionT, class InputStreamT, class OutputStreamT>
using PipeDataStream = Pipe<PipeFunctionT, DataStream<InputStreamT>, DataStream<OutputStreamT>>;

template<class Pass>
using FunctionPassPipe = PipeDataStream<FunctionPassF<Pass>, FunctionPtr, FunctionPtr>;

bool compiler_execute(KphpEnviroment *env) {
  double st = dl_time();
  G = new CompilerCore();
  G->register_env(env);
  G->start();
  if (!env->get_warnings_filename().empty()) {
    FILE *f = fopen(env->get_warnings_filename().c_str(), "w");
    if (!f) {
      fprintf(stderr, "Can't open warnings-file %s\n", env->get_warnings_filename().c_str());
      return false;
    }

    env->set_warnings_file(f);
  } else {
    env->set_warnings_file(nullptr);
  }
  if (!env->get_stats_filename().empty()) {
    FILE *f = fopen(env->get_stats_filename().c_str(), "w");
    if (!f) {
      fprintf(stderr, "Can't open stats-file %s\n", env->get_stats_filename().c_str());
      return false;
    }
    env->set_stats_file(f);
  } else {
    env->set_stats_file(nullptr);
  }

  //TODO: call it with pthread_once on need
  lexer_init();
  gen_tree_init();
  OpInfo::init_static();
  MultiKey::init_static();
  TypeData::init_static();
//  PhpDocTypeRuleParser::run_tipa_unit_tests_parsing_tags(); return true;

  DataStream<SrcFilePtr> file_stream;

  for (const auto &main_file : env->get_main_files()) {
    G->register_main_file(main_file, file_stream);
  }

  static lockf_wrapper unique_file_lock;
  if (!unique_file_lock.lock()) {
    return false;
  }

  {
    SchedulerBase *scheduler;
    if (G->env().get_threads_count() == 1) {
      scheduler = new OneThreadScheduler();
    } else {
      auto s = new Scheduler();
      s->set_threads_count(G->env().get_threads_count());
      scheduler = s;
    }


    PipeDataStream<LoadFileF, SrcFilePtr, SrcFilePtr> load_file_pipe;
    PipeDataStream<FileToTokensF, SrcFilePtr, FileAndTokens> file_to_tokens_pipe;
    PipeDataStream<ParseF, FileAndTokens, FunctionPtr> parse_pipe;
    PipeDataStream<ApplyBreakFileF, FunctionPtr, FunctionPtr> apply_break_file_pipe;
    PipeDataStream<SplitSwitchF, FunctionPtr, FunctionPtr> split_switch_pipe;
    FunctionPassPipe<CreateSwitchForeachVarsF> create_switch_foreach_vars_pipe;

    Pipe<CollectRequiredF,
      DataStream<FunctionPtr>,
      MultipleDataStreams<ReadyFunctionPtr, SrcFilePtr, FunctionPtr>> collect_required_pipe;

    PipeDataStream<CollectClassF, ReadyFunctionPtr, FunctionPtr> collect_classes_pipe;
    FunctionPassPipe<CalcLocationsPass> calc_locations_pipe;
    PipeDataStream<PreprocessDefinesConcatenationF, FunctionPtr, FunctionPtr> process_defines_concat;
    FunctionPassPipe<CollectDefinesPass> collect_defines_pipe;
    PipeDataStream<PrepareFunctionF, FunctionPtr, FunctionPtr> prepare_function_pipe;
    FunctionPassPipe<RegisterDefinesPass> register_defines_pipe;

    FunctionPassPipe<PreprocessVarargPass> preprocess_vararg_pipe;
    FunctionPassPipe<PreprocessEq3Pass> preprocess_eq3_pipe;

    Pipe<PreprocessFunctionF,
      DataStream<FunctionPtr>,
      PreprocessFunctionCPass::OStreamT> preprocess_function_c_pipe;

    FunctionPassPipe<PreprocessBreakPass> preprocess_break_pipe;
    FunctionPassPipe<CalcConstTypePass> calc_const_type_pipe;
    FunctionPassPipe<CollectConstVarsPass> collect_const_vars_pipe;
    FunctionPassPipe<CheckInstanceProps> check_instance_props_pipe;
    FunctionPassPipe<ConvertListAssignmentsPass> convert_list_assignments_pipe;
    FunctionPassPipe<RegisterVariables> register_variables_pipe;

    PipeDataStream<CalcThrowEdgesF, FunctionPtr, FunctionAndEdges> calc_throw_edges_pipe;
    PipeDataStream<CalcThrowsF, FunctionAndEdges, FunctionPtr> calc_throws_pipe;
    FunctionPassPipe<CheckFunctionCallsPass> check_func_calls_pipe;
    PipeDataStream<CalcRLF, FunctionPtr, FunctionPtr> calc_rl_pipe;
    PipeDataStream<CFGBeginF, FunctionPtr, FunctionAndCFG> cfg_begin_pipe;
    PipeDataStream<CheckReturnsF, FunctionAndCFG, FunctionAndCFG> check_returns_pipe;
    PipeDataStream<TypeInfererF, FunctionAndCFG, FunctionAndCFG> type_inferer_pipe;
    PipeDataStream<TypeInfererEndF, FunctionAndCFG, FunctionAndCFG> type_inferer_end_pipe;
    PipeDataStream<CFGEndF, FunctionAndCFG, FunctionPtr> cfg_end_pipe;
    PipeDataStream<CheckInferredInstances, FunctionPtr, FunctionPtr> check_inferred_instances_pipe;
    FunctionPassPipe<OptimizationPass> optimization_pipe;
    FunctionPassPipe<CalcValRefPass> calc_val_ref_pipe;

    PipeDataStream<CalcBadVarsF, FunctionPtr, FunctionPtr> calc_bad_vars_pipe;
    PipeDataStream<CheckUBF, FunctionPtr, FunctionPtr> check_ub_pipe;
    FunctionPassPipe<ExtractResumableCallsPass> extract_resumable_calls_pipe;
    FunctionPassPipe<ExtractAsyncPass> extract_async_pipe;
    PipeDataStream<AnalyzerF, FunctionPtr, FunctionPtr> analyzer_pipe;
    FunctionPassPipe<CheckAccessModifiers> check_access_modifiers_pass;
    FunctionPassPipe<FinalCheckPass> final_check_pass;
    PipeDataStream<CodeGenF, FunctionPtr, WriterData *> code_gen_pipe;
    Pipe<WriteFilesF, DataStream<WriterData *>, EmptyStream> write_files_pipe(false);


    load_file_pipe.set_input_stream(&file_stream);

    scheduler_constructor(scheduler, load_file_pipe)
      >>
      file_to_tokens_pipe >>
      parse_pipe >>
      apply_break_file_pipe >>
      split_switch_pipe >>
      create_switch_foreach_vars_pipe >>
      collect_required_pipe >> use_nth_output_tag<0>{} >> sync_node_tag{} >>
      collect_classes_pipe >>
      calc_locations_pipe >>
      process_defines_concat >> use_previous_pipe_as_sync_node_tag{} >>
      collect_defines_pipe >> sync_node_tag{} >>
      prepare_function_pipe >> sync_node_tag{} >>
      register_defines_pipe >>
      preprocess_vararg_pipe >>
      preprocess_eq3_pipe >>
      // functions which were generated from templates
      // need to be preprocessed therefore we tie second output and input of Pipe
      preprocess_function_c_pipe >> use_nth_output_tag<1>{} >>
      preprocess_function_c_pipe >> use_nth_output_tag<0>{} >>
      preprocess_break_pipe >>
      calc_const_type_pipe >>
      collect_const_vars_pipe >>
      check_instance_props_pipe >>
      convert_list_assignments_pipe >>
      register_variables_pipe >>
      calc_throw_edges_pipe >>
      calc_throws_pipe >> use_previous_pipe_as_sync_node_tag{} >>
      check_func_calls_pipe >>
      calc_rl_pipe >>
      cfg_begin_pipe >>
      check_returns_pipe >> sync_node_tag{} >>
      type_inferer_pipe >> use_previous_pipe_as_sync_node_tag{} >>
      type_inferer_end_pipe >> use_previous_pipe_as_sync_node_tag{} >>
      cfg_end_pipe >>
      check_inferred_instances_pipe >>
      optimization_pipe >>
      calc_val_ref_pipe >>
      calc_bad_vars_pipe >> use_previous_pipe_as_sync_node_tag{} >>
      check_ub_pipe >>
      extract_resumable_calls_pipe >>
      extract_async_pipe >>
      analyzer_pipe >>
      check_access_modifiers_pass >>
      final_check_pass >>
      code_gen_pipe >> use_previous_pipe_as_sync_node_tag{} >>
      write_files_pipe;

    scheduler_constructor(scheduler, collect_required_pipe) >> use_nth_output_tag<1>{} >> load_file_pipe;
    scheduler_constructor(scheduler, collect_required_pipe) >> use_nth_output_tag<2>{} >> apply_break_file_pipe;

    get_scheduler()->execute();
  }

  if (G->env().get_error_on_warns() && stage::warnings_count > 0) {
    stage::error();
  }

  stage::die_if_global_errors();
  int verbosity = G->env().get_verbosity();
  if (G->env().get_use_make()) {
    fprintf(stderr, "start make\n");
    G->make();
  }
  G->finish();
  if (verbosity > 1) {
    profiler_print_all();
    double en = dl_time();
    double passed = en - st;
    fprintf(stderr, "PASSED: %lf\n", passed);
    mem_info_t mem_info;
    get_mem_stats(getpid(), &mem_info);
    fprintf(stderr, "RSS: %lluKb\n", mem_info.rss);
  }
  return true;
}
