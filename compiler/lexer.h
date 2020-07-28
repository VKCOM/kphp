#pragma once

#include "common/smart_ptrs/singleton.h"

#include "compiler/common.h"
#include "compiler/helper.h"
#include "compiler/token.h"
#include "compiler/utils/string-utils.h"

struct LexerData : private vk::not_copyable {
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
  template<TokenType token>
  struct except_token_tag{};
  template<typename ...Args>
  bool are_last_tokens(TokenType type1, Args ...args);
  template<typename ...Args>
  bool are_last_tokens(any_token_tag type1, Args ...args);
  template<TokenType token, typename ...Args>
  bool are_last_tokens(except_token_tag<token> type1, Args ...args);
  bool are_last_tokens();
  int get_num_tokens() const { return tokens.size(); }

  std::vector<Token>&& move_tokens();
  int get_line_num();

private:
  int line_num{-1};
  char *code{nullptr};
  char *code_end{nullptr};
  char *start{nullptr};
  int code_len{0};
  vector<Token> tokens;
  bool in_gen_str{false};
  const char *str_begin{nullptr};
  char *str_cur{nullptr};
  bool dont_hack_last_tokens{false};
};

struct TokenLexer : private vk::not_copyable {
  virtual int parse(LexerData *lexer_data) const = 0;
  virtual ~TokenLexer() = default;
};

//TODO ??
int parse_with_helper(LexerData *lexer_data, const std::unique_ptr<Helper<TokenLexer>> &h);

struct TokenLexerError final : TokenLexer {
  string error_str;

  explicit TokenLexerError(string error_str = "unknown_error") :
    error_str(std::move(error_str)) {
  }

  int parse(LexerData *lexer_data) const;
};

struct TokenLexerName final : TokenLexer {
  static void init_static();
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerNum final : TokenLexer {
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerSimpleString final : TokenLexer {
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerAppendChar final : TokenLexer {
  int c, pass;

  TokenLexerAppendChar(int c, int pass);
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerOctChar final : TokenLexer {
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerHexChar final : TokenLexer {
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerStringExpr final : TokenLexer {
  std::unique_ptr<Helper<TokenLexer>> h;

  void init();
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerHeredocString final : TokenLexer {
  std::unique_ptr<Helper<TokenLexer>> h;

  void add_esc(const string &s, char c);
  void init();
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerString final : TokenLexer {
  std::unique_ptr<Helper<TokenLexer>> h;

  void add_esc(const string &s, char c);
  void init();
  int parse(LexerData *lexer_data) const;
};


struct TokenLexerComment final : TokenLexer {
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerIfndefComment final : TokenLexer {
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerWithHelper : TokenLexer {
  std::unique_ptr<Helper<TokenLexer>> h;

  virtual void init() = 0;
  int parse(LexerData *lexer_data) const;
};

struct TokenLexerToken final : TokenLexer {
  TokenType tp;
  int len;

  TokenLexerToken(TokenType tp, int len) :
    tp(tp),
    len(len) {
  }

  int parse(LexerData *lexer_data) const;
};

struct TokenLexerCommon final : TokenLexerWithHelper {
  inline void add_rule(const std::unique_ptr<Helper<TokenLexer>> &h, const string &str, TokenType tp);

  void init() final;
};

struct TokenLexerSkip final : TokenLexer {
  int n;
  TokenLexerSkip(int n = 1) :
    n(n) {
  }

  int parse(LexerData *lexer_data) const;
};

struct TokenLexerPHP final : TokenLexerWithHelper {
  void init() final;
};

struct TokenLexerPHPDoc final : TokenLexerWithHelper {
  inline void add_rule(const std::unique_ptr<Helper<TokenLexer>> &h, const string &str, TokenType tp);

  void init() final;
};

struct TokenLexerGlobal final : TokenLexer {
  TokenLexerPHP *php_lexer{vk::singleton<TokenLexerPHP>::get()};

  int parse(LexerData *lexer_data) const;
};

void lexer_init();
vector<Token> php_text_to_tokens(char *text, int text_length);
vector<Token> phpdoc_to_tokens(const char *text, int text_length);
