#pragma once

#include "compiler/data/data_ptr.h"

class WriterData {
private:
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

  vector<Line> lines;
  string text;
  unsigned long long crc;

  vector<string> includes;
  vector<string> lib_includes;
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

  void append(const char *begin, size_t length) {
    text.append(begin, length);
  }
  void append(size_t n, char c) {
    text.append(n, c);
  }
  void append(char c) {
    text.push_back(c);
  }

  void begin_line();
  void end_line();
  void brk();
  void add_location(SrcFilePtr file, int line);

  void add_include(const string &s);
  const vector<string> &get_includes() const;

  void add_lib_include(const string &s);
  const vector<string> &get_lib_includes() const;

  unsigned long long calc_crc();
  void dump(string &dest_str);

  void swap(WriterData &other);

  bool compile_with_debug_info() const;
  bool compile_with_crc() const;
};
