#pragma once

#include "compiler/common.h"
#include "compiler/helper.h"
#include "compiler/token.h"
#include "compiler/utils/string-utils.h"

struct LexerData : private vk::not_copyable {
  LexerData();
  ~LexerData();
  void set_code(char *code, int code_len);
  void new_line();
  char *get_code();
  void pass(int shift);
  void pass_raw(int shift);
  template <typename ...Args>
  void add_token_(int shift, Args&& ...tok);
  template <typename ...Args>
  void add_token(int shift, Args&& ...tok);
  void start_str();
  void append_char(int c);
  void flush_str();
  void hack_last_tokens();
  void set_dont_hack_last_tokens();

  struct any_token_tag{};
  template<typename ...Args>
  bool are_last_tokens(TokenType type1, Args ...args);
  template<typename ...Args>
  bool are_last_tokens(any_token_tag type1, Args ...args);
  int get_num_tokens() const { return tokens.size(); }

  std::vector<Token>&& move_tokens();
  int get_line_num();

private:
  int line_num;
  char *code;
  char *code_end;
  char *start;
  int code_len;
  vector<Token> tokens;
  bool in_gen_str;
  const char *str_begin;
  char *str_cur;
  bool dont_hack_last_tokens;
};

struct TokenLexer : private vk::not_copyable {
  virtual int parse(LexerData *lexer_data) const = 0;
  TokenLexer();
  virtual ~TokenLexer();
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

  void add_esc(const string &s, char c);
  Helper<TokenLexer> *gen_helper();
  void init();
  TokenLexerHeredocString();
  ~TokenLexerHeredocString();
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerString : TokenLexer {
  Helper<TokenLexer> *h;

  void add_esc(const string &s, char c);
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
  inline void add_rule(Helper<TokenLexer> *h, const string &str, TokenType tp);

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

struct TokenLexerPHPDoc : TokenLexerWithHelper {
  inline void add_rule(Helper<TokenLexer> *h, const string &str, TokenType tp);

  Helper<TokenLexer> *gen_helper();
  TokenLexerPHPDoc();
};

struct TokenLexerGlobal : TokenLexer {
  //static TokenLexerPHP php_lexer;
  TokenLexerPHP *php_lexer;

  TokenLexerGlobal();
  int parse(LexerData *lexer_data) const;

  ~TokenLexerGlobal();
};

void lexer_init();
vector<Token> php_text_to_tokens(char *text, int text_length);
vector<Token> phpdoc_to_tokens(const char *text, int text_length);
