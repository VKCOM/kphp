#pragma once

#include "compiler/common.h"
#include "compiler/data/data_ptr.h"
#include "compiler/utils/string-utils.h"

struct Line {
  int begin_pos, end_pos;
  SrcFilePtr file;
  bool brk;
  set<int> line_ids;//????
  Line() :
    begin_pos(0),
    end_pos(-1),
    file(),
    brk(false),
    line_ids() {
  }

  explicit Line(int begin_pos) :
    begin_pos(begin_pos),
    end_pos(-1),
    file(),
    brk(false),
    line_ids() {
  }
};

class WriterData {
private:
  vector<Line> lines;
  string text;
  unsigned long long crc;

  vector<string> includes;
  bool compile_with_debug_info_flag;
  bool compile_with_crc_flag;

public:
  string file_name;
  string subdir;

private:
  void write_code(string &dest_str, const Line &line);
  template<class T>
  void dump(string &dest_str, const T &begin, const T &end, SrcFilePtr file);

public:
  explicit WriterData(bool compile_with_debug_info_flag = true, bool compile_with_crc = true);

  inline void append(const char *begin, size_t length) {
    text.append(begin, length);
  }
  inline void append(size_t n, char c) {
    text.append(n, c);
  }
  inline void append(char c) {
    text.push_back(c);
  }

  void begin_line();
  void end_line();
  void brk();
  void add_location(SrcFilePtr file, int line);
  void add_include(const string &s);
  const vector<string> &get_includes();

  unsigned long long calc_crc();
  void dump(string &dest_str);

  void swap(WriterData &other);

  bool compile_with_debug_info() const;
  bool compile_with_crc() const;
};

class WriterCallbackBase {
public:
  virtual ~WriterCallbackBase() {}

  virtual void on_end_write(WriterData *data) = 0;
};

class Writer {
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
private:
  DISALLOW_COPY_AND_ASSIGN (Writer);
};
