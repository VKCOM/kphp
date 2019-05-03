#pragma once

#include <memory>

#include "common/smart_ptrs/make_unique.h"

#include "compiler/code-gen/code-generator.h"
#include "compiler/code-gen/writer.h"
#include "compiler/pipes/code-gen/code-gen.h"
#include "compiler/stage.h"
#include "compiler/threading/tls.h"

#define NL NewLine()
#define BEGIN OpenBlock()
#define END CloseBlock()


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

struct OpenBlock {
  inline void compile(CodeGenerator &W) const;
};

struct CloseBlock {
  inline void compile(CodeGenerator &W) const;
};

struct ExternInclude {
  inline explicit ExternInclude(const vk::string_view &file_name);
  inline void compile(CodeGenerator &W) const;

protected:
  vk::string_view file_name;
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

inline OpenFile::OpenFile(const string &file_name, const string &subdir,
                          bool compile_with_debug_info_flag, bool compile_with_crc) :
  file_name(file_name),
  subdir(subdir),
  compile_with_debug_info_flag(compile_with_debug_info_flag),
  compile_with_crc(compile_with_crc) {
}

inline void OpenFile::compile(CodeGenerator &W) const {
  W.create_writer();
  W.get_writer().begin_write(compile_with_debug_info_flag, compile_with_crc);
  W.get_writer().set_file_name(file_name, subdir);
}

inline void CloseFile::compile(CodeGenerator &W) const {
  W.get_writer().end_write();
  W.clear_writer();
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

inline void OpenBlock::compile(CodeGenerator &W) const {
  W << "{" << NL << Indent(+2);
}

inline void CloseBlock::compile(CodeGenerator &W) const {
  W << Indent(-2) << "}";
}

inline ExternInclude::ExternInclude(const vk::string_view &file_name) :
  file_name(file_name) {
}

inline void ExternInclude::compile(CodeGenerator &W) const {
  W << "#include \"" << file_name << "\"" << NL;
}

inline void Include::compile(CodeGenerator &W) const {
  W.get_writer().add_include(static_cast<std::string>(file_name));
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
