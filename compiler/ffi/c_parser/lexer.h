// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

// set yylex() to get_next_token()
#undef YY_DECL
#define YY_DECL ffi::YYParser::symbol_type ffi::Lexer::get_next_token()

#include "common/algorithms/contains.h"
#include "compiler/ffi/c_parser/location.h"
#include "compiler/ffi/c_parser/string-span.h"
#include "compiler/ffi/c_parser/yy_parser_generated.hpp"

#include <unordered_map>

namespace ffi {

class Lexer {
public:
  explicit Lexer(const FFITypedefs& typedefs, const std::string& input)
      : p_{input.c_str()},
        input_start_{input.c_str()},
        input_end_{input.c_str() + input.length()},
        typedefs_{typedefs} {
    reset_comment();
  }

  int get_next_token(YYParser::semantic_type* sym, YYParser::location_type* loc) {
    auto tok = get_next_token_impl(sym, loc);
    offset_ = p_ - input_start_;
    return tok;
  }

  string_span get_comment() const noexcept {
    return comment_;
  }

  void reset_comment() noexcept {
    comment_.data_ = nullptr;
    comment_.len_ = 0;
  }

private:
  const char* p_;
  const char* input_start_;
  const char* input_end_;
  int64_t offset_ = 0;
  const FFITypedefs& typedefs_;

  using token_type = YYParser::token_type;

  string_span comment_;

  // get_next_token_impl() is implemented in a generated file after invoking Ragel
  // see lexer.rl file
  //
  // How to re-generate:
  // 1. install Ragel (github.com/adrian-thurston/ragel)
  // 2. run `$ ragel -C -G2 lexer.rl -o lexer_generated.cpp`
  //
  // Ragel args explanation:
  // -C is for C/C++ output format;
  // -G2 generates a goto-based FSM, 2 is like optimization level (G0 code is more compact, but slower)
  int get_next_token_impl(YYParser::semantic_type* sym, YYParser::location_type* loc);

  void set_location(int tok, const char* token_begin, const char* token_end, YYParser::location_type* loc) {
    if (tok == token_type::C_TOKEN_END) {
      loc->begin = input_end_ - input_start_;
      loc->end = loc->begin;
    } else {
      loc->begin = token_begin - input_start_;
      loc->end = token_end - input_start_;
    }
  }

  int int_constant(YYParser::semantic_type* sym, string_span text) {
    sym->str_ = text;
    return token_type::C_TOKEN_INT_CONSTANT;
  }

  int float_constant(YYParser::semantic_type* sym, string_span text) {
    sym->str_ = text;
    return token_type::C_TOKEN_FLOAT_CONSTANT;
  }

  int typename_or_identifier(YYParser::semantic_type* sym, string_span text) {
    sym->str_ = text;
    if (vk::contains(typedefs_, text.to_string())) {
      return token_type::C_TOKEN_TYPEDEF_NAME;
    }
    return token_type::C_TOKEN_IDENTIFIER;
  }
};

} // namespace ffi
