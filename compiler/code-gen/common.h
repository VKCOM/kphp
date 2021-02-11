// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

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
    W.open_file_create_writer(compile_with_debug_info_flag, compile_with_crc, file_name, subdir);
  }
};

struct CloseFile {
  void compile(CodeGenerator &W) const {
    W.close_file_clear_writer();
  }
};

struct UpdateLocation {
  const Location &location;
  explicit UpdateLocation(const Location &location):
    location(location) {
  }
  void compile(CodeGenerator &W) const {
    if (!W.is_comments_locked()) {
      stage::set_location(location);
      W.add_location(stage::get_file(), stage::get_line());
    }
  }
};

struct NewLine {
  void compile(CodeGenerator &W) const {
    W.new_line();
  }
};

struct Indent {
  int val;
  Indent(int val) : val(val) { }
  inline void compile(CodeGenerator &W) const {
    W.indent(val);
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

struct SemicolonAndNL {
  inline void compile(CodeGenerator &W) const {
    W << ";" << NL;
  }
};

struct LockComments {
  inline void compile(CodeGenerator &W) const {
    W.lock_comments();
  }
};

struct UnlockComments {
  inline void compile(CodeGenerator &W) const {
    W.unlock_comments();
  }
};

enum class join_mode {
  one_line,
  multiple_lines
};

template<typename T, typename F>
class JoinValuesImpl {
public:
  JoinValuesImpl(const T &values_container, const char *sep, join_mode mode, F &&value_gen) :
    values_container_(values_container),
    sep_(sep),
    mode_(mode),
    value_gen_(std::forward<F>(value_gen)) {
  }

  void compile(CodeGenerator &W) const {
    auto it = std::begin(values_container_);
    const auto last = std::end(values_container_);
    for (; it != last;) {
      value_gen_(W, *it);
      if (++it != last) {
        W << sep_;
        if (mode_ == join_mode::multiple_lines) {
          W << NL;
        }
      }
    }
  }

private:
  const T &values_container_;
  const char *sep_{nullptr};
  const join_mode mode_{join_mode::one_line};
  F value_gen_;
};

template<typename T>
struct ValueSelfGen {
  using element_type = decltype(*std::begin(std::declval<T>()));

  void operator()(CodeGenerator &W, const element_type &obj) const {
    W << obj;
  }
};

// TODO rename JoinValuesImpl -> JoinValues in c++17 and get rid of this function
template<typename T, typename F = ValueSelfGen<T>>
JoinValuesImpl<T, F> JoinValues(const T &values_container, const char *sep,
                              join_mode mode = join_mode::one_line, F &&value_gen = {}) {
  return JoinValuesImpl<T, F>{values_container, sep, mode, std::forward<F>(value_gen)};
}
