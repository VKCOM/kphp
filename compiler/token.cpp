#include "compiler/token.h"

/*** Token ***/
Token::Token(TokenType type_) :
  type_(type_),
  str_val(nullptr, nullptr),
  debug_str(nullptr, nullptr),
  line_num(-1) {
}

Token::Token(TokenType type_, const vk::string_view &s) :
  type_(type_),
  str_val(s),
  debug_str(nullptr, nullptr),
  line_num(-1) {

}

Token::Token(TokenType type_, const char *s, const char *t) :
  type_(type_),
  str_val(s, t),
  debug_str(nullptr, nullptr),
  line_num(-1) {
}

string Token::to_str() {
  return static_cast<string>(debug_str);
}



