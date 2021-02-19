// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <set>
#include <string>
#include <vector>

#include "compiler/data/data_ptr.h"

class File;

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

  File *file;

  std::vector<Line> lines;
  std::string text;
  unsigned long long hash_of_cpp;
  unsigned long long hash_of_comments;

  bool compile_with_crc_flag;


  void write_code(std::string &dest_str, const Line &line);
  void dump(std::string &dest_str, const std::vector<Line>::iterator &begin, const std::vector<Line>::iterator &end, SrcFilePtr file);

public:
  explicit WriterData(File *file, bool compile_with_crc);

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

  void set_calculated_hashes(unsigned long long hash_of_cpp, unsigned long long hash_of_comments);
  unsigned long long get_hash_of_cpp() { return hash_of_cpp; }
  unsigned long long get_hash_of_comments() { return hash_of_comments; }
  void dump(std::string &dest_str);

  File *get_file() const { return file; }
  bool compile_with_crc() const { return compile_with_crc_flag; }
};
