#pragma once

#include <string>

#include "compiler/code-gen/code-generator.h"
#include "compiler/stage.h"

#define NL NewLine()
#define BEGIN OpenBlock()
#define END CloseBlock()

struct OpenFile {
  std::string file_name;
  std::string subdir;
  bool compile_with_debug_info_flag;
  bool compile_with_crc;
  OpenFile(const std::string &file_name, const std::string &subdir = "",
           bool compile_with_debug_info_flag = true, bool compile_with_crc = true) :
    file_name(file_name),
    subdir(subdir),
    compile_with_debug_info_flag(compile_with_debug_info_flag),
    compile_with_crc(compile_with_crc) {
  }

  void compile(CodeGenerator &W) const {
    W.create_writer();
    W.get_writer().begin_write(compile_with_debug_info_flag, compile_with_crc);
    W.get_writer().set_file_name(file_name, subdir);
  }
};

struct CloseFile {
  void compile(CodeGenerator &W) const {
    W.get_writer().end_write();
    W.clear_writer();
  }
};

struct UpdateLocation {
  const Location &location;
  explicit UpdateLocation(const Location &location):
    location(location) {
  }
  void compile(CodeGenerator &W) const {
    if (!W.get_writer().is_comments_locked()) {
      stage::set_location(location);
      W.get_writer().add_location(stage::get_file(), stage::get_line());
    }
  }
};

struct NewLine {
  void compile(CodeGenerator &W) const {
    W.get_writer().new_line();
  }
};

struct Indent {
  int val;
  Indent(int val) : val(val) { }
  inline void compile(CodeGenerator &W) const {
    W.get_writer().indent(val);
  }
};

struct OpenBlock {
  void compile(CodeGenerator &W) const {
    W << "{" << NL << Indent(+2);
  }
};

struct CloseBlock {
  inline void compile(CodeGenerator &W) const {
    W << Indent(-2) << "}";
  }
};

struct LockComments {
  inline void compile(CodeGenerator &W) const {
    W.get_writer().lock_comments();
  }
};

struct UnlockComments {
  inline void compile(CodeGenerator &W) const {
    W.get_writer().unlock_comments();
  }
};
