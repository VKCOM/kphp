#pragma once

#include "compiler/common.h"
#include "compiler/helper.h"
#include "compiler/token.h"
#include "compiler/utils/string-utils.h"

struct LexerData {
  LexerData();
  ~LexerData();
  void set_code(char *code, int code_len);
  void new_line();
  char *get_code();
  void pass(int x);
  void pass_raw(int x);
  void add_token_(Token *tok, int shift);
  void add_token(Token *tok, int shift);
  void start_str();
  void append_char(int c);
  void flush_str();
  void post_process(const string &main_func_name = string());


  void move_tokens(vector<Token *> *dest);
  int get_line_num();

private:
  DISALLOW_COPY_AND_ASSIGN (LexerData);
  int line_num;
  char *code;
  char *code_end;
  char *start;
  int code_len;
  vector<Token *> tokens;
  bool in_gen_str;
  const char *str_begin;
  char *str_cur;
};

struct TokenLexer {
  virtual int parse(LexerData *lexer_data) const = 0;
  TokenLexer();
  virtual ~TokenLexer();

private:
  DISALLOW_COPY_AND_ASSIGN (TokenLexer);
};

//TODO ??
int parse_with_helper(LexerData *lexer_data, Helper<TokenLexer> *h);

struct TokenLexerError : TokenLexer {
  string error_str;

  explicit TokenLexerError(string error_str = "unknown_error");
  ~TokenLexerError();
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerName : TokenLexer {
  static void init_static();
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerNum : TokenLexer {
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerSimpleString : TokenLexer {
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerAppendChar : TokenLexer {
  int c, pass;

  TokenLexerAppendChar(int c, int pass);
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerOctChar : TokenLexer {
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerHexChar : TokenLexer {
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerStringExpr : TokenLexer {
  Helper<TokenLexer> *h;

  Helper<TokenLexer> *gen_helper();
  void init();
  TokenLexerStringExpr();
  ~TokenLexerStringExpr();
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerHeredocString : TokenLexer {
  Helper<TokenLexer> *h;

  void add_esc(string s, char c);
  Helper<TokenLexer> *gen_helper();
  void init();
  TokenLexerHeredocString();
  ~TokenLexerHeredocString();
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerString : TokenLexer {
  Helper<TokenLexer> *h;

  void add_esc(string s, char c);
  Helper<TokenLexer> *gen_helper();
  void init();
  TokenLexerString();
  ~TokenLexerString();
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerTypeHint : TokenLexer {
  int parse(LexerData *lexer_data) const;
};


struct TokenLexerComment : TokenLexer {
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerIfndefComment : TokenLexer {
  int parse(LexerData *lexer_data) const;
};


struct TokenLexerWithHelper : TokenLexer {
  Helper<TokenLexer> *h;
  virtual Helper<TokenLexer> *gen_helper() = 0;

  TokenLexerWithHelper();

  virtual ~TokenLexerWithHelper();

  void init();
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerToken : TokenLexer {
  TokenType tp;
  int len;

  TokenLexerToken(TokenType tp, int len);

  int parse(LexerData *lexer_data) const;
};

struct TokenLexerCommon : TokenLexerWithHelper {
  inline void add_rule(Helper<TokenLexer> *h, string str, TokenType tp);

  Helper<TokenLexer> *gen_helper();
  TokenLexerCommon();
};

struct TokenLexerSkip : TokenLexer {
  int n;
  TokenLexerSkip(int n = 1);

  int parse(LexerData *lexer_data) const;
};

struct TokenLexerPHP : TokenLexerWithHelper {
  Helper<TokenLexer> *gen_helper();
  TokenLexerPHP();
};

struct TokenLexerGlobal : TokenLexer {
  //static TokenLexerPHP php_lexer;
  TokenLexerPHP *php_lexer;

  TokenLexerGlobal();
  int parse(LexerData *lexer_data) const;

  ~TokenLexerGlobal();
};

void lexer_init();
int php_text_to_tokens(char *text, int text_length, const string &main_func_name, vector<Token *> *result);
