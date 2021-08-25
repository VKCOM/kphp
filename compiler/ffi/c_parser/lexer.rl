// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/ffi/c_parser/lexer.h"

#if defined(__GNUC__) && (__GNUC__ >= 7)
#  pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#elif defined(__clang__)
#  pragma clang diagnostic ignored "-Wunused-variable"
#elif defined(__has_warning)
#  if __has_warning("-Wimplicit-fallthrough")
#    pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#  endif
#endif

%%machine lexer;

%%write data nofinal;

int ffi::Lexer::get_next_token_impl(YYParser::semantic_type *sym, YYParser::location_type *loc) {
  int result = token_type::C_TOKEN_END;

  // ragel-related variables that we initialize ourselves
  p_ = input_start_ + offset_;
  const char *pe = input_end_;
  const char *eof = pe;

  // ragel-related variables that are initialized in the generated init() section below
  int cs, act;
  const char *ts, *te;
  %%write init;

  const auto token_text = [&ts, &te]() {
    return string_span{ts, te};
  };

  %%{
    variable p p_;

    whitespace = [ \t\v\n\f];

    int_lit_suffix = [uU][lL]{1,2} | [lL]{1,2}[uU];
    int_hex_lit = '0'[xX][a-fA-F0-9]+;
    int_dec_lit = [1-9][0-9]*;
    int_oct_lit = '0'[0-7]+;
    int_lit = ('0' | int_hex_lit | int_dec_lit | int_oct_lit) int_lit_suffix?;

    float_lit_suffix = [flFL];
    float_digits = [0-9]* '.' [0-9]+ | [0-9]+ '.';
    float_lit_exp = [eE] [+\-]? [0-9]+;
    float_lit = float_digits float_lit_exp? float_lit_suffix? | [0-9]+ float_lit_exp float_lit_suffix?;

    multiline_comment := any* :>> '*/' @{ fgoto main; };

    main := |*
      whitespace => { /* ignore */ };

      '//' [^\n]* '\n';
      '/*' { fgoto multiline_comment; };

      'auto'     { result = token_type::C_TOKEN_AUTO; fbreak; };
      'register' { result = token_type::C_TOKEN_REGISTER; fbreak; };
      'extern'   { result = token_type::C_TOKEN_EXTERN; fbreak; };
      'enum'     { result = token_type::C_TOKEN_ENUM; fbreak; };
      'static'   { result = token_type::C_TOKEN_STATIC; fbreak; };
      'inline'   { result = token_type::C_TOKEN_INLINE; fbreak; };
      'typedef'  { result = token_type::C_TOKEN_TYPEDEF; fbreak; };
      'const'    { result = token_type::C_TOKEN_CONST; fbreak; };
      'volatile' { result = token_type::C_TOKEN_VOLATILE; fbreak; };
      'void'     { result = token_type::C_TOKEN_VOID; fbreak; };
      'float'    { result = token_type::C_TOKEN_FLOAT; fbreak; };
      'double'   { result = token_type::C_TOKEN_DOUBLE; fbreak; };
      'char'     { result = token_type::C_TOKEN_CHAR; fbreak; };
      'bool'     { result = token_type::C_TOKEN_BOOL; fbreak; };
      'int'      { result = token_type::C_TOKEN_INT; fbreak; };
      'int8_t'   { result = token_type::C_TOKEN_INT8; fbreak; };
      'int16_t'  { result = token_type::C_TOKEN_INT16; fbreak; };
      'int32_t'  { result = token_type::C_TOKEN_INT32; fbreak; };
      'int64_t'  { result = token_type::C_TOKEN_INT64; fbreak; };
      'uint8_t'  { result = token_type::C_TOKEN_UINT8; fbreak; };
      'uint16_t' { result = token_type::C_TOKEN_UINT16; fbreak; };
      'uint32_t' { result = token_type::C_TOKEN_UINT32; fbreak; };
      'uint64_t' { result = token_type::C_TOKEN_UINT64; fbreak; };
      'size_t'   { result = token_type::C_TOKEN_SIZE_T; fbreak; };
      'long'     { result = token_type::C_TOKEN_LONG; fbreak; };
      'short'    { result = token_type::C_TOKEN_SHORT; fbreak; };
      'unsigned' { result = token_type::C_TOKEN_UNSIGNED; fbreak; };
      'union'    { result = token_type::C_TOKEN_UNION; fbreak; };
      'signed'   { result = token_type::C_TOKEN_SIGNED; fbreak; };
      'struct'   { result = token_type::C_TOKEN_STRUCT; fbreak; };

      '-' { result = token_type::C_TOKEN_MINUS; fbreak; };
      '+' { result = token_type::C_TOKEN_PLUS; fbreak; };
      ';' { result = token_type::C_TOKEN_SEMICOLON; fbreak; };
      '{' { result = token_type::C_TOKEN_LBRACE; fbreak; };
      '}' { result = token_type::C_TOKEN_RBRACE; fbreak; };
      '(' { result = token_type::C_TOKEN_LPAREN; fbreak; };
      ')' { result = token_type::C_TOKEN_RPAREN; fbreak; };
      '[' { result = token_type::C_TOKEN_LBRACKET; fbreak; };
      ']' { result = token_type::C_TOKEN_RBRACKET; fbreak; };
      ',' { result = token_type::C_TOKEN_COMMA; fbreak; };
      '*' { result = token_type::C_TOKEN_MUL; fbreak; };
      '=' { result = token_type::C_TOKEN_EQUAL; fbreak; };

      '\.\.\.' { result = token_type::C_TOKEN_ELLIPSIS; fbreak; };

      int_lit { result = int_constant(sym, token_text()); fbreak; };
      float_lit { result = float_constant(sym, token_text()); fbreak; };

      [a-zA-Z_][a-zA-Z0-9_]* { result = typename_or_identifier(sym, token_text()); fbreak; };
    *|;
  }%%

  %%write exec;

  set_location(result, ts, te, loc);

  return result;
}
