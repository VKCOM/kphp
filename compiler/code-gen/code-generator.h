// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <string>
#include <vector>

#include "common/algorithms/simd-int-to-string.h"
#include "common/wrappers/fmt_format.h"

#include "compiler/code-gen/writer-data.h"
#include "compiler/data/data_ptr.h"
#include "compiler/threading/data-stream.h"

struct CGContext {
  std::vector<std::string> catch_labels;
  std::vector<int> catch_label_used;
  FunctionPtr parent_func;
  bool resumable_flag{false};
  bool namespace_opened{false};
  int inside_macro{0};
  size_t inside_null_coalesce_fallback{0};
};

class CodeGenerator {
private:
  WriterData *data{nullptr};
  DataStream<WriterData *> &os;
  CGContext context;

  int indent_level;
  bool need_indent;
  int lock_comments_cnt;

public:

  explicit CodeGenerator(DataStream<WriterData *> &os): os(os) {}
  ~CodeGenerator() = default;

  CodeGenerator(const CodeGenerator &from) = delete;
  CodeGenerator &operator=(const CodeGenerator &) = delete;

  void open_file_create_writer(bool compile_with_debug_info_flag, bool compile_with_crc, const std::string &file_name, const std::string &subdir);
  void close_file_clear_writer();

  void append(char c) {
    data->append(c);
  }

  void append(const char *p, size_t len) {
    if (need_indent) {
      need_indent = false;
      data->append(indent_level, ' ');
    }
    data->append(p, len);
  }

  void append(long long value) {
    char buf[32];
    data->append(buf, static_cast<size_t>(simd_int64_to_string(value, buf) - buf));
  }

  void append(unsigned long long value) {
    char buf[32];
    data->append(buf, static_cast<size_t>(simd_uint64_to_string(value, buf) - buf));
  }

  void append(int value) {
    char buf[16];
    data->append(buf, static_cast<size_t>(simd_int32_to_string(value, buf) - buf));
  }

  void append(unsigned int value) {
    char buf[16];
    data->append(buf, static_cast<size_t>(simd_uint32_to_string(value, buf) - buf));
  }


  void indent(int diff) {
    indent_level += diff;
  }

  void new_line() {
    data->end_line();
    data->begin_line();
    need_indent = true;
  }

  void add_location(SrcFilePtr file, int line_num) {
    if (!is_comments_locked()) {
      data->add_location(file, line_num);
    }
  }

  bool is_comments_locked() { return lock_comments_cnt > 0; }
  void lock_comments() { lock_comments_cnt++; data->brk(); }
  void unlock_comments() { lock_comments_cnt--; }

  void add_include(const std::string &s) { data->add_include(s); }
  void add_lib_include(const std::string &s) { data->add_lib_include(s); }

  CGContext &get_context() {
    return context;
  }
};

template<class T, class = decltype(&T::compile)>
CodeGenerator &operator<<(CodeGenerator &c, const T &value) {
  value.compile(c);
  return c;
}

inline CodeGenerator &operator<<(CodeGenerator &c, const std::string &value) {
  c.append(value.c_str(), value.size());
  return c;
}

inline CodeGenerator &operator<<(CodeGenerator &c, const char *value) {
  c.append(value, strlen(value));
  return c;
}

inline CodeGenerator &operator<<(CodeGenerator &c, const vk::string_view &value) {
  c.append(value.data(), value.size());
  return c;
}

inline CodeGenerator &operator<<(CodeGenerator &c, long long value) {
  c.append(value);
  return c;
}

inline CodeGenerator &operator<<(CodeGenerator &c, long value) {
  c.append(static_cast<long long>(value));
  return c;
}

inline CodeGenerator &operator<<(CodeGenerator &c, unsigned long long value) {
  c.append(value);
  return c;
}

inline CodeGenerator &operator<<(CodeGenerator &c, unsigned long value) {
  c.append(static_cast<unsigned long long>(value));
  return c;
}

inline CodeGenerator &operator<<(CodeGenerator &c, int value) {
  c.append(value);
  return c;
}

inline CodeGenerator &operator<<(CodeGenerator &c, unsigned int value) {
  c.append(value);
  return c;
}

inline CodeGenerator& operator<<(CodeGenerator &c, char value) {
  c.append(value);
  return c;
}

