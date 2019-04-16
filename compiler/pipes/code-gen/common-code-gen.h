#pragma once
#include "compiler/pipes/code-gen/code-gen.h"
#include "compiler/threading/tls.h"
#include "compiler/stage.h"

#define NL NewLine()
#define BEGIN OpenBlock()
#define END CloseBlock()

struct CGContext {
  vector<string> catch_labels;
  vector<int> catch_label_used;
  FunctionPtr parent_func;
  bool use_safe_integer_arithmetic{false};
  bool resumable_flag{false};
  bool namespace_opened{false};
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

  CodeGenerator &operator=(const CodeGenerator &) = delete;

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

  inline void lock_writer();
  inline void unlock_writer();

  inline CodeGenerator &operator<<(char *s);
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

struct VertexCompiler {
  VertexPtr vertex;
  inline VertexCompiler(VertexPtr vertex);
  inline void compile(CodeGenerator &W) const;
};

void compile_vertex(VertexPtr root, CodeGenerator &W);

/*** Implementation ***/
inline VertexCompiler::VertexCompiler(VertexPtr vertex) :
  vertex(vertex) {
}

inline void VertexCompiler::compile(CodeGenerator &W) const {
  compile_vertex(vertex, W);
}

inline CodeGenerator &CodeGenerator::operator<<(char *s) {
  get_writer().append(s);
  return *this;
}

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

inline void CodeGenerator::lock_writer() {
  assert (writer == nullptr);
  writer = master_writer->lock_get();
}

inline void CodeGenerator::unlock_writer() {
  assert (writer != nullptr);
  master_writer->unlock_get(writer);
  writer = nullptr;
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

void compile_string_raw(const string &str, CodeGenerator &W);

template <typename Container,
  typename = decltype(std::declval<Container>().begin()),
  typename = decltype(std::declval<Container>().end())>
void compile_raw_data(CodeGenerator &W, std::vector<int> &const_string_shifts, const Container &values) {
  std::string raw_data;
  const_string_shifts.resize(values.size());
  int ii = 0;
  for (const auto &s : values) {
    int shift_to_align = (((int)raw_data.size() + 7) & -8) - (int)raw_data.size();
    if (shift_to_align != 0) {
      raw_data.append(shift_to_align, 0);
    }
    int raw_len = string_raw_len(static_cast<int>(s.size()));
    kphp_assert (raw_len != -1);
    const_string_shifts[ii] = (int)raw_data.size();
    raw_data.append(raw_len, 0);
    int err = string_raw(&raw_data[const_string_shifts[ii]], raw_len, s.c_str(), (int)s.size());
    kphp_assert (err == raw_len);
    ii++;
  }
  if (!raw_data.empty()) {
    W << "static const char *raw = ";
    compile_string_raw(raw_data, W);
    W << ";" << NL;
  }
}