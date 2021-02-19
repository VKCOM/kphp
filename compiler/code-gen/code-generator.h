// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <string>
#include <vector>

#include "common/algorithms/simd-int-to-string.h"
#include "common/php-functions.h"
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

class File;

// CodeGenerator is created for every root codegen command as an async task, see code-gen-task.h
// usually one command (one CodeGenerator) produces one file (function cpp / function h for example),
// but generally it can produce many files (calling open and close multiple times, a tl schema compiler for example)
class CodeGenerator {
private:
  // a CodeGenerator is used consequently in two steps:
  // 1) we generate contents of every file, but DO NOT STORE contents and locations — just calculate hashes on the fly
  //    if calculated hashes differ to actually stored on disk, the same codegen command is passed is launched again:
  // 2) we do the same codegeneration (for command that emerged diff) and STORE contents and php comments
  // this reduces memory usage, as typically there are a few changes in php code between kphp launches
  bool is_step_just_calc_hashes;
  int diff_files_count{0};

  // hashes of the currently opened file, calculated on the fly
  // (one codegen command may produce multiple files, they are set to 0 on opening a new one)
  unsigned long long hash_of_cpp;
  unsigned long long hash_of_comments;
  File *cur_file{nullptr};

  WriterData *data{nullptr};      // stored contents, is created only on step 2 (re-generating diff files)
  DataStream<WriterData *> &os;   // output stream for stored contents, used only on step 2
  
  CGContext context;

  int indent_level;
  bool need_indent;
  int lock_comments_cnt;

  void feed_hash(unsigned long long val) {
    hash_of_cpp = hash_of_cpp * 56235515617499ULL + val;
  }

  void feed_hash_of_comments(SrcFilePtr file, int line_num);

public:

  explicit CodeGenerator(bool is_step_just_calc_hashes, DataStream<WriterData *> &os)
    : is_step_just_calc_hashes(is_step_just_calc_hashes)
    , os(os) {}
  ~CodeGenerator() = default;

  CodeGenerator(const CodeGenerator &from) = delete;
  CodeGenerator &operator=(const CodeGenerator &) = delete;

  void open_file_create_writer(bool compile_with_debug_info_flag, bool compile_with_crc, const std::string &file_name, const std::string &subdir);
  void close_file_clear_writer();

  void append(char c) {
    feed_hash(c);
    if (!is_step_just_calc_hashes) {
      data->append(c);
    }
  }

  void append(const char *p, size_t len) {
    if (need_indent) {
      need_indent = false;
      feed_hash(static_cast<unsigned long long>(' ') * indent_level);
      if (!is_step_just_calc_hashes) {
        data->append(indent_level, ' ');
      }
    }
    feed_hash(string_hash(p, len));
    if (!is_step_just_calc_hashes) {
      data->append(p, len);
    }
  }

  void append(long long value) {
    feed_hash(value);
    if (!is_step_just_calc_hashes) {
      char buf[32];
      data->append(buf, static_cast<size_t>(simd_int64_to_string(value, buf) - buf));
    }
  }

  void append(unsigned long long value) {
    feed_hash(value);
    if (!is_step_just_calc_hashes) {
      char buf[32];
      data->append(buf, static_cast<size_t>(simd_uint64_to_string(value, buf) - buf));
    }
  }

  void append(int value) {
    feed_hash(value);
    if (!is_step_just_calc_hashes) {
      char buf[16];
      data->append(buf, static_cast<size_t>(simd_int32_to_string(value, buf) - buf));
    }
  }

  void append(unsigned int value) {
    feed_hash(value);
    if (!is_step_just_calc_hashes) {
      char buf[16];
      data->append(buf, static_cast<size_t>(simd_uint32_to_string(value, buf) - buf));
    }
  }


  void indent(int diff) {
    indent_level += diff;
  }

  void new_line() {
    feed_hash('\n');
    if (!is_step_just_calc_hashes) {
      data->end_line();
      data->begin_line();
    }
    need_indent = true;
  }

  void add_location(SrcFilePtr file, int line_num) {
    if (!is_comments_locked()) {
      feed_hash_of_comments(file, line_num);
      if (!is_step_just_calc_hashes) {
        data->add_location(file, line_num);
      }
    }
  }

  bool is_comments_locked() {
    return lock_comments_cnt > 0;
  }
  void lock_comments() {
    lock_comments_cnt++;
    if (!is_step_just_calc_hashes) {
      data->brk();
    }
  }
  void unlock_comments() {
    lock_comments_cnt--;
  }

  void add_include(const std::string &s);
  void add_lib_include(const std::string &s);

  CGContext &get_context() {
    return context;
  }

  bool was_diff_in_any_file() {
    return diff_files_count > 0;
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

