#pragma once

#include "common/mixin/not_copyable.h"

#include "compiler/code-gen/writer-data.h"
#include "compiler/common.h"
#include "compiler/data/data_ptr.h"
#include "compiler/utils/string-utils.h"

class WriterCallbackBase {
public:
  virtual ~WriterCallbackBase() {}

  virtual void on_end_write(WriterData &&data) = 0;
};

class Writer : private vk::not_copyable {
private:
  enum state_t {
    w_stopped,
    w_running
  };

  state_t state;

  WriterData data;

  WriterCallbackBase *callback;

  int indent_level;
  bool need_indent;

  int lock_comments_cnt;

  void write_indent();
  void begin_line();
  void end_line();

public:
  Writer();
  ~Writer() = default;

  void set_file_name(const string &file_name, const string &subdir = "");
  void set_callback(WriterCallbackBase *new_callback);

  void begin_write(bool compile_with_debug_info_flag = true, bool compile_with_crc = true);
  void end_write();

  inline void append(const string &s) {
    if (need_indent) {
      write_indent();
    }
    data.append(s.c_str(), s.size());
  }

  inline void append(const char *s) {
    if (need_indent) {
      write_indent();
    }
    data.append(s, strlen(s));
  }

  inline void append(char c) {
    data.append(c);
  }

  inline void append(const vk::string_view &s) {
    if (need_indent) {
      write_indent();
    }
    data.append(s.begin(), s.size());
  }

  void indent(int diff);
  void new_line();
  void brk();
  void add_location(SrcFilePtr file_id, int line_num);
  void lock_comments();
  void unlock_comments();
  bool is_comments_locked();
  void add_include(const string &s);
  void add_lib_include(const string &s);
};
