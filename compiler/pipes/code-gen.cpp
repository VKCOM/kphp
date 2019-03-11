#include <unordered_map>

#include "compiler/pipes/code-gen.h"

#include "common/algorithms/find.h"
#include "common/mixin/not_copyable.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/define-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"
#include "compiler/gentree.h"
#include "compiler/inferring/public.h"
#include "compiler/io.h"
#include "compiler/scheduler/scheduler-base.h"
#include "compiler/scheduler/task.h"
#include "compiler/vertex.h"

struct CGContext {
  vector<string> catch_labels;
  vector<int> catch_label_used;
  FunctionPtr parent_func;
  bool use_safe_integer_arithmetic{false};
  bool resumable_flag{false};
  bool namespace_opened{false};
};

struct PlainCode;

class WriterCallback : public WriterCallbackBase {
private:
  DataStream<WriterData *> &os;
public:
  WriterCallback(DataStream<WriterData *> &os, const string dir __attribute__((unused)) = "./") :
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


class CodeGenerator {
private:
  TLS<Writer> *master_writer;
  Writer *writer;
  WriterCallbackBase *callback_;
  CGContext context;
  bool own_flag;
public:

  CodeGenerator() :
    master_writer(nullptr),
    writer(nullptr),
    callback_(nullptr),
    context(),
    own_flag(false) {
  }

  CodeGenerator(const CodeGenerator &from) :
    master_writer(from.master_writer),
    writer(nullptr),
    callback_(from.callback_),
    context(from.context),
    own_flag(false) {
  }

  void init(WriterCallbackBase *new_callback) {
    master_writer = new TLS<Writer>();
    callback_ = new_callback;
    own_flag = true;
  }

  void clear() {
    if (own_flag) {
      delete master_writer;
      delete callback_;
      own_flag = false;
    }
    master_writer = nullptr;
    callback_ = nullptr;
  }

  ~CodeGenerator() {
    clear();
  }

  void use_safe_integer_arithmetic(bool flag = true) {
    context.use_safe_integer_arithmetic = flag;
  }

  WriterCallbackBase *callback() {
    return callback_;
  }

  inline void lock_writer() {
    assert (writer == nullptr);
    writer = master_writer->lock_get();
  }

  inline void unlock_writer() {
    assert (writer != nullptr);
    master_writer->unlock_get(writer);
    writer = nullptr;
  }


  inline CodeGenerator &operator<<(const char *s);
  inline CodeGenerator &operator<<(char c);
  inline CodeGenerator &operator<<(const string &s);
  inline CodeGenerator &operator<<(const vk::string_view &s);

  template<Operation Op>
  inline CodeGenerator &operator<<(VertexAdaptor<Op> vertex);
  template<class T>
  CodeGenerator &operator<<(const T &value);

  inline Writer &get_writer();
  inline CGContext &get_context();
};

inline void compile_vertex(VertexPtr, CodeGenerator &W);

#define NL NewLine()
#define BEGIN OpenBlock()
#define END CloseBlock()

template<class T>
struct AsyncImpl {
  const T &cmd;
  AsyncImpl(const T &cmd);
  void compile(CodeGenerator &W) const;
};

template<class T>
inline AsyncImpl<T> Async(const T &cmd);

struct LockComments {
  inline void compile(CodeGenerator &W) const;
};

struct UnlockComments {
  inline void compile(CodeGenerator &W) const;
};

struct OpenFile {
  string file_name;
  string subdir;
  bool compile_with_debug_info_flag;
  bool compile_with_crc;
  inline OpenFile(const string &file_name, const string &subdir = "",
                  bool compile_with_debug_info_flag = true, bool compile_with_crc = true);
  inline void compile(CodeGenerator &W) const;
};

struct CloseFile {
  inline void compile(CodeGenerator &W) const;
};

struct UpdateLocation {
  const Location &location;
  inline UpdateLocation(const Location &location);
  inline void compile(CodeGenerator &W) const;
};

struct NewLine {
  inline void compile(CodeGenerator &W) const;
};

struct Indent {
  int val;
  Indent(int val);
  inline void compile(CodeGenerator &W) const;
};

struct PlainCode {
  vk::string_view str;
  inline PlainCode(const char *s);
  inline PlainCode(const string &s);
  inline PlainCode(const vk::string_view &s);
  inline void compile(CodeGenerator &W) const;
};

struct OpenBlock {
  inline void compile(CodeGenerator &W) const;
};

struct CloseBlock {
  inline void compile(CodeGenerator &W) const;
};

struct OpenNamespace {
  inline OpenNamespace();
  inline OpenNamespace(const string &ns);

  inline void compile(CodeGenerator &W) const;

private:
  const string &ns_;
};

struct CloseNamespace {
  inline void compile(CodeGenerator &W) const;
};

struct ExternInclude {
  inline explicit ExternInclude(const PlainCode &plain_code);
  inline void compile(CodeGenerator &W) const;

protected:
  const PlainCode &plain_code_;
};

struct Include : private ExternInclude {
  using ExternInclude::ExternInclude;
  inline void compile(CodeGenerator &W) const;
};

struct LibInclude : private ExternInclude {
  using ExternInclude::ExternInclude;
  inline void compile(CodeGenerator &W) const;
};

struct IncludesCollector {
public:
  void add_function_body_depends(const FunctionPtr &function);
  void add_function_signature_depends(const FunctionPtr &function);
  void add_var_signature_depends(const VarPtr &var);

  void add_class_include(const ClassPtr &klass);

  void compile(CodeGenerator &W) const;

  void start_next_block();

private:
  void add_all_class_types(const TypeData &tinf_type);

  std::unordered_set<ClassPtr> classes_;
  std::set<std::string> internal_headers_;
  std::set<std::string> lib_headers_;

  std::unordered_set<ClassPtr> prev_classes_;
  std::set<std::string> prev_headers_;
};

struct FunctionName {
  FunctionPtr function;
  inline FunctionName(FunctionPtr function);
  inline void compile(CodeGenerator &W) const;
};

struct FunctionClassName {
  FunctionPtr function;
  inline FunctionClassName(FunctionPtr function);
  inline void compile(CodeGenerator &W) const;
};

struct FunctionForkName {
  FunctionPtr function;
  inline FunctionForkName(FunctionPtr function);
  inline void compile(CodeGenerator &W) const;
};


struct VarName {
  VarPtr var;
  inline VarName(VarPtr var);
  inline void compile(CodeGenerator &W) const;
};

enum class gen_out_style {
  cpp,
  txt
};

struct TypeName {
  const TypeData *type;
  gen_out_style style;
  inline TypeName(const TypeData *type, gen_out_style style = gen_out_style::cpp);
  inline void compile(CodeGenerator &W) const;
};

struct TypeNameInsideMacro {
  const TypeData *type;
  inline TypeNameInsideMacro(const TypeData *type);
  inline void compile(CodeGenerator &W) const;
};

struct FunctionCallFlag {
  FunctionPtr function;
  inline FunctionCallFlag(FunctionPtr function);
  inline void compile(CodeGenerator &W) const;
};

struct FunctionDeclaration {
  FunctionPtr function;
  bool in_header;
  gen_out_style style;
  inline FunctionDeclaration(FunctionPtr function, bool in_header = false,
                             gen_out_style style = gen_out_style::cpp);
  inline void compile(CodeGenerator &W) const;
};

struct FunctionForkDeclaration {
  FunctionPtr function;
  bool in_header;
  inline FunctionForkDeclaration(FunctionPtr function, bool in_header = false);
  inline void compile(CodeGenerator &W) const;
};

struct FunctionParams {
  FunctionPtr function;
  VertexRange params;
  bool in_header;
  gen_out_style style;
  size_t shift;

  inline FunctionParams(FunctionPtr function, bool in_header = false,
                        gen_out_style style = gen_out_style::cpp);

  inline FunctionParams(FunctionPtr function, size_t shift, bool in_header = false,
                        gen_out_style style = gen_out_style::cpp);

  inline void compile(CodeGenerator &W) const;

private:
  inline void declare_cpp_param(CodeGenerator &W, VertexPtr var, const TypeName &type) const;
  inline void declare_txt_param(CodeGenerator &W, VertexPtr var, const TypeName &type) const;
};

struct ClassDeclaration {
  ClassPtr klass;
  inline ClassDeclaration(ClassPtr klass);
  inline void compile(CodeGenerator &W) const;
};

struct VarDeclaration {
  VarPtr var;
  bool extern_flag;
  bool defval_flag;
  inline VarDeclaration(VarPtr var, bool extern_flag = false, bool defval_flag = true);
  inline void compile(CodeGenerator &W) const;
};

struct Function {
  FunctionPtr function;
  bool in_header;
  inline Function(FunctionPtr function, bool in_header = false);
  inline void compile(CodeGenerator &W) const;
};

VarDeclaration VarExternDeclaration(VarPtr var);
VarDeclaration VarPlainDeclaration(VarPtr var);

struct Operand {
  VertexPtr root;
  Operation parent_type;
  bool is_left;
  inline Operand(VertexPtr root, Operation parent_type, bool is_left);
  inline void compile(CodeGenerator &W) const;
};

struct LabelName {
  int label_id;
  inline LabelName(int label_id);
  inline void compile(CodeGenerator &W) const;
};

struct Label {
  int label_id;
  inline Label(int label_id);
  inline void compile(CodeGenerator &W) const;
};

struct AsList {
  VertexPtr root;
  string delim;
  inline AsList(VertexPtr root, string delim);
  inline void compile(CodeGenerator &W) const;
};

struct AsSeq {
  VertexPtr root;
  inline AsSeq(VertexPtr root);
  inline void compile(CodeGenerator &W) const;
};

struct TupleGetIndex {
  VertexPtr tuple;
  std::string int_index;
  inline TupleGetIndex(VertexPtr tuple, const std::string &int_index);
  inline TupleGetIndex(VertexPtr tuple, VertexPtr key);
  inline void compile(CodeGenerator &W) const;
};

struct CycleBody {
  VertexPtr body;
  int continue_label_id;
  int break_label_id;
  inline CycleBody(VertexPtr body, int continue_label_id, int break_label_id);
  inline void compile(CodeGenerator &W) const;
};

struct VertexCompiler {
  VertexPtr vertex;
  inline VertexCompiler(VertexPtr vertex);
  inline void compile(CodeGenerator &W) const;
};

struct InitVar {
  VarPtr var;
  inline InitVar(VarPtr var);
  inline void compile(CodeGenerator &W) const;
};

struct StaticInit {
  const vector<FunctionPtr> &all_functions;
  inline StaticInit(const vector<FunctionPtr> &all_functions);
  inline void compile(CodeGenerator &W) const;
};

struct LibGlobalVarsReset {
  const FunctionPtr &main_function;
  inline LibGlobalVarsReset(const FunctionPtr &main_function);
  inline void compile(CodeGenerator &W) const;
};

struct StaticLibraryRunGlobal {
  gen_out_style style;
  inline StaticLibraryRunGlobal(gen_out_style style);
  inline void compile(CodeGenerator &W) const;
};

struct StaticLibraryRunGlobalHeaderH {
  inline void compile(CodeGenerator &W) const;
};

struct LibHeaderH {
  FunctionPtr exported_function;
  inline LibHeaderH(FunctionPtr exported_function);
  inline void compile(CodeGenerator &W) const;
};

struct LibHeaderTxt {
  vector<FunctionPtr> exported_functions;
  inline LibHeaderTxt(vector<FunctionPtr> &&exported_functions);
  inline void compile(CodeGenerator &W) const;
};

struct InitScriptsCpp {
  vector<SrcFilePtr> main_file_ids;
  vector<FunctionPtr> all_functions;
  inline InitScriptsCpp(vector<SrcFilePtr> &&main_file_ids, vector<FunctionPtr> &&all_functions);
  inline void compile(CodeGenerator &W) const;
};

struct RunFunction {
  FunctionPtr function;
  inline RunFunction(FunctionPtr function);
  inline void compile(CodeGenerator &W) const;
};

struct GlobalResetFunction {
  FunctionPtr function;
  inline GlobalResetFunction(FunctionPtr function);
  inline void compile(CodeGenerator &W) const;
};

struct VarsCpp {
  inline VarsCpp(std::vector<VarPtr> &&vars, size_t parts_cnt);
  inline void compile(CodeGenerator &W) const;

private:
  std::vector<VarPtr> vars_;
  size_t parts_cnt_;
};

struct GlobalVarsReset {
  inline GlobalVarsReset(SrcFilePtr main_file);

  inline void compile(CodeGenerator &W) const;

  static inline void compile_part(FunctionPtr func, const std::set<VarPtr> &used_vars, int part_i, CodeGenerator &W);

  static inline void compile_func(FunctionPtr func, const std::set<FunctionPtr> &used_functions, int parts_n, CodeGenerator &W);

  static inline void collect_used_funcs_and_vars(
    FunctionPtr func, std::set<FunctionPtr> *visited_functions,
    std::set<VarPtr> *used_vars, int used_vars_cnt);

  static inline void declare_extern_for_init_val(VertexPtr v, std::set<VarPtr> &externed_vars, CodeGenerator &W);

private:
  SrcFilePtr main_file_;
};

struct GlobalVarsResetFuncName {
  explicit GlobalVarsResetFuncName(FunctionPtr main_func, int part = -1);

  void compile(CodeGenerator &W) const;

private:
  const FunctionPtr main_func_;
  const int part_{-1};
};

struct FunctionH {
  FunctionPtr function;
  inline FunctionH(FunctionPtr function);
  inline void compile(CodeGenerator &W) const;
};

struct FunctionCpp {
  FunctionPtr function;
  inline FunctionCpp(FunctionPtr function);
  inline void compile(CodeGenerator &W) const;
};

inline void compile_prefix_op(VertexAdaptor<meta_op_unary> root, CodeGenerator &W);
inline void compile_postfix_op(VertexAdaptor<meta_op_unary> root, CodeGenerator &W);
inline void compile_conv_op(VertexAdaptor<meta_op_unary> root, CodeGenerator &W);
inline void compile_noerr(VertexAdaptor<op_noerr> root, CodeGenerator &W);
inline void compile_binary_func_op(VertexAdaptor<meta_op_binary> root, CodeGenerator &W);
inline void compile_binary_op(VertexAdaptor<meta_op_binary> root, CodeGenerator &W);
inline void compile_ternary_op(VertexAdaptor<op_ternary> root, CodeGenerator &W);
inline void compile_if(VertexAdaptor<op_if> root, CodeGenerator &W);
inline void compile_while(VertexAdaptor<op_while> root, CodeGenerator &W);
inline void compile_do(VertexAdaptor<op_do> root, CodeGenerator &W);
inline void compile_require(VertexPtr root, CodeGenerator &W);
inline void compile_return(VertexAdaptor<op_return> root, CodeGenerator &W);
inline void compile_for(VertexAdaptor<op_for> root, CodeGenerator &W);
inline void compile_throw_action(CodeGenerator &W);
inline void compile_throw(VertexAdaptor<op_throw> root, CodeGenerator &W);
inline void compile_try(VertexAdaptor<op_try> root, CodeGenerator &W);
inline void compile_fork(VertexAdaptor<op_fork> root, CodeGenerator &W);
inline void compile_async(VertexAdaptor<op_async> root, CodeGenerator &W);
inline void compile_foreach(VertexAdaptor<op_foreach> root, CodeGenerator &W);
inline void compile_switch_str(VertexAdaptor<op_switch> root, CodeGenerator &W);
inline void compile_switch_int(VertexAdaptor<op_switch> root, CodeGenerator &W);
inline void compile_switch_var(VertexAdaptor<op_switch> root, CodeGenerator &W);
inline void compile_switch(VertexAdaptor<op_switch> root, CodeGenerator &W);
inline void compile_function(VertexAdaptor<op_function> root, CodeGenerator &W);
inline void compile_index(VertexAdaptor<op_index> root, CodeGenerator &W);
inline void compile_index_of_array(VertexAdaptor<op_index> root, CodeGenerator &W);
inline void compile_index_of_string(VertexAdaptor<op_index> root, CodeGenerator &W);
inline void compile_seq_rval(VertexPtr root, CodeGenerator &W);
inline void compile_as_printable(VertexPtr root, CodeGenerator &W);
inline void compile_echo(VertexPtr root, CodeGenerator &W);
inline void compile_var_dump(VertexPtr root, CodeGenerator &W);
inline void compile_print(VertexAdaptor<op_print> root, CodeGenerator &W);
inline void compile_xset(VertexAdaptor<meta_op_xset> root, CodeGenerator &W);
inline void compile_list(VertexAdaptor<op_list> root, CodeGenerator &W);
inline void compile_array(VertexAdaptor<op_array> root, CodeGenerator &W);
inline void compile_tuple(VertexAdaptor<op_tuple> root, CodeGenerator &W);
inline void compile_func_call_fast(VertexAdaptor<op_func_call> root, CodeGenerator &W);
inline void compile_func_call(VertexAdaptor<op_func_call> root, CodeGenerator &W, int state = 0);
inline void compile_func_ptr(VertexAdaptor<op_func_ptr> root, CodeGenerator &W);
inline void compile_define_val(VertexPtr root, CodeGenerator &W);
inline void compile_defined(VertexPtr root, CodeGenerator &W);
inline void compile_safe_version(VertexPtr root, CodeGenerator &W);
inline void compile_set_value(VertexAdaptor<op_set_value> root, CodeGenerator &W);
inline void compile_push_back(VertexAdaptor<op_push_back> root, CodeGenerator &W);
inline void compile_push_back_return(VertexAdaptor<op_push_back_return> root, CodeGenerator &W);
void compile_string_raw(const string &str, CodeGenerator &W);
inline void compile_string_raw(VertexAdaptor<op_string> root, CodeGenerator &W);
inline void compile_string(VertexAdaptor<op_string> root, CodeGenerator &W);
inline void compile_string_build(VertexAdaptor<op_string_build> root, CodeGenerator &W);
inline void compile_break_continue(VertexAdaptor<meta_op_goto> root, CodeGenerator &W);
inline void compile_conv_array_l(VertexAdaptor<op_conv_array_l> root, CodeGenerator &W);
inline void compile_conv_int_l(VertexAdaptor<op_conv_int_l> root, CodeGenerator &W);
inline void compile_cycle_op(VertexPtr root, CodeGenerator &W);
inline void compile_min_max(VertexPtr root, CodeGenerator &W);
inline void compile_power(VertexAdaptor<op_pow> power, CodeGenerator &W);
inline void compile_common_op(VertexPtr root, CodeGenerator &W);
inline void compile(VertexPtr root, CodeGenerator &W);

/*** Implementation ***/
inline CodeGenerator &CodeGenerator::operator<<(const char *s) {
  get_writer().append(s);
  return *this;
}

inline CodeGenerator &CodeGenerator::operator<<(char c) {
  get_writer().append(c);
  return *this;
}

inline CodeGenerator &CodeGenerator::operator<<(const string &s) {
  get_writer().append(s);
  return *this;
}

inline CodeGenerator &CodeGenerator::operator<<(const vk::string_view &s) {
  get_writer().append(s);
  return *this;
}

template<Operation Op>
inline CodeGenerator &CodeGenerator::operator<<(VertexAdaptor<Op> vertex) {
  return (*this) << VertexCompiler(vertex);
}

template<class T>
CodeGenerator &CodeGenerator::operator<<(const T &value) {
  value.compile(*this);
  return *this;
}

inline Writer &CodeGenerator::get_writer() {
  assert (writer != nullptr);
  return *writer;
}

inline CGContext &CodeGenerator::get_context() {
  return context;
}

template<class T>
inline AsyncImpl<T>::AsyncImpl(const T &cmd) :
  cmd(cmd) {
}

static ProfilerRaw &get_code_gen_profiler() {
  static CachedProfiler profiler{"writer"};
  return *profiler;
}

template<class T>
class CodeGenTask : public Task {
private:
  CodeGenerator W;
  T cmd;
public:
  CodeGenTask(CodeGenerator &W, const T &cmd) :
    W(W),
    cmd(cmd) {
  }

  void execute() {
    AutoProfiler profler{get_code_gen_profiler()};
    stage::set_name("Async code generation");
    W << cmd;
  }
};

template<class T>
inline CodeGenTask<T> *create_async_task(CodeGenerator &W, const T &cmd) {
  return new CodeGenTask<T>(W, cmd);
}

template<class T>
inline void AsyncImpl<T>::compile(CodeGenerator &W) const {
  Task *task = create_async_task(W, cmd);
  register_async_task(task);
}

template<class T>
inline AsyncImpl<T> Async(const T &cmd) {
  return AsyncImpl<T>(cmd);
}

inline void LockComments::compile(CodeGenerator &W) const {
  W.get_writer().lock_comments();
}

inline void UnlockComments::compile(CodeGenerator &W) const {
  W.get_writer().unlock_comments();
}

inline OpenFile::OpenFile(const string &file_name, const string &subdir,
                          bool compile_with_debug_info_flag, bool compile_with_crc) :
  file_name(file_name),
  subdir(subdir),
  compile_with_debug_info_flag(compile_with_debug_info_flag),
  compile_with_crc(compile_with_crc) {
}

inline void OpenFile::compile(CodeGenerator &W) const {
  W.lock_writer();
  W.get_writer().set_callback(W.callback());
  W.get_writer().begin_write(compile_with_debug_info_flag, compile_with_crc);
  W.get_writer().set_file_name(file_name, subdir);
}

inline void CloseFile::compile(CodeGenerator &W) const {
  W.get_writer().end_write();
  W.unlock_writer();
}

inline UpdateLocation::UpdateLocation(const Location &location) :
  location(location) {
}

inline void UpdateLocation::compile(CodeGenerator &W) const {
  if (!W.get_writer().is_comments_locked()) {
    stage::set_location(location);
    W.get_writer().add_location(stage::get_file(), stage::get_line());
  }
}

inline void NewLine::compile(CodeGenerator &W) const {
  W.get_writer().new_line();
}

inline Indent::Indent(int val) :
  val(val) {
}

inline void Indent::compile(CodeGenerator &W) const {
  W.get_writer().indent(val);
}

inline PlainCode::PlainCode(const char *s) :
  str(s, s + strlen(s)) {
}

inline PlainCode::PlainCode(const string &s) :
  str(&s[0], &s[0] + s.size()) {
}

inline PlainCode::PlainCode(const vk::string_view &s) :
  str(s) {
}

inline void PlainCode::compile(CodeGenerator &W) const {
  W.get_writer().append(str);
}


inline void OpenBlock::compile(CodeGenerator &W) const {
  W << "{" << NL << Indent(+2);
}

inline void CloseBlock::compile(CodeGenerator &W) const {
  W << Indent(-2) << "}";
}

inline OpenNamespace::OpenNamespace() :
  OpenNamespace(G->get_global_namespace()) {
}

inline OpenNamespace::OpenNamespace(const string &ns) :
  ns_(ns) {
}

inline void OpenNamespace::compile(CodeGenerator &W) const {
  if (!ns_.empty()) {
    kphp_assert(!W.get_context().namespace_opened);
    W << "namespace " << ns_ << " {" << NL;
    W.get_context().namespace_opened = true;
  }
}

inline void CloseNamespace::compile(CodeGenerator &W) const {
  if (W.get_context().namespace_opened) {
    W << "}" << NL << NL;
    W.get_context().namespace_opened = false;
  }
}

inline ExternInclude::ExternInclude(const PlainCode &plain_code) :
  plain_code_(plain_code) {
}

inline void ExternInclude::compile(CodeGenerator &W) const {
  W << "#include \"" << plain_code_ << "\"" << NL;
}

inline void Include::compile(CodeGenerator &W) const {
  W.get_writer().add_include(static_cast<std::string>(plain_code_.str));
  ExternInclude::compile(W);
}

inline void LibInclude::compile(CodeGenerator &W) const {
  W.get_writer().add_lib_include(static_cast<std::string>(plain_code_.str));
  ExternInclude::compile(W);
}

void IncludesCollector::add_function_body_depends(const FunctionPtr &function) {
  for (auto to_include : function->dep) {
    if (to_include == function) {
      continue;
    }
    if (to_include->is_imported_from_static_lib()) {
      lib_headers_.emplace(to_include->header_full_name);
    } else if (!to_include->is_extern()) {
      internal_headers_.emplace(to_include->header_full_name);
    }
  }

  for (auto local_var : function->local_var_ids) {
    add_var_signature_depends(local_var);
  }

  for (auto global_var : function->global_var_ids) {
    add_var_signature_depends(global_var);
  }
}

void IncludesCollector::add_function_signature_depends(const FunctionPtr &function) {
  for (const auto &tinf_node : function->tinf_nodes) {
    add_all_class_types(*tinf_node.get_type());
  }
}

void IncludesCollector::add_class_include(const ClassPtr &klass) {
  classes_.emplace(klass);
}

void IncludesCollector::add_var_signature_depends(const VarPtr &var) {
  add_all_class_types(*var->tinf_node.get_type());
}

void IncludesCollector::add_all_class_types(const TypeData &tinf_type) {
  if (tinf_type.has_class_type_inside()) {
    tinf_type.get_all_class_types_inside(classes_);
  }
}

void IncludesCollector::compile(CodeGenerator &W) const {
  for (auto lib_header : lib_headers_) {
    if (!prev_headers_.count(lib_header)) {
      W << LibInclude(lib_header);
    }
  }

  std::set<std::string> class_headers;
  for (const auto &klass : classes_) {
    if (!prev_classes_.count(klass) && !klass->is_builtin()) {
      class_headers.emplace(klass->get_subdir() + "/" + klass->header_name);
    }
  }
  for (const auto &class_header : class_headers) {
    W << Include(class_header);
  }

  for (const auto &internal_header : internal_headers_) {
    if (!prev_headers_.count(internal_header)) {
      W << Include(internal_header);
    }
  }
}

void IncludesCollector::start_next_block() {
  prev_classes_.insert(classes_.cbegin(), classes_.cend());
  classes_.clear();

  prev_headers_.insert(internal_headers_.begin(), internal_headers_.end());
  prev_headers_.insert(lib_headers_.begin(), lib_headers_.end());
  internal_headers_.clear();
  lib_headers_.clear();
}

inline FunctionName::FunctionName(FunctionPtr function) :
  function(function) {
}

void FunctionName::compile(CodeGenerator &W) const {
  W << "f$";
  if (W.get_context().use_safe_integer_arithmetic && function->name == "intval") {
    W << "safe_intval";
  } else {
    W << function->name;
  }
}

inline FunctionClassName::FunctionClassName(FunctionPtr function) :
  function(function) {
}

void FunctionClassName::compile(CodeGenerator &W) const {
  W << "c$" << function->name;
}

inline FunctionForkName::FunctionForkName(FunctionPtr function) :
  function(function) {
}

void FunctionForkName::compile(CodeGenerator &W) const {
  W << "f$fork$" << function->name;
}

inline VarName::VarName(VarPtr var) :
  var(var) {
}

void VarName::compile(CodeGenerator &W) const {
  if (var->is_function_static_var()) {
    W << FunctionName(var->holder_func) << "$";
  }

  W << "v$" << var->name;
}

inline TypeName::TypeName(const TypeData *type, gen_out_style style) :
  type(type),
  style(style) {
}

inline void TypeName::compile(CodeGenerator &W) const {
  W << type_out(type, style == gen_out_style::cpp);
}

inline TypeNameInsideMacro::TypeNameInsideMacro(const TypeData *type) :
  type(type) {
}

inline void TypeNameInsideMacro::compile(CodeGenerator &W) const {
  string s = type_out(type);
  while (s.find(',') != string::npos) {
    s = s.replace(s.find(','), 1, " COMMA ");   // такое есть у tuple'ов
  }
  W << s;
}


inline FunctionCallFlag::FunctionCallFlag(FunctionPtr function) :
  function(function) {
}

inline void FunctionCallFlag::compile(CodeGenerator &W) const {
  W << FunctionName(function) << "$called";
}

inline FunctionDeclaration::FunctionDeclaration(FunctionPtr function, bool in_header, gen_out_style style) :
  function(function),
  in_header(in_header),
  style(style) {
}

inline void FunctionDeclaration::compile(CodeGenerator &W) const {
  TypeName ret_type_gen(tinf::get_type(function, -1), style);
  FunctionParams params_gen(function, in_header, style);

  switch (style) {
    case gen_out_style::cpp:
      W << ret_type_gen << " " << FunctionName(function) << "(" << params_gen << ")";
      break;
    case gen_out_style::txt:
      W << "function " << function->name << "(" << params_gen << ") ::: " << ret_type_gen;
      break;
  }
}

inline FunctionForkDeclaration::FunctionForkDeclaration(FunctionPtr function, bool in_header) :
  function(function),
  in_header(in_header) {
}

inline void FunctionForkDeclaration::compile(CodeGenerator &W) const {
  W << "int " << FunctionForkName(function) <<
    "(" << FunctionParams(function, in_header) << ")";
}


inline FunctionParams::FunctionParams(FunctionPtr function, bool in_header, gen_out_style style) :
  FunctionParams(function, 0, in_header, style) {
}

inline FunctionParams::FunctionParams(FunctionPtr function, size_t shift, bool in_header, gen_out_style style) :
  function(function),
  params(function->root->params().as<op_func_param_list>()->params()),
  in_header(in_header),
  style(style),
  shift(shift) {
    if (shift > 0) {
      params = {std::next(params.begin(), shift), params.end()};
    }
}

inline void FunctionParams::declare_cpp_param(CodeGenerator &W, VertexPtr var, const TypeName &type) const {
  W << type << " ";
  if (var->ref_flag) {
    W << "&";
  }
  W << VarName(var->get_var_id());
}

inline void FunctionParams::declare_txt_param(CodeGenerator &W, VertexPtr var, const TypeName &type) const {
  if (var->ref_flag) {
    W << "&";
  }
  W << "$" << var->get_var_id()->name << " :<=: " << type;
}

inline void FunctionParams::compile(CodeGenerator &W) const {
  bool first = true;
  size_t ii = shift;
  for (auto i : params) {
    if (i->type() == op_func_param) {
      assert ("functions with callback are not supported");
    }

    if (first) {
      first = false;
    } else {
      W << ", ";
    }

    auto param = i.as<op_func_param>();
    VertexPtr var = param->var();
    TypeName type_gen(tinf::get_type(function, ii), style);
    switch (style) {
      case gen_out_style::cpp: {
        declare_cpp_param(W, var, type_gen);
        if (param->has_default_value() && param->default_value() && in_header) {
          W << " = " << param->default_value();
        }
        break;
      }
      case gen_out_style::txt: {
        declare_txt_param(W, var, type_gen);
        if (param->has_default_value() && param->default_value() && in_header) {
          VertexPtr default_value = GenTree::get_actual_value(param->default_value());
          kphp_assert(vk::any_of_equal(default_value->type(), op_int_const, op_float_const));
          W << " = " << default_value->get_string();
        }
        break;
      }
    }
    ii++;
  }
}

ClassDeclaration::ClassDeclaration(ClassPtr klass) :
  klass(klass) {
}

void ClassDeclaration::compile(CodeGenerator &W) const {
  W << OpenFile(klass->header_name, klass->get_subdir());
  W << "#pragma once" << NL;

  W << OpenNamespace()
    << "struct " << klass->src_name << ";" << NL
    << CloseNamespace();

  IncludesCollector includes;
  klass->members.for_each([&includes](const ClassMemberInstanceField &f) {
    includes.add_var_signature_depends(f.var);
  });

  W << includes;

  W << OpenNamespace();
  W << NL << "struct " << klass->src_name << " " << BEGIN;
  W << "int ref_cnt;" << NL << NL;
  klass->members.for_each([&](const ClassMemberInstanceField &f) {
    W << TypeName(tinf::get_type(f.var)) << " $" << f.local_name() << ";" << NL;
  });

  if (!klass->is_lambda_class()) {
    W << NL << "inline const char *get_class() const " << BEGIN << "return ";
    compile_string_raw(klass->name, W);
    W << ";" << NL << END << NL;
  }

  W << END << ";" << NL;
  W << CloseNamespace();
  W << CloseFile();
}


inline VarDeclaration::VarDeclaration(VarPtr var, bool extern_flag, bool defval_flag) :
  var(var),
  extern_flag(extern_flag),
  defval_flag(defval_flag) {
}

void VarDeclaration::compile(CodeGenerator &W) const {
  const TypeData *type = tinf::get_type(var);

  if (var->is_builtin_global()) {
    W << CloseNamespace();
  }

  kphp_assert(type->ptype() != tp_void);

  W << (extern_flag ? "extern " : "") << TypeName(type) << " " << VarName(var);

  if (defval_flag) {
    if (type->ptype() == tp_float || type->ptype() == tp_int) {
      W << " = 0";
    } else if (type->get_real_ptype() == tp_bool) {
      W << " = false";
    }
  }
  W << ";" << NL;
  if (var->type() == VarData::var_local_t && type->ptype() == tp_Class && var->name == "this") {
    W << VarName(var) << ".alloc();" << NL << NL;    // инициализация $this в самом начале __construct()
  }
  if (var->needs_const_iterator_flag) {
    W << (extern_flag ? "extern " : "") <<
      "decltype(const_begin(" << VarName(var) << "))" << " " << VarName(var) << "$it;" << NL;
  }

  if (var->is_builtin_global()) {
    W << OpenNamespace();
  }
}

inline VarDeclaration VarExternDeclaration(VarPtr var) {
  return VarDeclaration(var, true, false);
}

inline VarDeclaration VarPlainDeclaration(VarPtr var) {
  return VarDeclaration(var, false, false);
}

inline Function::Function(FunctionPtr function, bool in_header) :
  function(function),
  in_header(in_header) {
}

inline void Function::compile(CodeGenerator &W) const {
  if (in_header) {
    W << FunctionDeclaration(function, in_header) << ";";
    if (function->is_resumable) {
      W << NL << FunctionForkDeclaration(function, in_header) << ";";
    }
  } else {
    W << function->root;
  }
  W << NL;
}

inline VertexCompiler::VertexCompiler(VertexPtr vertex) :
  vertex(vertex) {
}

inline void VertexCompiler::compile(CodeGenerator &W) const {
  compile_vertex(vertex, W);
}


inline RunFunction::RunFunction(FunctionPtr function) :
  function(function) {
}

inline void RunFunction::compile(CodeGenerator &W) const {
  W << "void " << FunctionName(function) << "$run() " << BEGIN
    << "TRY_CALL_VOID (void, " << FunctionName(function) << "());" << NL
    << "finish (0);" << NL
    << END;
  W << NL;
}

inline GlobalResetFunction::GlobalResetFunction(FunctionPtr function) :
  function(function) {
}

inline void GlobalResetFunction::compile(CodeGenerator &W) const {
  for (LibPtr lib: G->get_libs()) {
    if (lib && !lib->is_raw_php()) {
      W << OpenNamespace(lib->lib_namespace())
        << "void lib_global_vars_reset();" << NL
        << CloseNamespace();
    }
  }

  W << "void " << FunctionName(function) << "$global_reset() " << BEGIN;
  W << "void " << GlobalVarsResetFuncName(function) << ";" << NL;
  W << GlobalVarsResetFuncName(function) << ";" << NL;
  for (LibPtr lib: G->get_libs()) {
    if (lib && !lib->is_raw_php()) {
      W << lib->lib_namespace() << "::lib_global_vars_reset();" << NL;
    }
  }
  W << END << NL;
}

inline StaticLibraryRunGlobal::StaticLibraryRunGlobal(gen_out_style style) :
  style(style) {
}

inline void StaticLibraryRunGlobal::compile(CodeGenerator &W) const {
  switch (style) {
    case gen_out_style::cpp:
      W << "void f$" << LibData::run_global_function_name(G->get_global_namespace()) << "()";
      break;
    case gen_out_style::txt:
      W << "function " << LibData::run_global_function_name(G->get_global_namespace()) << "() ::: void";
      break;
  }
}

inline void StaticLibraryRunGlobalHeaderH::compile(CodeGenerator &W) const {
  const std::string header_path = LibData::headers_tmp_dir() +
                                  LibData::run_global_function_name(G->get_global_namespace()) + ".h";

  W << OpenFile(header_path, "", false);

  W << "#pragma once" << NL << NL;
  W << StaticLibraryRunGlobal(gen_out_style::cpp) << ";" << NL;
  W << CloseFile();
}

inline LibHeaderTxt::LibHeaderTxt(vector<FunctionPtr> &&exported_functions) :
  exported_functions(std::move(exported_functions)) {
}

inline void LibHeaderTxt::compile(CodeGenerator &W) const {
  W << OpenFile(LibData::functions_txt_tmp_file(), "", false, false);

  W << "<?php" << NL << NL;
  W << StaticLibraryRunGlobal(gen_out_style::txt) << ';' << NL << NL;

  for (const auto &function: exported_functions) {
    W << FunctionDeclaration(function, true, gen_out_style::txt) << ";" << NL;
  }

  W << CloseFile();
}

inline LibHeaderH::LibHeaderH(FunctionPtr exported_function) :
  exported_function(exported_function) {
}

inline void LibHeaderH::compile(CodeGenerator &W) const {
  W << OpenFile(LibData::headers_tmp_dir() + exported_function->header_name, "", false);

  W << "#pragma once" << NL;
  W << ExternInclude("php_functions.h") << NL;

  W << OpenNamespace()
    << FunctionDeclaration(exported_function, true, gen_out_style::cpp) << ";" << NL
    << CloseNamespace();

  W << "using " << G->get_global_namespace() << "::" << FunctionName(exported_function) << ";" << NL;
  W << CloseFile();
}

inline InitScriptsCpp::InitScriptsCpp(vector<SrcFilePtr> &&main_file_ids, vector<FunctionPtr> &&all_functions) :
  main_file_ids(std::move(main_file_ids)),
  all_functions(std::move(all_functions)) {}

inline void InitScriptsCpp::compile(CodeGenerator &W) const {
  W << OpenFile("init_php_scripts.cpp", "", false);

  W << ExternInclude("php_functions.h") <<
    ExternInclude("php_script.h");

  for (auto i : main_file_ids) {
    W << Include(i->main_function->header_full_name);
  }

  if (!G->env().is_static_lib_mode()) {
    W << NL
      << "extern \"C\" " << BEGIN
      << "void global_init_php_scripts();" << NL
      << "void init_php_scripts();" << NL
      << END << NL;
  }

  W << NL << StaticInit(all_functions) << NL;

  if (G->env().is_static_lib_mode()) {
    // only one main file is allowed for static lib mode
    kphp_assert(main_file_ids.size() == 1);
    W << LibGlobalVarsReset(main_file_ids.back()->main_function);
    W << CloseFile();
    return;
  }

  for (auto i : main_file_ids) {
    W << RunFunction(i->main_function) << NL;
    W << GlobalResetFunction(i->main_function) << NL;
  }

  W << "void init_php_scripts() " << BEGIN;

  for (auto i : main_file_ids) {
    W << FunctionName(i->main_function) << "$global_reset();" << NL;

    W << "set_script (" <<
      "\"@" << i->short_file_name << "\", " <<
      FunctionName(i->main_function) << "$run, " <<
      FunctionName(i->main_function) << "$global_reset);" << NL;
  }

  W << END;

  W << CloseFile();
}

inline static void add_dependent_declarations(VertexPtr vertex, std::set<VarPtr> &dependent_vars) {
  if (!vertex) {
    return;
  }
  for (auto child: *vertex) {
    add_dependent_declarations(child, dependent_vars);
  }
  if (vertex->type() == op_var) {
    dependent_vars.emplace(vertex->get_var_id());
  }
}

inline int array_len() {
  return (8 * sizeof(int)) / sizeof(double);
}

inline void compile_raw_array(CodeGenerator &W, const VarPtr &var, int shift) {
  if (shift == -1) {
    W << InitVar(var);
    W << VarName(var) << ".set_reference_counter_to_const();" << NL << NL;
    return;
  }

  const Location &save_location = stage::get_location();
  W << UnlockComments();
  W << UpdateLocation(var->init_val->location);

  W << VarName(var) << ".assign_raw((char *) &raw_arrays[" << int_to_str(shift) << "]);" << NL << NL;

  W << LockComments();
  stage::set_location(save_location);
}

static inline bool can_generate_raw_representation(VertexAdaptor<op_array> vertex) {
  for (auto it : vertex->args()) {
    switch (GenTree::get_actual_value(it)->type()) {
      case op_int_const:
      case op_float_const:
        continue;
      default:
        return false;
    }
  }

  return true;
}

std::vector<int> compile_arrays_raw_representation(const std::vector<VarPtr> &const_raw_array_vars, CodeGenerator &W) {
  if (const_raw_array_vars.empty()) {
    return std::vector<int>();
  }

  std::vector<int> shifts;
  shifts.reserve(const_raw_array_vars.size());

  int shift = 0;

  W << "static const union { struct { int a; int b; } is; double d; } raw_arrays[] = { ";

  for (auto var_it : const_raw_array_vars) {
    VertexAdaptor<op_array> vertex = var_it->init_val.as<op_array>();

    TypeData *vertex_inner_type = vertex->tinf_node.get_type()->lookup_at(Key::any_key());


    int array_size = vertex->size();
    int array_len_in_doubles = -1;

    if (0 <= array_size && array_size <= (1 << 30) - array_len()) {
      if (vertex_inner_type->ptype() == tp_int) {
        array_len_in_doubles = array_len() + (array_size + 1) / 2;
      } else if (vertex_inner_type->ptype() == tp_float) {
        array_len_in_doubles = array_len() + array_size;
      }
    }

    if (array_len_in_doubles == -1 || !can_generate_raw_representation(vertex)) {
      shifts.push_back(-1);
      continue;
    }

    if (shift != 0) {
      W << ",";
    }

    shifts.push_back(shift);
    shift += array_len_in_doubles;

    // ref_cnt, max_key
    W << "{ .is = { .a = " << int_to_str(REF_CNT_FOR_CONST) << ", .b = " << int_to_str(array_size - 1) << "}},";

    // end_.next, end_.prev
    W << "{ .is = { .a = 0, .b = 0}},";

    // int_size, int_buf_size
    W << "{ .is = { .a = " << int_to_str(array_size) << ", .b = " << int_to_str(array_size) << "}},";

    // string_size, string_buf_size
    W << "{ .is = { .a = 0 , .b = -1 }}";

    auto args_end = vertex->args().end();
    for (auto it = vertex->args().begin(); it != args_end; ++it) {
      VertexPtr actual_vertex = GenTree::get_actual_value(*it);
      kphp_assert(vertex_inner_type->ptype() == tp_int || vertex_inner_type->ptype() == tp_float);

      if (vertex_inner_type->ptype() == tp_int) {
        W << ",{ .is = { .a = " << actual_vertex << ", .b = ";
        ++it;

        if (it != args_end) {
          actual_vertex = GenTree::get_actual_value(*it);
          W << actual_vertex << "}}";
        } else {
          W << "0}}";
          break;
        }
      } else {
        assert(vertex_inner_type->ptype() == tp_float);

        W << ", { .d =" << actual_vertex << " }";
      }
    }
  }

  W << "};\n";

  return shifts;
}

std::vector<bool> compile_vars_part(CodeGenerator &W, const std::vector<VarPtr> &vars, size_t part) {
  std::string file_name = "vars" + std::to_string(part) + ".cpp";
  W << OpenFile(file_name, "o_vars", false);

  W << ExternInclude("php_functions.h");

  std::vector<VarPtr> const_raw_string_vars;
  std::vector<VarPtr> const_raw_array_vars;
  std::vector<VarPtr> other_const_vars;
  std::set<VarPtr> dependent_vars;

  IncludesCollector includes;
  for (auto var : vars) {
    if (!G->env().is_static_lib_mode() || !var->is_builtin_global()) {
      includes.add_var_signature_depends(var);
    }
  }
  W << includes;

  std::vector<bool> dep_mask;
  W << OpenNamespace();
  for (auto var : vars) {
    if (G->env().is_static_lib_mode() && var->is_builtin_global()) {
      continue;
    }

    W << VarDeclaration(var);
    if (var->is_constant()) {
      if (dep_mask.size() <= var->dependency_level) {
        dep_mask.resize(var->dependency_level + 1, false);
        dep_mask[var->dependency_level] = true;
      }
      switch (var->init_val->type()) {
        case op_string:
          const_raw_string_vars.push_back(var);
          break;
        case op_array:
          add_dependent_declarations(var->init_val, dependent_vars);
          const_raw_array_vars.push_back(var);
          break;
        case op_var:
          add_dependent_declarations(var->init_val, dependent_vars);
          other_const_vars.emplace_back(var);
          break;
        default:
          other_const_vars.emplace_back(var);
          break;
      }
    }
  }

  std::vector<VarPtr> extern_depends;
  std::set_difference(dependent_vars.begin(), dependent_vars.end(),
                      vars.begin(), vars.end(), std::back_inserter(extern_depends));
  for (auto var : extern_depends) {
    W << VarExternDeclaration(var);
  }

  std::string raw_data;
  std::vector<int> const_string_shifts(const_raw_string_vars.size());
  std::vector<int> const_string_length(const_raw_string_vars.size());
  int ii = 0;
  for (auto var : const_raw_string_vars) {
    int shift_to_align = (((int)raw_data.size() + 7) & -8) - (int)raw_data.size();
    if (shift_to_align != 0) {
      raw_data.append(shift_to_align, 0);
    }
    const string &s = var->init_val.as<op_string>()->get_string();
    int raw_len = string_raw_len(static_cast<int>(s.size()));
    kphp_assert (raw_len != -1);
    const_string_shifts[ii] = (int)raw_data.size();
    raw_data.append(raw_len, 0);
    int err = string_raw(&raw_data[const_string_shifts[ii]], raw_len, s.c_str(), (int)s.size());
    kphp_assert (err == raw_len);
    const_string_length[ii] = raw_len;
    ii++;
  }

  if (!raw_data.empty()) {
    W << "static const char *raw = ";
    compile_string_raw(raw_data, W);
    W << ";" << NL;
  }

  const std::vector<int> const_array_shifts = compile_arrays_raw_representation(const_raw_array_vars, W);
  kphp_assert(const_array_shifts.size() == const_raw_array_vars.size());

  for (size_t dep_level = 0; dep_level < dep_mask.size(); ++dep_level) {
    if (!dep_mask[dep_level]) {
      continue;
    }

    W << NL << "void const_vars_init_priority_" << std::to_string(dep_level) << "_file_" << std::to_string(part) << "()";
    W << BEGIN;

    for (size_t ii = 0; ii < const_raw_string_vars.size(); ++ii) {
      VarPtr var = const_raw_string_vars[ii];
      if (var->dependency_level == dep_level) {
        W << VarName(var) << ".assign_raw (&raw[" << std::to_string(const_string_shifts[ii]) << "]);" << NL;
      }
    }

    for (size_t array_id = 0; array_id < const_raw_array_vars.size(); ++array_id) {
      VarPtr var = const_raw_array_vars[array_id];
      if (var->dependency_level == dep_level) {
        compile_raw_array(W, var, const_array_shifts[array_id]);
      }
    }

    for (const auto &var: other_const_vars) {
      if (var->dependency_level == dep_level) {
        W << InitVar(var);
        auto type_data = var->tinf_node.get_type();
        PrimitiveType ptype = type_data->ptype();
        if (vk::any_of_equal(ptype, tp_array, tp_var, tp_string)) {
          W << VarName(var);
          if (type_data->use_or_false()) {
            W << ".val()";
          }
          W << ".set_reference_counter_to_const();" << NL;
        }
      }
    }
    W << END << NL;
  }

  W << CloseNamespace();
  W << CloseFile();
  return dep_mask;
}

inline VarsCpp::VarsCpp(std::vector<VarPtr> &&vars, size_t parts_cnt) :
  vars_(std::move(vars)),
  parts_cnt_(parts_cnt) {
  kphp_assert (1 <= parts_cnt_ && parts_cnt_ <= 128);
}

inline void VarsCpp::compile(CodeGenerator &W) const {
  std::vector<std::vector<VarPtr>> vars_batches(parts_cnt_);
  std::vector<std::vector<bool>> dep_masks(parts_cnt_);
  for (auto var : vars_) {
    vars_batches[vk::std_hash(var->name) % parts_cnt_].emplace_back(var);
  }
  for (size_t part_id = 0; part_id < parts_cnt_; ++part_id) {
    std::sort(vars_batches[part_id].begin(), vars_batches[part_id].end());
    dep_masks[part_id] = compile_vars_part(W, vars_batches[part_id], part_id);
  }

  W << OpenFile("vars.cpp", "", false);
  W << OpenNamespace();

  W << "void const_vars_init() " << BEGIN;

  char init_fun[128] = {0};
  const auto longest_dep_mask = std::max_element(
    dep_masks.begin(), dep_masks.end(),
    [](const std::vector<bool> &l, const std::vector<bool> &r) {
      return l.size() < r.size();
    });
  for (int dep_level = 0; dep_level < longest_dep_mask->size(); ++dep_level) {
    for (size_t part_id = 0; part_id < parts_cnt_; ++part_id) {
      if (dep_masks[part_id].size() > dep_level && dep_masks[part_id][dep_level]) {
        const int s = snprintf(init_fun, sizeof(init_fun),
                               "const_vars_init_priority_%d_file_%zu();", dep_level, part_id);
        kphp_assert(s > 0 && s < sizeof(init_fun));
        // function declaration
        W << "void " << vk::string_view{init_fun, static_cast<size_t>(s)} << NL;
        // function call
        W << vk::string_view{init_fun, static_cast<size_t>(s)} << NL;
      }
    }
  }
  W << END;
  W << CloseNamespace();
  W << CloseFile();
}

inline InitVar::InitVar(VarPtr var) :
  var(var) {
}

inline void InitVar::compile(CodeGenerator &W) const {
  Location save_location = stage::get_location();
  W << UnlockComments();

  VertexPtr init_val = var->init_val;
  if (init_val->type() == op_conv_regexp) {
    W << VarName(var) << ".init (" << var->init_val << ");" << NL;
  } else {
    W << VarName(var) << " = " << var->init_val << ";" << NL;
  }

  W << LockComments();
  stage::set_location(save_location);
}

inline StaticInit::StaticInit(const vector<FunctionPtr> &all_functions) :
  all_functions(all_functions) {
}

inline void StaticInit::compile(CodeGenerator &W) const {
  for (LibPtr lib: G->get_libs()) {
    if (lib && !lib->is_raw_php()) {
      W << OpenNamespace(lib->lib_namespace())
        << "void global_init_lib_scripts();" << NL
        << CloseNamespace();
    }
  }

  W << OpenNamespace();
  W << "void const_vars_init();" << NL << NL;

  if (G->env().is_static_lib_mode()) {
    W << "void global_init_lib_scripts() " << BEGIN;
  } else {
    W << "void global_init_php_scripts() " << BEGIN;
    for (LibPtr lib: G->get_libs()) {
      if (lib && !lib->is_raw_php()) {
        W << lib->lib_namespace() << "::global_init_lib_scripts();" << NL;
      }
    }
  }

  W << "const_vars_init();" << NL;

  W << END << NL;
  W << CloseNamespace();
}

inline LibGlobalVarsReset::LibGlobalVarsReset(const FunctionPtr &main_fn) :
  main_function(main_fn) {
}

inline void LibGlobalVarsReset::compile(CodeGenerator &W) const {
  W << OpenNamespace();
  W << "void lib_global_vars_reset() " << BEGIN
    << FunctionCallFlag(main_function) << " = false;" << NL
    << "void " << GlobalVarsResetFuncName(main_function) << ";" << NL
    << GlobalVarsResetFuncName(main_function) << ";" << NL
    << END << NL << NL;
  W << CloseNamespace();

  W << StaticLibraryRunGlobal(gen_out_style::cpp) << BEGIN
    << "using namespace " << G->get_global_namespace() << ";" << NL
    << "require_once ("
    << FunctionCallFlag(main_function) << ", "
    << FunctionName(main_function) << "());" << NL
    << END << NL << NL;
}

GlobalVarsResetFuncName::GlobalVarsResetFuncName(FunctionPtr main_func, int part) :
  main_func_(main_func),
  part_(part) {}

void GlobalVarsResetFuncName::compile(CodeGenerator &W) const {
  W << FunctionName(main_func_) << "$global_vars_reset";
  if (part_ >= 0) {
    W << std::to_string(part_);
  }
  W << "()";
}


GlobalVarsReset::GlobalVarsReset(SrcFilePtr main_file) :
  main_file_(main_file) {
}

void GlobalVarsReset::declare_extern_for_init_val(VertexPtr v, std::set<VarPtr> &externed_vars, CodeGenerator &W) {
  if (v->type() == op_var) {
    VarPtr var = v->get_var_id();
    if (externed_vars.insert(var).second) {
      W << VarExternDeclaration(var);
    }
    return;
  }
  for (VertexPtr son : *v) {
    declare_extern_for_init_val(son, externed_vars, W);
  }
}

void GlobalVarsReset::compile_part(FunctionPtr func, const set<VarPtr> &used_vars, int part_i, CodeGenerator &W) {
  IncludesCollector includes;
  for (auto var : used_vars) {
    includes.add_var_signature_depends(var);
  }
  W << includes;
  W << OpenNamespace();

  std::set<VarPtr> externed_vars;
  for (auto var : used_vars) {
    W << VarExternDeclaration(var);
    if (var->init_val) {
      declare_extern_for_init_val(var->init_val, externed_vars, W);
    }
  }

  W << "void " << GlobalVarsResetFuncName(func, part_i) << " " << BEGIN;
  for (auto var : used_vars) {
    if (G->env().is_static_lib_mode() && var->is_builtin_global()) {
      continue;
    }

    W << "hard_reset_var(" << VarName(var);
    //FIXME: brk and comments
    if (var->init_val) {
      W << UnlockComments();
      W << ", " << var->init_val;
      W << LockComments();
    }
    W << ");" << NL;
  }

  W << END;
  W << NL;
  W << CloseNamespace();
}

void GlobalVarsReset::compile_func(FunctionPtr func, const std::set<FunctionPtr> &used_functions, int parts_n, CodeGenerator &W) {
  W << OpenNamespace();
  W << "void " << GlobalVarsResetFuncName(func) << " " << BEGIN;
  for (auto func : used_functions) {
    if (func->type == FunctionData::func_global) {
      W << "extern bool " << FunctionCallFlag(func) << ";" << NL;
      W << FunctionCallFlag(func) << " = false;" << NL;
    }
  }

  for (int i = 0; i < parts_n; i++) {
    W << "void " << GlobalVarsResetFuncName(func, i) << ";" << NL;
    W << GlobalVarsResetFuncName(func, i) << ";" << NL;
  }

  W << END;
  W << NL;
  W << CloseNamespace();
}

template<class It>
void collect_vars(set<VarPtr> *used_vars, int used_vars_cnt, It begin, It end) {
  for (; begin != end; begin++) {
    VarPtr var_id = *begin;
    size_t var_hash;
    if (var_id->class_id) {
      var_hash = vk::std_hash(var_id->class_id->file_id->main_func_name);
    } else {
      var_hash = vk::std_hash(var_id->name);
    }
    int bucket = var_hash % used_vars_cnt;
    used_vars[bucket].insert(var_id);
  }
}

void GlobalVarsReset::collect_used_funcs_and_vars(
  FunctionPtr func, std::set<FunctionPtr> *visited_functions,
  std::set<VarPtr> *used_vars, int used_vars_cnt) {
  for (int i = 0, ni = (int)func->dep.size(); i < ni; i++) {
    FunctionPtr to = func->dep[i];
    if (visited_functions->insert(to).second) {
      collect_used_funcs_and_vars(to, visited_functions, used_vars, used_vars_cnt);
    }
  }

  collect_vars(used_vars, used_vars_cnt, func->global_var_ids.begin(), func->global_var_ids.end());
  collect_vars(used_vars, used_vars_cnt, func->static_var_ids.begin(), func->static_var_ids.end());
}

inline void GlobalVarsReset::compile(CodeGenerator &W) const {
  FunctionPtr main_func = main_file_->main_function;

  std::set<FunctionPtr> used_functions;

  const int parts_n = 32;
  std::set<VarPtr> used_vars[parts_n];
  collect_used_funcs_and_vars(main_func, &used_functions, used_vars, parts_n);

  auto last_part = std::remove_if(std::begin(used_vars), std::end(used_vars),
                                  [](const std::set<VarPtr> &p) { return p.empty(); });
  auto non_empty_parts = std::distance(std::begin(used_vars), last_part);

  static const std::string vars_reset_src_prefix = "vars_reset.";
  std::vector<std::string> src_names(non_empty_parts);
  for (int i = 0; i < non_empty_parts; i++) {
    src_names[i] = vars_reset_src_prefix + std::to_string(i) + "." + main_func->src_name;
  }

  for (int i = 0; i < non_empty_parts; i++) {
    W << OpenFile(src_names[i], "o_vars_reset", false);
    W << ExternInclude("php_functions.h");
    compile_part(main_func, used_vars[i], i, W);
    W << CloseFile();
  }

  W << OpenFile(vars_reset_src_prefix + main_func->src_name, "", false);
  W << ExternInclude("php_functions.h");
  compile_func(main_func, used_functions, non_empty_parts, W);
  W << CloseFile();
}

static inline void declare_global_vars(FunctionPtr function, CodeGenerator &W) {
  for (auto global_var : function->global_var_ids) {
    W << VarExternDeclaration(global_var) << NL;
  }
}

static inline void declare_const_vars(FunctionPtr function, CodeGenerator &W) {
  for (auto const_var : function->explicit_const_var_ids) {
    W << VarExternDeclaration(const_var) << NL;
  }
}

static inline void declare_static_vars(FunctionPtr function, CodeGenerator &W) {
  for (auto static_var : function->static_var_ids) {
    W << VarExternDeclaration(static_var) << NL;
  }
}

inline FunctionH::FunctionH(FunctionPtr function) :
  function(function) {
}

void FunctionH::compile(CodeGenerator &W) const {
  W << OpenFile(function->header_name, function->subdir);
  W << "#pragma once" << NL <<
    ExternInclude("php_functions.h");

  IncludesCollector includes;
  includes.add_function_signature_depends(function);
  W << includes;

  W << OpenNamespace();
  for (auto const_var : function->explicit_header_const_var_ids) {
    W << VarExternDeclaration(const_var) << NL;
  }

  if (function->type == FunctionData::func_global) {
    W << "extern bool " << FunctionCallFlag(function) << ";" << NL;
  }

  if (function->is_inline) {
    W << "static inline " << Function(function, true);
    stage::set_function(function);
    declare_global_vars(function, W);
    declare_const_vars(function, W);
    declare_static_vars(function, W);
    W << CloseNamespace();
    includes.start_next_block();
    includes.add_function_body_depends(function);
    W << includes;
    W << OpenNamespace();
    W << UnlockComments();
    W << Function(function);
    W << LockComments();
  } else {
    W << Function(function, true);
  }

  W << CloseNamespace();
  W << CloseFile();
}

inline FunctionCpp::FunctionCpp(FunctionPtr function) :
  function(function) {
}

void FunctionCpp::compile(CodeGenerator &W) const {
  if (function->is_inline) {
    return;
  }
  W << OpenFile(function->src_name, function->subdir);
  W << ExternInclude("php_functions.h");
  W << Include(function->header_full_name);

  stage::set_function(function);

  IncludesCollector includes;
  includes.add_function_body_depends(function);
  W << includes;

  W << OpenNamespace();
  declare_global_vars(function, W);
  declare_const_vars(function, W);
  declare_static_vars(function, W);

  if (function->type == FunctionData::func_global) {
    W << "bool " << FunctionCallFlag(function) << ";" << NL;
  }

  W << UnlockComments();
  W << Function(function);
  W << LockComments();

  W << CloseNamespace();
  W << CloseFile();
}


Operand::Operand(VertexPtr root, Operation parent_type, bool is_left) :
  root(root),
  parent_type(parent_type),
  is_left(is_left) {
}

inline void Operand::compile(CodeGenerator &W) const {
  int priority = OpInfo::priority(parent_type);
  bool left_to_right = OpInfo::fixity(parent_type) == left_opp;

  int root_priority = OpInfo::priority(root->type());

  bool need_par = (root_priority < priority || (root_priority == priority && (left_to_right ^ is_left))) && root_priority > 0;
  need_par |= parent_type == op_log_and_let || parent_type == op_log_or_let || parent_type == op_log_xor_let;

  if (need_par) {
    W << "(";
  }

  W << root;

  if (need_par) {
    W << ")";
  }
}

inline LabelName::LabelName(int label_id) :
  label_id(label_id) {
}

inline void LabelName::compile(CodeGenerator &W) const {
  W << "label" << int_to_str(label_id);
}

inline Label::Label(int label_id) :
  label_id(label_id) {
}

inline void Label::compile(CodeGenerator &W) const {
  if (label_id != 0) {
    W << NL << LabelName(label_id) << ":;" << NL;
  }
}

inline AsList::AsList(VertexPtr root, string delim) :
  root(root),
  delim(delim) {
}

inline void AsList::compile(CodeGenerator &W) const {
  bool first = true;
  for (auto i : *root) {
    if (first) {
      first = false;
    } else {
      W << delim;
    }
    W << i;
  }
}

inline AsSeq::AsSeq(VertexPtr root) :
  root(root) {
}

inline void AsSeq::compile(CodeGenerator &W) const {
  if (root->type() != op_seq) {
    W << root << ";" << NL;
    return;
  }

  for (auto i : *root) {
    if (vk::none_of_equal(i->type(), op_var, op_empty)) {
      W << i << ";" << NL;
    }
  }
}

inline TupleGetIndex::TupleGetIndex(VertexPtr tuple, const std::string &int_index) :
  tuple(tuple),
  int_index(int_index) {
}

inline TupleGetIndex::TupleGetIndex(VertexPtr tuple, VertexPtr key) :
  tuple(tuple),
  int_index(GenTree::get_actual_value(key)->get_string()) {
}

inline void TupleGetIndex::compile(CodeGenerator &W) const {
  W << "std::get<" << int_index << ">(" << tuple << ")";
}

void compile_prefix_op(VertexAdaptor<meta_op_unary> root, CodeGenerator &W) {
  W << OpInfo::str(root->type()) << Operand(root->expr(), root->type(), true);
}

void compile_postfix_op(VertexAdaptor<meta_op_unary> root, CodeGenerator &W) {
  W << Operand(root->expr(), root->type(), true) << OpInfo::str(root->type());
}


void compile_conv_op(VertexAdaptor<meta_op_unary> root, CodeGenerator &W) {
  if (root->type() == op_conv_regexp) {
    W << root->expr();
  } else {
    W << OpInfo::str(root->type()) << " (" << root->expr() << ")";
  }
}

void compile_noerr(VertexAdaptor<op_noerr> root, CodeGenerator &W) {
  if (root->rl_type == val_none) {
    W << "NOERR_VOID (" << Operand(root->expr(), root->type(), true) << ")";
  } else {
    const TypeData *res_tp = tinf::get_type(root);
    W << "NOERR (" << Operand(root->expr(), root->type(), true) << ", " << TypeNameInsideMacro(res_tp) << ")";
  }
}


void compile_binary_func_op(VertexAdaptor<meta_op_binary> root, CodeGenerator &W) {
  if (root->type() == op_pow) {
    compile_power(root.as<op_pow>(), W);
  } else {
    W << OpInfo::str(root->type());
  }
  W << " (" <<
    Operand(root->lhs(), root->type(), true) <<
    ", " <<
    Operand(root->rhs(), root->type(), false) <<
    ")";
}


void compile_binary_op(VertexAdaptor<meta_op_binary> root, CodeGenerator &W) {
  auto &root_type_str = OpInfo::str(root->type());
  kphp_error_return (root_type_str[0] != '@', format("Unexpected %s\n", root_type_str.c_str() + 1));

  auto lhs = root->lhs();
  auto rhs = root->rhs();

  auto lhs_tp = tinf::get_type(lhs);
  auto rhs_tp = tinf::get_type(rhs);

  if (root->type() == op_add) {
    if (lhs_tp->ptype() == tp_array && rhs_tp->ptype() == tp_array && type_out(lhs_tp) != type_out(rhs_tp)) {
      const TypeData *res_tp = tinf::get_type(root)->const_read_at(Key::any_key());
      W << "array_add < " << TypeName(res_tp) << " > (" << lhs << ", " << rhs << ")";
      return;
    }
  }
  // специальные inplace переменные, которые объявляются в момент присваивания, а не в начале функции
  if (root->type() == op_set && lhs->type() == op_var && lhs->extra_type == op_ex_var_superlocal_inplace) {
    W << TypeName(lhs_tp) << " ";    // получится не "$tmp = v", а "array<T> $tmp = v" к примеру
  }

  if (OpInfo::type(root->type()) == binary_func_op) {
    compile_binary_func_op(root, W);
    return;
  }

  bool lhs_is_bool = lhs_tp->get_real_ptype() == tp_bool;
  bool rhs_is_bool = rhs_tp->get_real_ptype() == tp_bool;
  if (rhs_is_bool ^ lhs_is_bool) {
    static const std::unordered_map<int, const char *, std::hash<int>> tp_to_str{
      {op_lt,   "lt"},
      {op_gt,   "gt"},
      {op_le,   "leq"},
      {op_ge,   "geq"},
      {op_eq2,  "eq2"},
      {op_neq2, "neq2"},
    };

    auto str_repr_it = tp_to_str.find(root->type());
    if (str_repr_it != tp_to_str.end()) {
      W << str_repr_it->second << " (" << lhs << ", " << rhs << ")";
      return;
    }
  }

  W << Operand(lhs, root->type(), true) <<
    " " << root_type_str << " " <<
    Operand(rhs, root->type(), false);
}


void compile_ternary_op(VertexAdaptor<op_ternary> root, CodeGenerator &W) {
  VertexPtr cond = root->cond();
  VertexPtr true_expr = root->true_expr();
  VertexPtr false_expr = root->false_expr();

  const TypeData *true_expr_tp, *false_expr_tp, *res_tp = nullptr;
  true_expr_tp = tinf::get_type(true_expr);
  false_expr_tp = tinf::get_type(false_expr);

  //TODO: optimize type_out
  if (type_out(true_expr_tp) != type_out(false_expr_tp)) {
    res_tp = tinf::get_type(root);
  }

  W << Operand(cond, root->type(), true) << " ? ";

  if (res_tp != nullptr) {
    W << TypeName(res_tp) << "(";
  }
  W << Operand(true_expr, root->type(), true);
  if (res_tp != nullptr) {
    W << ")";
  }

  W << " : ";

  if (res_tp != nullptr) {
    W << TypeName(res_tp) << "(";
  }
  W << Operand(false_expr, root->type(), true);
  if (res_tp != nullptr) {
    W << ")";
  }
}


void compile_if(VertexAdaptor<op_if> root, CodeGenerator &W) {
  W << "if (" << root->cond() << ") " <<
    BEGIN <<
    AsSeq(root->true_cmd()) <<
    END;

  if (root->has_false_cmd()) {
    W << " else " << root->false_cmd();
  }
}


inline CycleBody::CycleBody(VertexPtr body, int continue_label_id, int break_label_id) :
  body(body),
  continue_label_id(continue_label_id),
  break_label_id(break_label_id) {
}

inline void CycleBody::compile(CodeGenerator &W) const {
  W << BEGIN <<
    AsSeq(body) <<
    Label(continue_label_id) <<
    END <<
    Label(break_label_id);
}


void compile_while(VertexAdaptor<op_while> root, CodeGenerator &W) {
  W << "while (" << root->cond() << ") " <<
    CycleBody(root->cmd(), root->continue_label_id, root->break_label_id);
}


void compile_do(VertexAdaptor<op_do> root, CodeGenerator &W) {
  W << "do " <<
    BEGIN <<
    AsSeq(root->cmd()) <<
    Label(root->continue_label_id) <<
    END << " while (";
  if (root->cond()->type() == op_empty) {
    W << "0";
  } else {
    W << root->cond();
  }
  W << ");" << NL << Label(root->break_label_id);
}


void compile_require(VertexPtr root, CodeGenerator &W) {
  if (root->type() == op_require) {
    W << "require (";
  } else if (root->type() == op_require_once) {
    W << "require_once (";
  } else {
    kphp_fail();
  }

  kphp_assert_msg(root->size() == 1, "Only one child possible for require vertex");
  auto func = root->back().as<op_func_call>();
  W << FunctionCallFlag(func->get_func_id()) << ", " << func << ")";
}


void compile_return(VertexAdaptor<op_return> root, CodeGenerator &W) {
  bool resumable_flag = W.get_context().resumable_flag;
  if (resumable_flag) {
    if (root->has_expr()) {
      W << "RETURN (";
    } else {
      W << "RETURN_VOID (";
    }
  } else {
    W << "return ";
  }

  if (root->has_expr()) {
    W << root->expr();
  }

  if (resumable_flag) {
    W << ")";
  }
}


void compile_for(VertexAdaptor<op_for> root, CodeGenerator &W) {
  W << "for (" <<
    AsList(root->pre_cond(), ", ") << "; " <<
    AsList(root->cond(), ", ") << "; " <<
    AsList(root->post_cond(), ", ") << ") " <<
    CycleBody(root->cmd(), root->continue_label_id, root->break_label_id);
}

//TODO: some interface for context?
//TODO: it's copypasted to compile_return
void compile_throw_action(CodeGenerator &W) {
  CGContext &context = W.get_context();
  if (context.catch_labels.empty() || context.catch_labels.back().empty()) {
    const TypeData *tp = tinf::get_type(context.parent_func, -1);
    if (context.resumable_flag) {
      if (tp->ptype() != tp_void) {
        W << "RETURN (";
      } else {
        W << "RETURN_VOID (";
      }
    } else {
      W << "return ";
    }
    if (tp->ptype() != tp_void) {
      W << "(" << TypeName(tp) << "())";
    }
    if (context.resumable_flag) {
      W << ")";
    }
  } else {
    W << "goto " << context.catch_labels.back();
    context.catch_label_used.back() = 1;
  }
}


void compile_throw(VertexAdaptor<op_throw> root, CodeGenerator &W) {
  W << BEGIN <<
    "THROW_EXCEPTION (" << root->exception() << ");" << NL;
  compile_throw_action(W);
  W << ";" << NL <<
    END << NL;
}


void compile_try(VertexAdaptor<op_try> root, CodeGenerator &W) {
  CGContext &context = W.get_context();

  string catch_label = gen_unique_name("catch_label");
  W << "/""*** TRY ***""/" << NL;
  context.catch_labels.push_back(catch_label);
  context.catch_label_used.push_back(0);
  W << root->try_cmd() << NL;
  context.catch_labels.pop_back();
  bool used = context.catch_label_used.back();
  context.catch_label_used.pop_back();

  if (used) {
    W << "/""*** CATCH ***""/" << NL <<
      "if (0) " <<
      BEGIN <<
      catch_label << ":;" << NL << //TODO: Label (lable_id) ?
      root->exception() << " = *CurException;" << NL <<
      "FREE_EXCEPTION;" << NL <<
      root->catch_cmd() << NL <<
      END << NL;
  }
}

void compile_fork(VertexAdaptor<op_fork> root, CodeGenerator &W) {
  compile_func_call(root->func_call().as<op_func_call>(), W, 2);
}

void compile_async(VertexAdaptor<op_async> root, CodeGenerator &W) {
  VertexPtr lhs = root->lhs();
  auto func_call = root->func_call().as<op_func_call>();
  if (lhs->type() != op_empty) {
    kphp_error (lhs->type() == op_var, "Can't save result of async call into non-var");
    W << lhs << " = ";
  }
  compile_func_call(func_call, W, 1);
  FunctionPtr func = func_call->get_func_id();
  W << ";" << NL;
  if (lhs->type() != op_empty) {
    W << "TRY_WAIT(" << gen_unique_name("resumable_label") << ", " << lhs << ", "
      << TypeNameInsideMacro(tinf::get_type(func_call)) << ");";
  } else {
    W << "TRY_WAIT_DROP_RESULT(" << gen_unique_name("resumable_label") << ", "
      << TypeNameInsideMacro(tinf::get_type(func_call)) << ");";
  }

  if (func->can_throw) {
    W << NL;
    W << "CHECK_EXCEPTION(";
    compile_throw_action(W);
    W << ")";
  }
}

void compile_foreach_ref_header(VertexAdaptor<op_foreach> root, CodeGenerator &W) {
  kphp_error(!W.get_context().resumable_flag, "foreach by reference is forbidden in resumable mode");
  auto params = root->params();

  //foreach (xs as [key =>] x)
  VertexPtr xs = params->xs();
  VertexPtr x = params->x();
  VertexPtr key;
  if (params->has_key()) {
    key = params->key();
  }

  string xs_copy_str;
  xs_copy_str = gen_unique_name("tmp_expr");
  const TypeData *xs_type = tinf::get_type(xs);

  W << BEGIN;
  //save array to 'xs_copy_str'
  W << TypeName(xs_type) << " &" << xs_copy_str << " = " << xs << ";" << NL;

  string it = gen_unique_name("it");
  W << "for (auto " << it << " = begin (" << xs_copy_str << "); " <<
    it << " != end (" << xs_copy_str << "); " <<
    "++" << it << ")" <<
    BEGIN;


  //save value
  W << TypeName(tinf::get_type(x)) << " &";
  W << x << " = " << it << ".get_value();" << NL;

  //save key
  if (key) {
    W << key << " = " << it << ".get_key();" << NL;
  }
}

void compile_foreach_noref_header(VertexAdaptor<op_foreach> root, CodeGenerator &W) {
  auto params = root->params();
  //foreach (xs as [key =>] x)
  VertexPtr x = params->x();
  VertexPtr xs = params->xs();
  VertexPtr key;
  auto temp_var = params->temp_var().as<op_var>();
  if (params->has_key()) {
    key = params->key();
  }

  TypeData const *type_data = xs->tinf_node.type_;
  if (xs->type() == op_var) {
    type_data = xs->get_var_id()->tinf_node.type_;
  }

  PrimitiveType ptype = type_data->get_real_ptype();

  if (vk::none_of_equal(ptype, tp_array, tp_var)) {
    kphp_error_return(false, format("%s (%s)", "Invalid argument supplied for foreach()", ptype_name(ptype)));
  }

  W << BEGIN;
  //save array to 'xs_copy_str'
  W << temp_var << " = " << xs << ";" << NL;
  W << "for (" << temp_var << "$it" << " = const_begin (" << temp_var << "); " <<
    temp_var << "$it" << " != const_end (" << temp_var << "); " <<
    "++" << temp_var << "$it" << ")" <<
    BEGIN;


  //save value
  W << x << " = " << temp_var << "$it" << ".get_value();" << NL;

  //save key
  if (key) {
    W << key << " = " << temp_var << "$it" << ".get_key();" << NL;
  }
}

void compile_foreach(VertexAdaptor<op_foreach> root, CodeGenerator &W) {
  auto params = root->params();
  VertexPtr cmd = root->cmd();

  //foreach (xs as [key =>] x)
  if (params->x()->ref_flag) {
    compile_foreach_ref_header(root, W);
  } else {
    compile_foreach_noref_header(root, W);
  }

  if (stage::has_error()) {
    return;
  }

  W << AsSeq(cmd) << NL <<
    Label(root->continue_label_id) <<
    END <<
    Label(root->break_label_id) << NL;
  if (!params->x()->ref_flag) {
    VertexPtr temp_var = params->temp_var();
    W << "clear_array(" << temp_var << ");" << NL;
  }
  W << END;
}

struct CaseInfo {
  unsigned int hash;
  bool is_default;
  string goto_name;
  CaseInfo *next;
  VertexPtr v;
  VertexPtr expr;
  VertexPtr cmd;

  inline CaseInfo() :
    hash(0),
    is_default(false),
    next(nullptr) {}

  inline CaseInfo(VertexPtr root) :
    hash(0),
    next(nullptr),
    v(root) {
    if (v->type() == op_default) {
      is_default = true;
      cmd = v.as<op_default>()->cmd();
    } else {
      is_default = false;
      auto cs = v.as<op_case>();

      expr = cs->expr();
      cmd = cs->cmd();

      VertexPtr val = GenTree::get_actual_value(expr);
      kphp_assert (val->type() == op_string);
      const string &s = val.as<op_string>()->str_val;
      hash = string_hash(s.c_str(), (int)s.size());
    }
  }
};


void compile_switch_str(VertexAdaptor<op_switch> root, CodeGenerator &W) {
  vector<CaseInfo> cases;

  for (auto i : root->cases()) {
    cases.push_back(CaseInfo(i));
  }
  int n = (int)cases.size();

  CaseInfo *default_case = nullptr;
  for (int i = 0; i < n; i++) {
    if (cases[i].is_default) {
      kphp_error_return (default_case == nullptr, "Several default cases in switch");
      default_case = &cases[i];
    }
  }

  map<unsigned int, int> prev;
  for (int i = n - 1; i >= 0; i--) {
    if (cases[i].is_default) {
      continue;
    }
    pair<unsigned int, int> new_val(cases[i].hash, i);
    pair<map<unsigned int, int>::iterator, bool> insert_res = prev.insert(new_val);
    if (insert_res.second == false) {
      int next_i = insert_res.first->second;
      insert_res.first->second = i;
      cases[i].next = &cases[next_i];
    } else {
      cases[i].next = default_case;
    }
    CaseInfo *next = cases[i].next;
    if (next != nullptr && next->goto_name.empty()) {
      next->goto_name = gen_unique_name("switch_goto");
    }
  }

  W << BEGIN;
  W << "static_cast<void>(" << root->switch_var() << ");" << NL;
  W << root->ss() << " = f$strval (" << root->expr() << ");" << NL;
  W << root->ss_hash() << " = " << root->ss() << ".hash();" << NL;
  W << root->switch_flag() << " = false;" << NL;

  W << "switch (" << root->ss_hash() << ") " <<
    BEGIN;
  for (int i = 0; i < n; i++) {
    CaseInfo *cur = &cases[i];
    if (cur->is_default) {
      W << "default:" << NL;
    } else if (cur->goto_name.empty()) {
      char buf[100];
      sprintf(buf, "0x%x", cur->hash);

      W << "case " << (const char *)buf << ":" << NL;
    }
    if (!cur->goto_name.empty()) {
      W << cur->goto_name << ":;" << NL;
    }

    if (!cur->is_default) {
      W << "if (!" << root->switch_flag() << ") " <<
        BEGIN <<
        "if (!equals (" << root->ss() << ", " << cur->expr << ")) " <<
        BEGIN;
      string next_goto;
      if (cur->next != nullptr) {
        next_goto = cur->next->goto_name;
        W << "goto " << next_goto << ";" << NL;
      } else {
        W << "break;" << NL;
      }
      W << END <<
        " else " <<
        BEGIN <<
        root->switch_flag() << " = true;" << NL <<
        END << NL <<
        END << NL;
    } else {
      W << root->switch_flag() << " = true;" << NL;
    }
    W << cur->cmd << NL;
  }
  W << END << NL;

  W << Label(root->continue_label_id) <<
    END <<
    Label(root->break_label_id);
}

void compile_switch_int(VertexAdaptor<op_switch> root, CodeGenerator &W) {
  W << "switch (f$intval (" << root->expr() << "))" <<
    BEGIN;
  W << "static_cast<void>(" << root->ss() << ");" << NL;
  W << "static_cast<void>(" << root->ss_hash() << ");" << NL;
  W << "static_cast<void>(" << root->switch_var() << ");" << NL;
  W << "static_cast<void>(" << root->switch_flag() << ");" << NL;

  set<string> used;
  for (auto i : root->cases()) {
    Operation tp = i->type();
    VertexPtr cmd;
    if (tp == op_case) {
      auto cs = i.as<op_case>();
      cmd = cs->cmd();

      VertexPtr val = GenTree::get_actual_value(cs->expr());
      kphp_assert (val->type() == op_int_const || is_const_int(val));
      W << "case ";
      if (val->type() == op_int_const) {
        string str = val.as<op_int_const>()->str_val;
        W << str;
        kphp_error (!used.count(str),
                    format("Switch: repeated cases found [%s]", str.c_str()));
        used.insert(str);
      } else {
        compile_vertex(val, W);
      }
    } else if (tp == op_default) {
      W << "default";
      cmd = i.as<op_default>()->cmd();
    } else {
      kphp_fail();
    }
    W << ": " << cmd << NL;
  }
  W << Label(root->continue_label_id) <<
    END <<
    Label(root->break_label_id);
}


void compile_switch_var(VertexAdaptor<op_switch> root, CodeGenerator &W) {
  string goto_name;

  W << "do " << BEGIN;
  W << "static_cast<void>(" << root->ss() << ");" << NL;
  W << "static_cast<void>(" << root->ss_hash() << ");" << NL;
  W << root->switch_var() << " = " << root->expr() << ";" << NL <<
    root->switch_flag() << " = false;" << NL;

  for (auto i : root->cases()) {
    Operation tp = i->type();
    VertexPtr expr;
    VertexPtr cmd;
    if (tp == op_case) {
      auto cs = i.as<op_case>();
      expr = cs->expr();
      cmd = cs->cmd();
    } else if (tp == op_default) {
      cmd = i.as<op_default>()->cmd();
    } else {
      kphp_fail();
    }

    W << "if (" << root->switch_flag();

    if (tp == op_case) {
      W << " || eq2 (" << root->switch_var() << ", " << expr << ")";
    }
    W << ") " <<
      BEGIN;
    if (tp == op_default) {
      goto_name = gen_unique_name("switch_goto");
      W << goto_name + ": ";
    }

    W << root->switch_flag() << " = true;" << NL <<
      AsSeq(cmd) <<
      END << NL;
  }


  if (!goto_name.empty()) {
    W << "if (" << root->switch_flag() << ") " <<
      BEGIN <<
      "break;" << NL <<
      END << NL <<
      root->switch_flag() << " = true;" << NL <<
      "goto " << goto_name << ";" << NL;
  }


  W << Label(root->continue_label_id) <<
    END << " while (0)" <<
    Label(root->break_label_id);
}


void compile_switch(VertexAdaptor<op_switch> root, CodeGenerator &W) {
  kphp_assert(root->ss()->type() == op_var);
  kphp_assert(root->ss_hash()->type() == op_var);
  kphp_assert(root->switch_var()->type() == op_var);
  kphp_assert(root->switch_flag()->type() == op_var);
  int cnt_int = 0, cnt_str = 0, cnt_default = 0;

  for (auto i : root->cases()) {
    if (i->type() == op_default) {
      cnt_default++;
    } else {
      auto cs = i.as<op_case>();
      VertexPtr val = GenTree::get_actual_value(cs->expr());
      if (val->type() == op_int_const || is_const_int(val)) {
        cnt_int++;
      } else if (val->type() == op_string) {
        cnt_str++;
      } else {
        cnt_str++;
        cnt_int++;
      }
    }
  }
  kphp_error_return (cnt_default <= 1, "Switch: several default cases found");

  if (!cnt_int) {
    compile_switch_str(root, W);
  } else if (!cnt_str) {
    compile_switch_int(root, W);
  } else {
    compile_switch_var(root, W);
  }
}

void compile_function_resumable(VertexAdaptor<op_function> func_root, CodeGenerator &W) {
  FunctionPtr func = func_root->get_func_id();
  W << "//RESUMABLE FUNCTION IMPLEMENTATION" << NL;
  W << "class " << FunctionClassName(func) << " : public Resumable " <<
    BEGIN <<
    "private:" << NL << Indent(+2);


  //MEMBER VARIABLES
  for (auto var : func->param_ids) {
    kphp_error(!var->is_reference, "reference function parametrs are forbidden in resumable mode");
    W << VarPlainDeclaration(var);
  }
  for (auto var : func->local_var_ids) {
    W << VarPlainDeclaration(var);         // inplace-переменные тоже, идут как члены Resumable класса, а не по месту
  }

  W << Indent(-2) << "public:" << NL << Indent(+2);

  //ReturnT
  W << "typedef " << TypeName(tinf::get_type(func, -1)) << " ReturnT;" << NL;

  //CONSTRUCTOR
  W << FunctionClassName(func) << "(" << FunctionParams(func) << ")";
  if (!func->param_ids.empty()) {
    W << " :" << NL <<
      Indent(+2);
    bool flag = false;
    int i = 0;
    for (auto var : func->param_ids) {
      if (flag) {
        W << "," << NL;
      } else {
        flag = true;
      }
      W << VarName(var) << "(" << VarName(var) << ")";
      i++;
    }
    for (auto var : func->local_var_ids) {
      if (flag) {
        W << "," << NL;
      } else {
        flag = true;
      }
      W << VarName(var) << "()";
    }
    W << Indent(-2);
  }
  W << " " << BEGIN << END << NL;

  //RUN FUNCTION
  W << "bool run() " <<
    BEGIN;
  if (G->env().get_enable_profiler()) {
    W << "Profiler __profiler(\"" << func->name.c_str() << "\");" << NL;
  }
  W << "RESUMABLE_BEGIN" << NL << Indent(+2);

  W << AsSeq(func_root->cmd()) << NL;

  W << Indent(-2) <<
    "RESUMABLE_END" << NL <<
    END << NL;


  W << Indent(-2);
  W << END << ";" << NL;

  //CALL FUNCTION
  W << FunctionDeclaration(func, false) << " " <<
    BEGIN;
  W << "return start_resumable < " << FunctionClassName(func) << "::ReturnT >" <<
    "(new " << FunctionClassName(func) << "(";
  bool flag = false;
  for (auto var : func->param_ids) {
    if (flag) {
      W << ", ";
    } else {
      flag = true;
    }
    W << VarName(var);
  }
  W << "));" << NL;
  W << END << NL;

  //FORK FUNCTION
  W << FunctionForkDeclaration(func, false) << " " <<
    BEGIN;
  W << "return fork_resumable < " << FunctionClassName(func) << "::ReturnT >" <<
    "(new " << FunctionClassName(func) << "(";
  flag = false;
  for (auto var : func->param_ids) {
    if (flag) {
      W << ", ";
    } else {
      flag = true;
    }
    W << VarName(var);
  }
  W << "));" << NL;
  W << END << NL;

}

void compile_function(VertexAdaptor<op_function> func_root, CodeGenerator &W) {
  FunctionPtr func = func_root->get_func_id();

  W.get_context().parent_func = func;
  W.get_context().resumable_flag = func->is_resumable;

  if (func->is_resumable) {
    compile_function_resumable(func_root, W);
    return;
  }

  if (func->is_inline) {
    W << "static inline ";
  }

  W << FunctionDeclaration(func, false) << " " << BEGIN;

  if (G->env().get_enable_profiler()) {
    W << "Profiler __profiler(\"" << func->name.c_str() << "\");" << NL;
  }

  for (auto var : func->local_var_ids) {
    if (var->type() != VarData::var_local_inplace_t) {
      W << VarDeclaration(var);
    }
  }

  if (func->has_variadic_param) {
    auto params = func->get_params();
    kphp_assert(!params.empty());
    auto variadic_arg = std::prev(params.end());
    auto name_of_variadic_param = VarName(variadic_arg->as<op_func_param>()->var()->get_var_id());
    W << "if (!" << name_of_variadic_param << ".is_vector())" << BEGIN;
    W << "php_warning(\"pass associative array(" << name_of_variadic_param << ") to variadic function: " << FunctionName(func) << "\");" << NL;
    W << name_of_variadic_param << " = f$array_values(" << name_of_variadic_param << ");" << NL;
    W << END << NL;
  }
  W << AsSeq(func_root->cmd()) << END << NL;
}

struct StrlenInfo {
  VertexPtr v;
  int len{0};
  bool str_flag{false};
  bool var_flag{false};
  string str;
};

static bool can_save_ref(VertexPtr v) {
  if (v->type() == op_var) {
    return true;
  }
  if (v->type() == op_func_call) {
    FunctionPtr func = v.as<op_func_call>()->get_func_id();
    if (func->is_extern()) {
      //todo
      return false;
    }
    return true;
  }
  return false;
}

void compile_string_build_as_string(VertexAdaptor<op_string_build> root, CodeGenerator &W) {
  vector<StrlenInfo> info(root->size());
  bool ok = true;
  bool was_dynamic = false;
  bool was_object = false;
  int static_length = 0;
  int ii = 0;
  for (auto i : root->args()) {
    info[ii].v = i;
    VertexPtr value = GenTree::get_actual_value(i);
    const TypeData *type = tinf::get_type(value);

    int value_length = type_strlen(type);
    if (value_length == STRLEN_ERROR) {
      kphp_error (0, format("Cannot convert type [%s] to string", type_out(type).c_str()));
      ok = false;
      ii++;
      continue;
    }

    if (value->type() == op_string) {
      info[ii].str_flag = true;
      info[ii].str = value->get_string();
      info[ii].len = (int)info[ii].str.size();
      static_length += info[ii].len;
    } else {
      if (value_length == STRLEN_DYNAMIC) {
        was_dynamic = true;
      } else if (value_length == STRLEN_OBJECT) {
        was_object = true;
      } else {
        if (value_length & STRLEN_WARNING_FLAG) {
          value_length &= ~STRLEN_WARNING_FLAG;
          kphp_warning (format("Suspicious convertion of type [%s] to string", type_out(type).c_str()));
        }

        kphp_assert (value_length >= 0);
        static_length += value_length;
      }

      info[ii].len = value_length;
    }

    ii++;
  }
  if (!ok) {
    return;
  }

  bool complex_flag = was_dynamic || was_object;
  string len_name;

  if (complex_flag) {
    W << "(" << BEGIN;
    vector<string> to_add;
    for (int i = 0; i < (int)info.size(); i++) {
      if (info[i].str_flag) {
        continue;
      }
      if (info[i].len == STRLEN_DYNAMIC || info[i].len == STRLEN_OBJECT) {
        string var_name = gen_unique_name("var");

        if (info[i].len == STRLEN_DYNAMIC) {
          bool can_save_ref_flag = can_save_ref(info[i].v);
          W << "const " << TypeName(tinf::get_type(info[i].v)) << " " <<
            (can_save_ref_flag ? "&" : "") <<
            var_name << "=" << info[i].v << ";" << NL;
        } else if (info[i].len == STRLEN_OBJECT) {
          W << "const string " << var_name << " = f$strval" <<
            "(" << info[i].v << ");" << NL;
        }

        to_add.push_back(var_name);
        info[i].var_flag = true;
        info[i].str = var_name;
      }
    }

    len_name = gen_unique_name("strlen");
    W << "dl::size_type " << len_name << " = " << int_to_str(static_length);
    for (int i = 0; i < (int)to_add.size(); i++) {
      W << " + max_string_size (" << to_add[i] << ")";
    }
    W << ";" << NL;
  }

  W << "string (";
  if (complex_flag) {
    W << len_name;
  } else {
    W << int_to_str(static_length);
  }
  W << ", true)";
  for (int i = 0; i < (int)info.size(); i++) {
    W << ".append_unsafe (";
    if (info[i].str_flag) {
      compile_string_raw(info[i].str, W);
      W << ", " << int_to_str((int)info[i].len);
    } else if (info[i].var_flag) {
      W << info[i].str;
    } else {
      W << info[i].v;
    }
    W << ")";
  }
  W << ".finish_append()";

  if (complex_flag) {
    W << ";" << NL <<
      END << ")";
  }
}

/**
 * Обращения к массиву по константной строке, типа $a['somekey'], хочется заменить не просто на get_value, но
 * ещё и на этапе компиляции посчитать хеш строки, чтобы не делать этого в рантайме.
 * Т.е. нужно проверить, что строка константная, а не $a[$var], не $a[3], не $a['a'.'b'] и т.п.
 * @return int string_hash или 0 (если случайно хеш сам получился 0 — не страшно, просто не заинлайнится)
 */
inline int can_use_precomputed_hash_indexing_array(VertexPtr key) {
  // если это просто ['строка'], которая превратилась в [$const_string$xxx] (ещё могут быть op_concat и другие странности)
  if (key->type() == op_var && key->extra_type == op_ex_var_const && key->get_var_id()->init_val->type() == op_string) {
    const std::string &string_key = key->get_var_id()->init_val->get_string();

    // см. array::get_value()/set_value(): числовые строки обрабатываются отдельной веткой
    int int_val;
    if (php_try_to_int(string_key.c_str(), (int)string_key.size(), &int_val)) {
      return 0;
    }

    return string_hash(string_key.c_str(), (int)string_key.size());
  }

  return 0;
}

void compile_index(VertexAdaptor<op_index> root, CodeGenerator &W) {
  PrimitiveType array_ptype = root->array()->tinf_node.get_type()->ptype();

  switch (array_ptype) {
    case tp_string:
      compile_index_of_string(root, W);
      break;
    case tp_tuple:
      W << TupleGetIndex(root->array(), root->key());
      break;
    default:
      compile_index_of_array(root, W);
  }
}

void compile_index_of_array(VertexAdaptor<op_index> root, CodeGenerator &W) {
  bool used_as_rval = root->rl_type != val_l;
  if (!used_as_rval) {
    kphp_assert(root->has_key());
    W << root->array() << "[" << root->key() << "]";
  } else {
    W << root->array() << ".get_value (" << root->key();
    // если это обращение по константной строке, типа $a['somekey'],
    // вычисляем хеш строки 'somekey' на этапе компиляции, и вызовем array<T>::get_value(string, precomputed_hash)
    int precomputed_hash = can_use_precomputed_hash_indexing_array(root->key());
    if (precomputed_hash) {
      W << ", " << int_to_str(precomputed_hash);
    }
    W << ")";
  }
}

void compile_index_of_string(VertexAdaptor<op_index> root, CodeGenerator &W) {
  kphp_assert(root->has_key());

  W << root->array() << ".get_value(" << root->key() << ")";
}

void compile_instance_prop(VertexAdaptor<op_instance_prop> root, CodeGenerator &W) {
  W << root->instance() << "->$" << root->get_string();
}

void compile_seq_rval(VertexPtr root, CodeGenerator &W) {
  kphp_assert(root->size());

  W << "(" << BEGIN;        // gcc конструкция: ({ ...; v$result_var; })
  for (auto i : *root) {
    W << i << ";" << NL;    // последнее выражение тут — результат
  }
  W << END << ")";
}

void compile_as_printable(VertexPtr root, CodeGenerator &W) {
  if (auto conv = root.try_as<op_conv_string>()) {
    if (conv->expr()->type() == op_string) {
      root = conv->expr();
    }
  }

  if (root->type() == op_string) {
    compile_string(root.as<op_string>(), W);
    return;
  }

  if (root->type() == op_string_build) {
    compile_string_build_as_string(root.as<op_string_build>(), W);
    return;
  }

  if (auto conv = root.try_as<op_conv_string>()) {
    if (conv->expr()->type() == op_string_build) {
      compile_as_printable(conv->expr().as<op_string_build>(), W);
      return;
    }
  }

  if (root->type() != op_conv_string) {
    W << "(";
  }
  W << root;
  if (root->type() != op_conv_string) {
    W << ")";
  }
}


void compile_echo(VertexPtr root, CodeGenerator &W) {
  bool first = true;

  for (auto i : *root) {
    if (first) {
      first = false;
    } else {
      W << ";" << NL;
    }

    if (root->type() == op_dbg_echo) {
      W << "dbg_echo (";
    } else {
      W << "print (";
    }
    compile_as_printable(i, W);
    W << ")";
  }
}


void compile_var_dump(VertexPtr root, CodeGenerator &W) {
  bool first = true;
  for (auto i : *root) {
    if (first) {
      first = false;
    } else {
      W << ";" << NL;
    }

    W << "f$var_dump (" << i << ")";
  }
}


void compile_print(VertexAdaptor<op_print> root, CodeGenerator &W) {
  W << "print (";
  compile_as_printable(root->expr(), W);
  W << ")";
}

void compile_xset(VertexAdaptor<meta_op_xset> root, CodeGenerator &W) {
  assert ((int)root->size() == 1 || root->type() == op_unset);

  bool first = true;
  for (auto arg : root->args()) {
    if (first) {
      first = false;
    } else {
      W << ";" << NL;
    }

    if (root->type() == op_unset && arg->type() == op_var) {
      W << "unset (" << arg << ")";
      continue;
    }
    if (root->type() == op_isset && arg->type() == op_var) {
      W << "(!f$is_null(" << arg << "))";
      continue;
    }
    if (auto index = arg.try_as<op_index>()) {
      kphp_assert (index->has_key());
      VertexPtr arr = index->array(), id = index->key();
      W << "(" << arr;
      if (root->type() == op_isset) {
        W << ").isset (";
      } else if (root->type() == op_unset) {
        W << ").unset (";
      } else {
        assert (0);
      }
      W << id << ")";
      continue;
    }
    kphp_error (0, "Some problems with isset/unset");
  }
}


void compile_list(VertexAdaptor<op_list> root, CodeGenerator &W) {
  VertexPtr arr = root->array();
  VertexRange list = root->list();
  PrimitiveType ptype = tinf::get_type(arr)->get_real_ptype();
  kphp_assert(vk::any_of_equal(ptype, tp_array, tp_var, tp_tuple));

  for (int i = list.size() - 1; i >= 0; --i) {    // именно в обратную сторону, поведение как в php
    VertexPtr cur = list[i];
    if (cur->type() != op_lvalue_null) {

      if (ptype != tp_tuple) {
        W << "assign (" << cur << ", " << arr << ".get_value (" << int_to_str(i) << "));" << NL;
      } else {
        W << "assign (" << cur << ", " << TupleGetIndex(arr, int_to_str(i)) << ");" << NL;
      }
    }
  }
}


void compile_array(VertexAdaptor<op_array> root, CodeGenerator &W) {
  int n = (int)root->args().size();
  const TypeData *type = tinf::get_type(root);

  if (n == 0) {
    W << TypeName(type) << " ()";
    return;
  }

  bool has_double_arrow = false;
  int int_cnt = 0, string_cnt = 0, xx_cnt = 0;
  for (auto i : root->args()) {
    if (auto arrow = i.try_as<op_double_arrow>()) {
      has_double_arrow = true;
      VertexPtr key = arrow->key();
      PrimitiveType tp = tinf::get_type(key)->ptype();
      if (tp == tp_int) {
        int_cnt++;
      } else {
        VertexPtr key_val = GenTree::get_actual_value(key);
        if (tp == tp_string && key_val->type() == op_string) {
          string key = key_val.as<op_string>()->str_val;
          if (php_is_int(key.c_str(), (int)key.size())) {
            int_cnt++;
          } else {
            string_cnt++;
          }
        } else {
          xx_cnt++;
        }
      }
    } else {
      int_cnt++;
    }
  }
  if (n <= 10 && !has_double_arrow && type->ptype() == tp_array && root->extra_type != op_ex_safe_version) {
    W << TypeName(type) << "::create(" << AsList(root, ", ") << ")";
    return;
  }

  W << "(" << BEGIN;

  string arr_name = "tmp_array";
  W << TypeName(type) << " " << arr_name << " = ";

  //TODO: check
  if (type->ptype() == tp_array) {
    W << TypeName(type);
  } else {
    W << "array <var>";
  }
  char tmp[70];
  sprintf(tmp, " (array_size (%d, %d, %s));", int_cnt + xx_cnt, string_cnt + xx_cnt, has_double_arrow ? "false" : "true");
  W << (const char *)tmp << NL;

  for (auto cur : root->args()) {
    W << arr_name;
    if (auto arrow = cur.try_as<op_double_arrow>()) {
      W << ".set_value (" << arrow->key() << ", " << arrow->value();
      int precomputed_hash = can_use_precomputed_hash_indexing_array(arrow->key());
      if (precomputed_hash) {
        W << ", " << int_to_str(precomputed_hash);
      }
      W << ")";
    } else {
      W << ".push_back (" << cur << ")";
    }

    W << ";" << NL;
  }

  W << arr_name << ";" << NL <<
    END << ")";
}

void compile_tuple(VertexAdaptor<op_tuple> root, CodeGenerator &W) {
  W << "std::make_tuple(";
  VertexRange args = root->args();
  for (int i = 0; i < args.size(); ++i) {
    if (i) {
      W << ", ";
    }
    W << args[i];
  }
  W << ")";
}

void compile_func_call_fast(VertexAdaptor<op_func_call> root, CodeGenerator &W) {
  if (!root->get_func_id()->can_throw) {
    compile_func_call(root, W);
    return;
  }
  bool is_void = root->rl_type == val_none;

  if (is_void) {
    W << "TRY_CALL_VOID_ (";
  } else {
    const TypeData *type = tinf::get_type(root);
    W << "TRY_CALL_ (" << TypeNameInsideMacro(type) << ", ";
  }

  W.get_context().catch_labels.push_back("");
  compile_func_call(root, W);
  W.get_context().catch_labels.pop_back();

  W << ", ";
  compile_throw_action(W);
  W << ")";
}

void compile_func_call(VertexAdaptor<op_func_call> root, CodeGenerator &W, int state) {
  FunctionPtr func;
  if (root->extra_type == op_ex_internal_func) {
    W << root->str_val;
  } else {
    func = root->get_func_id();
    if (state != 1 && state != 2 && W.get_context().resumable_flag && func->is_resumable) {
      kphp_error (0, format("Can't compile resumable function [%s] without async\n"
                             "Function is resumable because of calls chain:\n%s\n", func->get_human_readable_name().c_str(), func->get_resumable_path().c_str()));
    }

    if (state == 2) {
      W << FunctionForkName(func);
    } else {
      W << FunctionName(func);
    }
  }
  if (func && func->cpp_template_call) {
    const TypeData *tp = tinf::get_type(root);
    W << "< " << TypeName(tp) << " >";
  }
  W << "(";
  auto i = root->args().begin();
  auto i_end = root->args().end();
  bool first = true;
  for (; i != i_end; ++i) {
    if (first) {
      first = false;
    } else {
      W << ", ";
    }

    W << *i;
  }
  W << ")";
}

void compile_func_ptr(VertexAdaptor<op_func_ptr> root, CodeGenerator &W) {
  if (root->str_val == "boolval") {
    W << "(bool (*) (const var &))";
  }
  if (root->str_val == "intval") {
    W << "(int (*) (const var &))";
  }
  if (root->str_val == "floatval") {
    W << "(double (*) (const var &))";
  }
  if (root->str_val == "strval") {
    W << "(string (*) (const var &))";
  }
  if (root->str_val == "is_numeric" ||
      root->str_val == "is_null" ||
      root->str_val == "is_bool" ||
      root->str_val == "is_int" ||
      root->str_val == "is_float" ||
      root->str_val == "is_scalar" ||
      root->str_val == "is_string" ||
      root->str_val == "is_array" ||
      root->str_val == "is_object" ||
      root->str_val == "is_integer" ||
      root->str_val == "is_long" ||
      root->str_val == "is_double" ||
      root->str_val == "is_real") {
    W << "(bool (*) (const var &))";
  }
  if (root->get_func_id()->is_lambda()) {
    /**
     * KPHP code like this:
     *   array_map(function ($x) { return $x; }, ['a', 'b']);
     *
     * Will be transformed to:
     *   array_map(({
     *     auto bound_class = anon$$__construct();
     *     [bound_class] (string x) {
     *       return anon$$__invoke(bound_class, std::move(x));
     *     };
     *   }), const_array);
     */
    FunctionPtr invoke_method = root->get_func_id();
    W << "(" << BEGIN;
    {
      W << "auto bound_class = " << root->bound_class() << ";" << NL;
      VertexRange params = invoke_method->root->params().as<op_func_param_list>()->params();
      kphp_assert(!params.empty());
      VertexRange params_without_first(std::next(params.begin()), params.end());
      W << "[bound_class] (" << FunctionParams(invoke_method, 1u, true) << ") " << BEGIN;
      {
        W << "return " << FunctionName(invoke_method) << "(bound_class";
        for (auto param : params_without_first) {
          W << ", std::move(" << VarName(param.as<op_func_param>()->var()->get_var_id()) << ")";
        }
        W << ");" << NL;
      }
      W << END << ";";
    }
    W << END << ")" << NL;
  } else {
    W << FunctionName(root->get_func_id());
  }
}


void compile_define_val(VertexPtr root, CodeGenerator &W) {
  DefinePtr d = root.as<op_define_val>()->define_id;

  W << LockComments();
  switch (d->val->type()) {
    case op_int_const:
    case op_uint_const:
    case op_long_const:
    case op_ulong_const:
    case op_float_const:
    case op_string:
    case op_false:
    case op_true:
    case op_null:
    case op_array:
      W << d->val;
      break;

    default:
      W << "(" << d->val << ")";
      break;
  }
  W << UnlockComments();
}


void compile_defined(VertexPtr root __attribute__((unused)), CodeGenerator &W __attribute__((unused))) {
  W << "false";
  //TODO: it is not CodeGen part
}


void compile_safe_version(VertexPtr root, CodeGenerator &W) {
  if (auto set_value = root.try_as<op_set_value>()) {
    W << "SAFE_SET_VALUE (" <<
      set_value->array() << ", " <<
      set_value->key() << ", " <<
      TypeNameInsideMacro(tinf::get_type(set_value->key())) << ", " <<
      set_value->value() << ", " <<
      TypeNameInsideMacro(tinf::get_type(set_value->value())) <<
      ")";
  } else if (OpInfo::rl(root->type()) == rl_set) {
    auto op = root.as<meta_op_binary>();
    if (OpInfo::type(root->type()) == binary_func_op) {
      W << "SAFE_SET_FUNC_OP (";
    } else if (OpInfo::type(root->type()) == binary_op) {
      W << "SAFE_SET_OP (";
    } else {
      kphp_fail();
    }
    W << op->lhs() << ", " <<
      OpInfo::str(root->type()) << ", " <<
      op->rhs() << ", " <<
      TypeNameInsideMacro(tinf::get_type(op->rhs())) <<
      ")";
  } else if (auto pb = root.try_as<op_push_back>()) {
    W << "SAFE_PUSH_BACK (" <<
      pb->array() << ", " <<
      pb->value() << ", " <<
      TypeNameInsideMacro(tinf::get_type(pb->value())) <<
      ")";
  } else if (auto pb = root.try_as<op_push_back_return>()) {
    W << "SAFE_PUSH_BACK_RETURN (" <<
      pb->array() << ", " <<
      pb->value() << ", " <<
      TypeNameInsideMacro(tinf::get_type(pb->value())) <<
      ")";
  } else if (root->type() == op_array) {
    compile_array(root.as<op_array>(), W);
    return;
  } else if (auto index = root.try_as<op_index>()) {
    kphp_assert (index->has_key());
    W << "SAFE_INDEX (" <<
      index->array() << ", " <<
      index->key() << ", " <<
      TypeNameInsideMacro(tinf::get_type(index->key())) <<
      ")";
  } else {
    kphp_error (0, format("Safe version of [%s] is not supported", OpInfo::str(root->type()).c_str()));
    kphp_fail();
  }

}


void compile_set_value(VertexAdaptor<op_set_value> root, CodeGenerator &W) {
  W << "(" << root->array() << ").set_value (" << root->key() << ", " << root->value();
  int precomputed_hash = can_use_precomputed_hash_indexing_array(root->key());
  if (precomputed_hash) {
    W << ", " << int_to_str(precomputed_hash);
  }
  W << ")";
  // set_value для string/tuple нет отдельных (в отличие от compile_index()), т.к. при использовании их как lvalue
  // строка обобщается до var, а кортеж ругается, и тут остаётся только array/var
}

void compile_push_back(VertexAdaptor<op_push_back> root, CodeGenerator &W) {
  W << "(" << root->array() << ").push_back (" << root->value() << ")";
}

void compile_push_back_return(VertexAdaptor<op_push_back_return> root, CodeGenerator &W) {
  W << "(" << root->array() << ").push_back_return (" << root->value() << ")";
}


void compile_string_raw(const string &str, CodeGenerator &W) {
  W << "\"";
  for (int i = 0; i < (int)str.size(); i++) {
    switch (str[i]) {
      case '\r':
        W << "\\r";
        break;
      case '\n':
        W << "\\n";
        break;
      case '"':
        W << "\\\"";
        break;
      case '\\':
        W << "\\\\";
        break;
      case '\'':
        W << "\\\'";
        break;
      case 0: {
        if (str[i + 1] < '0' || str[i + 1] > '9') {
          W << "\\0";
        } else {
          W << "\\000";
        }
        break;
      }
      case '\a':
        W << "\\a";
        break;
      case '\b':
        W << "\\b";
        break;
      case '\f':
        W << "\\f";
        break;
      case '\t':
        W << "\\t";
        break;
      case '\v':
        W << "\\v";
        break;
      default:
        if ((unsigned char)str[i] < 32) {
          string tmp = "\\0";
          tmp += (char)('0' + (str[i] / 8));
          tmp += (char)('0' + (str[i] % 8));
          W << tmp;
        } else {
          W << str[i];
        }
        break;
    }
  }
  W << "\"";
}

void compile_string_raw(VertexAdaptor<op_string> root, CodeGenerator &W) {
  const string &str = root->str_val;
  compile_string_raw(str, W);
}


void compile_string(VertexAdaptor<op_string> root, CodeGenerator &W) {
  W << "string (";
  compile_string_raw(root, W);
  W << ", " << int_to_str((int)root->str_val.size()) << ")";
}


void compile_string_build(VertexAdaptor<op_string_build> root, CodeGenerator &W) {
  compile_string_build_as_string(root, W);
}


void compile_break_continue(VertexAdaptor<meta_op_goto> root, CodeGenerator &W) {
  if (root->int_val != 0) {
    W << "goto " << LabelName(root->int_val);
  } else {
    if (root->type() == op_break) {
      W << "break";
    } else if (root->type() == op_continue) {
      W << "continue";
    } else {
      assert (0);
    }
  }
}


void compile_conv_array_l(VertexAdaptor<op_conv_array_l> root, CodeGenerator &W) {
  VertexPtr val = root->expr();
  PrimitiveType tp = tinf::get_type(val)->get_real_ptype();
  if (tp == tp_array || tp == tp_var) {
    std::string fun_name = "unknown";
    if (auto cur_fun = stage::get_function()) {
      fun_name = cur_fun->get_human_readable_name();
    }
    W << "arrayval_ref (" << val << ", R\"(" << fun_name << ")\", -1)";
  } else {
    kphp_error (0, "Trying to pass non-array as reference to array");
  }
}


void compile_conv_int_l(VertexAdaptor<op_conv_int_l> root, CodeGenerator &W) {
  VertexPtr val = root->expr();
  PrimitiveType tp = tinf::get_type(val)->get_real_ptype();
  if (tp == tp_int || tp == tp_var) {
    W << "intval_ref (" << val << ")";
  } else {
    kphp_error (0, "Trying to pass non-int as reference to int");
  }
}


void compile_cycle_op(VertexPtr root, CodeGenerator &W) {
  Operation tp = root->type();
  switch (tp) {
    case op_while:
      compile_while(root.as<op_while>(), W);
      break;
    case op_do:
      compile_do(root.as<op_do>(), W);
      break;
    case op_for:
      compile_for(root.as<op_for>(), W);
      break;
    case op_foreach:
      compile_foreach(root.as<op_foreach>(), W);
      break;
    case op_switch:
      compile_switch(root.as<op_switch>(), W);
      break;
    default:
      assert (0);
      break;
  }
}


void compile_min_max(VertexPtr root, CodeGenerator &W) {
  W << OpInfo::str(root->type()) << "< " << TypeName(tinf::get_type(root)) << " > (" <<
    AsList(root, ", ") <<
    ")";
}

void compile_power(VertexAdaptor<op_pow> power, CodeGenerator &W) {
  switch (tinf::get_type(power)->ptype()) {
    case tp_int:
      // pow return type with positive constexpr integer exponent and any integer base is inferred as int
      W << "int_power";
      return;
    case tp_float:
      // pow return type with positive constexpr integer exponent and any float base is inferred as float
      W << "float_power";
      return;
    case tp_var:
      // pow return type with any other types in exponent,
      // including negative constexpr integer or unknown integer, is inferred as var
      W << "var_power";
      return;
    default:
      kphp_error(false, "Unexpected power return type");
  }
}

void compile_common_op(VertexPtr root, CodeGenerator &W) {
  Operation tp = root->type();
  string str;
  switch (tp) {
    case op_seq:
      W << BEGIN << AsSeq(root) << END;
      break;
    case op_seq_rval:
      compile_seq_rval(root, W);
      break;

    case op_int_const:
      str = root.as<op_int_const>()->str_val;
      if (str.size() > 9) {
        W << "(int)";
      }
      W << str;
      break;
    case op_uint_const:
      str = root.as<op_uint_const>()->str_val;
      if (str.size() > 9) {
        W << "(unsigned int)";
      }
      W << str << "u";
      break;
    case op_long_const:
      str = root.as<op_long_const>()->str_val;
      if (str.size() > 18) {
        W << "(long long)";
      }
      W << str << "ll";
      break;
    case op_ulong_const:
      str = root.as<op_ulong_const>()->str_val;
      if (str.size() > 18) {
        W << "(unsigned long long)";
      }
      W << str << "ull";
      break;
    case op_float_const:
      str = root.as<op_float_const>()->str_val;
      W << "(double)" << str;
      break;
    case op_false:
      W << "false";
      break;
    case op_true:
      W << "true";
      break;
    case op_null:
      W << "var()";
      break;
    case op_var:
      W << VarName(root->get_var_id());
      break;
    case op_string:
      compile_string(root.as<op_string>(), W);
      break;

    case op_if:
      compile_if(root.as<op_if>(), W);
      break;
    case op_require:
    case op_require_once:
      compile_require(root, W);
      break;
    case op_return:
      compile_return(root.as<op_return>(), W);
      break;
    case op_global:
    case op_static:
      //already processed
      break;
    case op_echo:
    case op_dbg_echo:
      compile_echo(root, W);
      break;
    case op_throw:
      compile_throw(root.as<op_throw>(), W);
      break;
    case op_var_dump:
      compile_var_dump(root, W);
      break;
    case op_print:
      compile_print(root.as<op_print>(), W);
      break;
    case op_min:
    case op_max:
      compile_min_max(root, W);
      break;
    case op_continue:
    case op_break:
      compile_break_continue(root.as<meta_op_goto>(), W);
      break;
    case op_try:
      compile_try(root.as<op_try>(), W);
      break;
    case op_fork:
      compile_fork(root.as<op_fork>(), W);
      break;
    case op_async:
      compile_async(root.as<op_async>(), W);
      break;
    case op_function:
      compile_function(root.as<op_function>(), W);
      break;
    case op_func_call:
    case op_constructor_call:
      compile_func_call_fast(root.as<op_func_call>(), W);
      break;
    case op_func_ptr:
      compile_func_ptr(root.as<op_func_ptr>(), W);
      break;
    case op_string_build:
      compile_string_build(root.as<op_string_build>(), W);
      break;
    case op_index:
      compile_index(root.as<op_index>(), W);
      break;
    case op_instance_prop:
      compile_instance_prop(root.as<op_instance_prop>(), W);
      break;
    case op_isset:
      compile_xset(root.as<meta_op_xset>(), W);
      break;
    case op_list:
      compile_list(root.as<op_list>(), W);
      break;
    case op_array:
      compile_array(root.as<op_array>(), W);
      break;
    case op_tuple:
      compile_tuple(root.as<op_tuple>(), W);
      break;
    case op_unset:
      compile_xset(root.as<meta_op_xset>(), W);
      break;
    case op_empty:
      break;
    case op_define_val:
      compile_define_val(root.as<op_define_val>(), W);
      break;
    case op_defined:
      compile_defined(root.as<op_defined>(), W);
      break;
    case op_conv_array_l:
      compile_conv_array_l(root.as<op_conv_array_l>(), W);
      break;
    case op_conv_int_l:
      compile_conv_int_l(root.as<op_conv_int_l>(), W);
      break;
    case op_set_value:
      compile_set_value(root.as<op_set_value>(), W);
      break;
    case op_push_back:
      compile_push_back(root.as<op_push_back>(), W);
      break;
    case op_push_back_return:
      compile_push_back_return(root.as<op_push_back_return>(), W);
      break;
    case op_noerr:
      compile_noerr(root.as<op_noerr>(), W);
      break;
    case op_clone:
      W << root.as<op_clone>()->expr() << ".clone()";
      break;
    default:
    kphp_fail();
      break;
  }
}


void compile_vertex(VertexPtr root, CodeGenerator &W) {
  OperationType tp = OpInfo::type(root->type());

  W << UpdateLocation(root->location);

  bool close_par = root->val_ref_flag == val_r || root->val_ref_flag == val_l;

  if (root->val_ref_flag == val_r) {
    W << "val(";
  } else if (root->val_ref_flag == val_l) {
    W << "ref(";
  }

  if (root->extra_type == op_ex_safe_version) {
    compile_safe_version(root, W);
  } else {
    switch (tp) {
      case prefix_op:
        compile_prefix_op(root.as<meta_op_unary>(), W);
        break;
      case postfix_op:
        compile_postfix_op(root.as<meta_op_unary>(), W);
        break;
      case binary_op:
      case binary_func_op:
        compile_binary_op(root.as<meta_op_binary>(), W);
        break;
      case ternary_op:
        compile_ternary_op(root.as<op_ternary>(), W);
        break;
      case common_op:
        compile_common_op(root, W);
        break;
      case cycle_op:
        compile_cycle_op(root, W);
        break;
      case conv_op:
        compile_conv_op(root.as<meta_op_unary>(), W);
        break;
      default:
        printf("%d: %d\n", tp, root->type());
        assert (0);
        break;
    }
  }

  if (close_par) {
    W << ")";
  }
}

size_t CodeGenF::calc_count_of_parts(size_t cnt_global_vars) {
  return 1u + cnt_global_vars / 4096u;
}

void CodeGenF::on_finish(DataStream<WriterData *> &os) {
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

  W.init(new WriterCallback(os));

  vector<SrcFilePtr> main_files = G->get_main_files();
  std::unordered_set<FunctionPtr> main_functions(main_files.size());
  for (const SrcFilePtr &main_file : main_files) {
    main_functions.emplace(main_file->main_function);
  }

  auto should_gen_function = [&](FunctionPtr fun) {
    return fun->type != FunctionData::func_class_holder &&
           (
             fun->body_seq != FunctionData::body_value::empty ||
             main_functions.count(fun)
           );
  };

  //TODO: parallelize;
  for (const auto &fun : xall) {
    if (should_gen_function(fun)) {
      prepare_generate_function(fun);
    }
  }
  for (const auto &c : all_classes) {
    if (c && c->does_need_codegen()) {
      prepare_generate_class(c);
    }
  }

  vector<FunctionPtr> all_functions;
  vector<FunctionPtr> exported_functions;
  for (const auto &function : xall) {
    if (!should_gen_function(function)) {
      continue;
    }
    if (function->is_extern()) {
      continue;
    }
    all_functions.push_back(function);
    W << Async(FunctionH(function));
    W << Async(FunctionCpp(function));

    if (function->kphp_lib_export && G->env().is_static_lib_mode()) {
      exported_functions.emplace_back(function);
    }
  }

  for (const auto &c : all_classes) {
    if (c && c->does_need_codegen()) {
      W << Async(ClassDeclaration(c));
    }
  }

  for (const auto &main_file : main_files) {
    W << Async(GlobalVarsReset(main_file));
  }
  W << Async(InitScriptsCpp(std::move(main_files), std::move(all_functions)));

  std::vector<VarPtr> vars = G->get_global_vars();
  for (FunctionPtr fun: xall) {
    vars.insert(vars.end(), fun->static_var_ids.begin(), fun->static_var_ids.end());
  }
  size_t parts_cnt = calc_count_of_parts(vars.size());
  W << Async(VarsCpp(std::move(vars), parts_cnt));

  if (G->env().is_static_lib_mode()) {
    for (FunctionPtr exported_function: exported_functions) {
      W << Async(LibHeaderH(exported_function));
    }
    W << Async(LibHeaderTxt(std::move(exported_functions)));
    W << Async(StaticLibraryRunGlobalHeaderH());
  }

  write_hashes_of_subdirs_to_dep_files(W);

  write_tl_schema(W);
  write_lib_version(W);
}

void CodeGenF::prepare_generate_function(FunctionPtr func) {
  string file_name = func->name;
  std::replace(file_name.begin(), file_name.end(), '$', '@');

  func->header_name = file_name + ".h";
  func->subdir = get_subdir(func->file_id->short_file_name);

  recalc_hash_of_subdirectory(func->subdir, func->header_name);

  if (!func->is_inline) {
    func->src_name = file_name + ".cpp";
    recalc_hash_of_subdirectory(func->subdir, func->src_name);
  }

  func->header_full_name =
    func->is_imported_from_static_lib()
    ? func->file_id->owner_lib->headers_dir() + func->header_name
    : func->subdir + "/" + func->header_name;

  my_unique(&func->static_var_ids);
  my_unique(&func->global_var_ids);
  my_unique(&func->local_var_ids);
}

string CodeGenF::get_subdir(const string &base) {
  int bucket = vk::std_hash(base) % 100;

  return string("o_") + int_to_str(bucket);
}

void CodeGenF::recalc_hash_of_subdirectory(const string &subdir, const string &file_name) {
  auto &cur_hash = subdir_hash[subdir];
  vk::hash_combine(cur_hash, vk::std_hash(file_name));
}

void CodeGenF::write_hashes_of_subdirs_to_dep_files(CodeGenerator &W) {
  for (const auto &dir_and_hash : subdir_hash) {
    W << OpenFile("_dep.cpp", dir_and_hash.first, false);
    char tmp[100];
    sprintf(tmp, "%zx", dir_and_hash.second);
    W << "//" << (const char *)tmp << NL;
    W << CloseFile();
  }
}

void CodeGenF::write_tl_schema(CodeGenerator &W) {
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
  if (!G->env().is_static_lib_mode()) {
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
}

void CodeGenF::write_lib_version(CodeGenerator &W) {
  W << OpenFile("_lib_version.h");
  W << "//" << G->env().get_runtime_sha256() << NL;
  W << CloseFile();
}

void CodeGenF::prepare_generate_class(ClassPtr) {
}
