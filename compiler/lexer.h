// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <vector>

#include "common/smart_ptrs/singleton.h"
#include "common/wrappers/string_view.h"

#include "compiler/helper.h"
#include "compiler/token.h"
#include "compiler/utils/string-utils.h"

struct LexerData : private vk::not_copyable {
  explicit LexerData(vk::string_view new_code);
  void new_line();
  const char *get_code() const { return code; }
  vk::string_view get_code_view() const { return vk::string_view{code, code_end}; };
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
  vk::string_view strip_whitespaces(char space_char, std::size_t spaces_to_skip, vk::string_view source) noexcept;

  int line_num{0};
  const char *code{nullptr};
  const char *code_end{nullptr};
  int code_len{0};
  std::vector<Token> tokens;
  bool in_gen_str{false};
  const char *str_begin{nullptr};
  const char *str_cur{nullptr};
  bool dont_hack_last_tokens{false};
};

struct TokenLexer : private vk::not_copyable {
  virtual bool parse(LexerData *lexer_data) const = 0;
  virtual ~TokenLexer() = default;
};

//TODO ??
bool parse_with_helper(LexerData *lexer_data, const std::unique_ptr<Helper<TokenLexer>> &h);

struct TokenLexerError final : TokenLexer {
  std::string error_str;

  explicit TokenLexerError(std::string error_str = "unknown_error") :
    error_str(std::move(error_str)) {
  }

  bool parse(LexerData *lexer_data) const final;
};

struct TokenLexerName final : TokenLexer {
  bool parse(LexerData *lexer_data) const final;
};

struct TokenLexerNum final : TokenLexer {
  bool parse(LexerData *lexer_data) const final;
};

struct TokenLexerSimpleString final : TokenLexer {
  bool parse(LexerData *lexer_data) const final;
};

struct TokenLexerAppendChar final : TokenLexer {
  int c, pass;

  TokenLexerAppendChar(int c, int pass) :
    c(c),
    pass(pass) {
  }

  bool parse(LexerData *lexer_data) const final;
};

struct TokenLexerOctChar final : TokenLexer {
  bool parse(LexerData *lexer_data) const final;
};

struct TokenLexerHexChar final : TokenLexer {
  bool parse(LexerData *lexer_data) const final;
};

struct TokenLexerStringSimpleExpr final : TokenLexer {
  std::unique_ptr<Helper<TokenLexer>> h;

  void init();
  bool parse(LexerData *lexer_data) const final;
  bool parse_operand(LexerData *lexer_data, TokenType type) const;
};

struct TokenLexerStringExpr final : TokenLexer {
  std::unique_ptr<Helper<TokenLexer>> h;

  void init();
  bool parse(LexerData *lexer_data) const final;
};

struct TokenLexerHeredocString final : TokenLexer {
  std::unique_ptr<Helper<TokenLexer>> h;

  void add_esc(const std::string &s, char c);
  void init();
  bool parse(LexerData *lexer_data) const final;
};

struct TokenLexerString final : TokenLexer {
  std::unique_ptr<Helper<TokenLexer>> h;

  void add_esc(const std::string &s, char c);
  void init();
  bool parse(LexerData *lexer_data) const final;
};


struct TokenLexerComment final : TokenLexer {
  bool parse(LexerData *lexer_data) const final;
};

struct TokenLexerIfndefComment final : TokenLexer {
  bool parse(LexerData *lexer_data) const final;
};

struct TokenLexerWithHelper : TokenLexer {
  std::unique_ptr<Helper<TokenLexer>> h;

  virtual void init() = 0;
  bool parse(LexerData *lexer_data) const final;
};

struct TokenLexerToken final : TokenLexer {
  TokenType tp;
  int len;

  TokenLexerToken(TokenType tp, int len) :
    tp(tp),
    len(len) {
  }

  bool parse(LexerData *lexer_data) const final;
};

struct TokenLexerCommon final : TokenLexerWithHelper {
  inline void add_rule(const std::unique_ptr<Helper<TokenLexer>> &h, const std::string &str, TokenType tp);

  void init() final;
};

struct TokenLexerSkip final : TokenLexer {
  int n;
  explicit TokenLexerSkip(int n = 1) :
    n(n) {
  }

  bool parse(LexerData *lexer_data) const final;
};

struct TokenLexerPHP final : TokenLexerWithHelper {
  void init() final;
};

struct TokenLexerPHPDoc final : TokenLexerWithHelper {
  inline void add_rule(const std::unique_ptr<Helper<TokenLexer>> &h, const std::string &str, TokenType tp);

  void init() final;
};

struct TokenLexerGlobal final : TokenLexer {
  TokenLexerPHP *php_lexer{&vk::singleton<TokenLexerPHP>::get()};

  bool parse(LexerData *lexer_data) const final;
};

void lexer_init();
void k2_lexer_init();
std::vector<Token> php_text_to_tokens(vk::string_view text);
std::vector<Token> phpdoc_to_tokens(vk::string_view text);
