// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <set>
#include <string>
#include <vector>

#include "compiler/data/data_ptr.h"

class WriterData {
private:
  struct Line {
    int begin_pos = 0;
    int end_pos = -1;
    SrcFilePtr file;
    bool brk = false;
    std::set<int> line_ids;
    Line() = default;
    explicit Line(int begin_pos) : begin_pos(begin_pos){}
  };

  std::vector<Line> lines;
  std::string text;
  unsigned long long crc;

  std::vector<std::string> includes;
  std::vector<std::string> lib_includes;
  bool compile_with_debug_info_flag;
  bool compile_with_crc_flag;

public:
  std::string file_name;
  std::string subdir;

private:
  void write_code(std::string &dest_str, const Line &line);
  void dump(std::string &dest_str, const std::vector<Line>::iterator &begin, const std::vector<Line>::iterator &end, SrcFilePtr file);

public:
  explicit WriterData(bool compile_with_debug_info_flag, bool compile_with_crc, std::string file_name, std::string subdir);

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

  void add_include(const std::string &s);
  std::vector<std::string> flush_includes();

  void add_lib_include(const std::string &s);
  std::vector<std::string> flush_lib_includes();

  unsigned long long calc_crc();
  void dump(std::string &dest_str);

  bool compile_with_debug_info() const;
  bool compile_with_crc() const;
};
