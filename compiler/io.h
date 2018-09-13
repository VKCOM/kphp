#pragma once

#include "compiler/common.h"

#include "compiler/data_ptr.h"
#include "compiler/utils.h"

class SrcFile {
public:
  int id;
  string prefix;
  string text, file_name, short_file_name;
  string unified_file_name;
  string unified_dir_name;
  bool loaded;
  bool is_required;

  string main_func_name;
  FunctionPtr main_function;
  FunctionPtr req_id;

  vector<string_ref> lines;

  string class_context;

  SrcFile();
  SrcFile(const string &file_name, const string &short_file_name, const string &class_context);
  void add_prefix(const string &s);
  bool load();

  string_ref get_line(int id);
};

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

public:
  string file_name;
  string subdir;

private:
  void write_code(string &dest_str, const Line &line);
  template<class T>
  void dump(string &dest_str, T begin, T end, SrcFilePtr file);

public:
  explicit WriterData(bool compile_with_debug_info_flag = true);

  void append(const char *begin, size_t length);
  void append(size_t n, char c);
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

  void write_indent(void);
  void append(const char *begin, size_t length);
  void append(size_t n, char c);
  void begin_line();
  void end_line();

public:
  Writer();
  ~Writer();

  void set_file_name(const string &file_name, const string &subdir = "");
  void set_callback(WriterCallbackBase *new_callback);

  void begin_write(bool compile_with_debug_info_flag = true);
  void end_write();

  void operator()(const string &s);
  void operator()(const char *s);
  void operator()(const string_ref &s);
  void indent(int diff);
  void new_line();
  void brk();
  void operator()(SrcFilePtr file_id, int line_num);
  void lock_comments();
  void unlock_comments();
  bool is_comments_locked();
  void add_include(const string &s);
private:
  DISALLOW_COPY_AND_ASSIGN (Writer);
};
