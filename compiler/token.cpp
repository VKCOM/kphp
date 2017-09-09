#include "compiler/token.h"

/*** Token ***/
Token::Token (TokenType type_)
  : type_ (type_), line_num(-1) {
}

Token::Token (TokenType type_, const string_ref &s)
  : type_ (type_), str_val (s), line_num(-1) {

}

Token::Token (TokenType type_, const char *s, const char *t)
  : type_ (type_), str_val (s, t), line_num(-1) {
}

string Token::to_str() {
  return debug_str;
}



