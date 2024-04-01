// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/debug.h"

class Location {
  DEBUG_STRING_METHOD {
    return as_human_readable();
  }

public:
  SrcFilePtr file;
  FunctionPtr function;
  int line = -1;

  Location() = default;
  explicit Location(int line)
    : line(line) {}
  Location(const SrcFilePtr &file, const FunctionPtr &function, int line);

  void set_file(SrcFilePtr file);
  void set_function(FunctionPtr function);
  void set_line(int line);
  SrcFilePtr get_file() const {
    return file;
  }
  FunctionPtr get_function() const {
    return function;
  }
  int get_line() const {
    return line;
  }

  std::string as_human_readable() const;

  friend bool operator<(const Location &lhs, const Location &rhs);
};
