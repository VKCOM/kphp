// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/lexer.h"

#include <map>
#include <utility>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wregister"
#include "auto/compiler/keywords_set.hpp"
#pragma GCC diagnostic pop

#include "common/algorithms/find.h"
#include "common/smart_ptrs/singleton.h"
#include "common/wrappers/fmt_format.h"

#include "compiler/stage.h"
#include "compiler/threading/thread-id.h"

/***
  LexerData
 ***/
void LexerData::new_line() {
  line_num++;
}

LexerData::LexerData(vk::string_view new_code) :
  code(new_code.data()),
  code_end(code + new_code.size()),
  code_len(new_code.size()) {
  new_line();
  tokens.reserve(static_cast<size_t >(code_len * 0.3));
}

void LexerData::pass(int shift) {
  line_num += std::count_if(code, code + shift, [](char c) { return c == '\n'; });
  pass_raw(shift);
}

void LexerData::pass_raw(int shift) {
  code += shift;
}

template <typename ...Args>
void LexerData::add_token_(int shift, Args&& ...tok) {
  kphp_assert (code + shift <= code_end);
  tokens.emplace_back(std::forward<Args>(tok)...);
  tokens.back().line_num = line_num;
  tokens.back().debug_str = vk::string_view(code, code + shift);
  //fprintf (stderr, "[%d] %.*s : %d\n", tok->type(), tok->debug_str.length(), tok->debug_str.begin(), line_num);
  pass(shift);
  hack_last_tokens();
}

template <typename ...Args>
void LexerData::add_token(int shift, Args&& ...tok) {
  flush_str();
  add_token_(shift, std::forward<Args>(tok)...);
}

void LexerData::start_str() {
  in_gen_str = true;
  str_begin = get_code();
  str_cur = get_code();
}

/**
 * append_char and flush_str are used to modify entities in PHP source code like string literals
 * e.g. we have to tokenize this code: $x = "New\n";
 * but in class-field `std::string SrcFile::text` we have R"($x = \"New\\n\";)"
 * we will replace "\\n" with "\n" and get string_view on this representation from text field
 *
 * It's really strange behaviour to modify source text, it's better to have dedicated tokens for them
 * which contains `std::string` not string_view, we will do it later
 */
void LexerData::append_char(int c) {
  if (!in_gen_str) {
    start_str();
  }
  if (c == -1) {
    int c = *get_code();
    *const_cast<char *>(str_cur++) = (char)c;
    if (c == '\n') {
      new_line();
    }
    pass_raw(1);
  } else {
    *const_cast<char *>(str_cur++) = (char)c;
  }
}

void LexerData::flush_str() {
  if (in_gen_str) {
    add_token_(0, tok_str, str_begin, str_cur);
    while (str_cur != get_code()) {
      *const_cast<char *>(str_cur++) = ' ';
    }
    in_gen_str = false;
  }
}

bool LexerData::are_last_tokens() {
  return true;
}

template<typename ...Args>
bool LexerData::are_last_tokens(TokenType type1, Args ...args) {
  return tokens.size() >= (sizeof...(args) + 1) &&
         tokens[tokens.size() - sizeof...(args) - 1].type() == type1 &&
         are_last_tokens(args...);
}

template<typename ...Args>
bool LexerData::are_last_tokens(any_token_tag, Args ...args) {
  return tokens.size() >= (sizeof...(args) + 1) &&
         are_last_tokens(args...);
}

template<TokenType token, typename ...Args>
bool LexerData::are_last_tokens(except_token_tag<token>, Args ...args) {
  return tokens.size() >= (sizeof...(args) + 1) &&
         tokens[tokens.size() - sizeof...(args) - 1].type() != token &&
         are_last_tokens(args...);
}

vk::string_view LexerData::strip_whitespaces(char space_char, std::size_t spaces_to_skip, vk::string_view source) noexcept {
  stage::set_line(get_line_num());
  kphp_assert(spaces_to_skip);
  std::string val;
  bool new_line = true;
  std::size_t spaces_skipped = 0;
  for (char c : source) {
    if (new_line && vk::any_of_equal(c, ' ', '\t') && spaces_skipped < spaces_to_skip) {
      kphp_error(space_char == c, "Invalid indentation - tabs and spaces cannot be mixed");
      ++spaces_skipped;
      continue;
    } else if (c == '\n') {
      new_line = true;
      spaces_skipped = 0;
    } else {
      kphp_error(spaces_skipped >= spaces_to_skip,
                 fmt_format("Invalid body indentation level (expecting an indentation level of at least {})", spaces_to_skip));
      new_line = false;
    }
    val.append(1, c);
  }
  return string_view_dup(val);
}

void LexerData::hack_last_tokens() {
  if (dont_hack_last_tokens) {
    return;
  }

  auto remove_last_tokens = [this](size_t cnt) {
    tokens.erase(std::prev(tokens.end(), cnt), tokens.end());
  };

  // perform a cast array matching only if we have something
  // that looks like a cast (a token surrounded by parenthesis)
  if (are_last_tokens(tok_oppar, any_token_tag{}, tok_clpar)) {
    static TokenType casts[][2] = {
      {tok_int,    tok_conv_int},
      {tok_float,  tok_conv_float},
      {tok_double,  tok_conv_float},
      {tok_string, tok_conv_string},
      {tok_array,  tok_conv_array},
      {tok_object, tok_conv_object},
      {tok_bool,   tok_conv_bool},
      {tok_boolean, tok_conv_bool},
    };
    for (auto &cast : casts) {
      // check the middle token, the one that goes before ')'
      if (tokens[tokens.size() - 2].type() == cast[0]) {
        remove_last_tokens(3);
        tokens.emplace_back(cast[1]);
        return;
      }
    }
  }

  /**
   * To properly handle these cases:
   *   \VK\Foo::array
   *   \VK\Foo::try
   *   \VK\Foo::$static_field
   * after tok_double_colon we'll get tok_array or tok_try, but we would like to get tok_func_name
   * as these are valid class member names;
   * we check whether a first char of the next token with is_alpha to filter-out things like tok_opbrk etc.
   */
  if (are_last_tokens(tok_static, tok_double_colon, any_token_tag{}) || are_last_tokens(tok_func_name, tok_double_colon, any_token_tag{})) {
    if (!tokens.back().str_val.empty() && is_alpha(tokens.back().str_val[0])) {
      auto val = static_cast<std::string>(tokens[tokens.size() - 3].str_val);
      val += "::";
      val += static_cast<std::string>(tokens[tokens.size() - 1].str_val);
      Token back = tokens.back();
      remove_last_tokens(3);
      tokens.emplace_back(back.type() == tok_var_name ? tok_var_name : tok_func_name, string_view_dup(val));
      tokens.back().line_num = back.line_num;
      return;
    }
  }

  /**
   * For a case when we encounter a keyword after the '->' it should be a tok_func_name,
   * not tok_array, tok_try, etc.
   * For example: $c->array, $c->try
   */
  if (are_last_tokens(tok_arrow, any_token_tag{})) {
    if (!tokens.back().str_val.empty() && is_alpha(tokens.back().str_val[0])) {
      tokens.back().type_ = tok_func_name;
      return;
    }
  }

  // replace elseif with else+if, but not if the previous token can cause elseif
  // to be interpreted as identifier (class const and method)
  if (are_last_tokens(tok_elseif)) {
    if (!are_last_tokens(tok_function, tok_elseif) && !are_last_tokens(tok_const, tok_elseif)) {
      tokens.back() = Token{tok_else};
      tokens.emplace_back(tok_if);
      return;
    }
  }

  if (are_last_tokens(tok_str_begin, tok_str_end)) {
    remove_last_tokens(2);
    tokens.emplace_back(tok_str);
    return;
  }

  if (are_last_tokens(tok_str_begin, tok_str, tok_str_end)) {
    tokens.pop_back();
    tokens.erase(std::prev(tokens.end(), 2));
    return;
  }

  if (are_last_tokens(tok_str, tok_str_skip_indent)) {
    auto spaces = tokens.back().str_val;
    std::size_t spaces_to_skip = spaces.size();
    tokens.pop_back();
    tokens.back().str_val = strip_whitespaces(spaces.front(), spaces_to_skip, tokens.back().str_val);
    return;
  }

  if (are_last_tokens(tok_commentTs)) {
    if (!are_last_tokens(tok_func_name, tok_commentTs)) {
      tokens.pop_back();
    }
  }

  if (are_last_tokens(tok_new, tok_func_name, except_token_tag<tok_oppar>{})) {
    Token t = tokens.back();
    tokens.pop_back();
    tokens.emplace_back(tok_oppar);
    tokens.emplace_back(tok_clpar);
    tokens.push_back(t);
    return;
  }

  if (are_last_tokens(tok_new, tok_static)) {
    tokens.back().type_ = tok_func_name;
  }

  // "err" is a specific vkcom function, something like "new Exception"; it's ugly and should be removed somewhen
  if (are_last_tokens(except_token_tag<tok_function>{}, tok_func_name, tok_oppar, any_token_tag{})) {
    if (tokens[tokens.size() - 3].str_val == "err") {
      Token t = tokens.back();
      tokens.pop_back();
      tokens.emplace_back(tok_file_relative_c);
      tokens.emplace_back(tok_comma);
      tokens.emplace_back(tok_line_c);
      if (t.type() != tok_clpar) {
        tokens.emplace_back(tok_comma);
      }
      tokens.push_back(t);
      return;
    }
  }

  /**
   * A kludge to parse functions with such names correctly in functions.txt;
   * but keep them as a separate token kinds as they need a slightly different parsing
   */
  if (are_last_tokens(tok_function, tok_var_dump) || are_last_tokens(tok_function, tok_dbg_echo) || are_last_tokens(tok_function, tok_print) || are_last_tokens(tok_function, tok_echo)) {
    tokens.back() = {tok_func_name, tokens.back().str_val};
  }

  // recognize leading slash for define/defined;
  // similar logic has already implemented for regular functions
  if (are_last_tokens(tok_func_name)) {
    auto name = tokens.back().str_val;
    if (name == "\\define") {
      tokens.back() = {tok_define, name};
    } else if (name == "\\defined") {
      tokens.back() = {tok_defined, name};
    }
  }
}

void LexerData::set_dont_hack_last_tokens() {
  dont_hack_last_tokens = true;
}

std::vector<Token> &&LexerData::move_tokens() {
  return std::move(tokens);
}

int LexerData::get_line_num() {
  return line_num;
}


bool parse_with_helper(LexerData *lexer_data, const std::unique_ptr<Helper<TokenLexer>> &h) {
  const char *s = lexer_data->get_code();

  TokenLexer *fnd = h->get_help(s);

  if (fnd && fnd->parse(lexer_data)) {
    return true;
  }
  return h->get_default()->parse(lexer_data);
}

bool TokenLexerError::parse(LexerData *lexer_data) const {
  stage::set_line(lexer_data->get_line_num());
  kphp_error (0, error_str.c_str());
  return false;
}

bool TokenLexerName::parse(LexerData *lexer_data) const {
  const char *s = lexer_data->get_code(), *st = s;
  const TokenType type = s[0] == '$' ? tok_var_name : tok_func_name;

  if (type == tok_var_name) {
    ++s;
  }

  const char *t = s;
  bool closing_curly = false; // whether it's a ${varname} and we need to find matching '}'
  if (type == tok_var_name) {
    if (t[0] == '{') {
      s++; // string value should start after the '{'
      t++; // consume '{'
      closing_curly = true;
    }
    if (is_alpha(t[0])) {
      t++;
      while (is_alphanum(t[0])) {
        t++;
      }
    }
  } else {
    if (is_alpha(t[0]) || t[0] == '\\') {
      t++;
      while (is_alphanum(t[0]) || t[0] == '\\') {
        t++;
      }
    }
    if (s != t) {
      bool bad = false;
      if (t[-1] == '\\') {
        bad = true;
      }
      for (const char *cur = s; cur + 1 != t; cur++) {
        if (cur[0] == '\\' && cur[1] == '\\') {
          bad = true;
          break;
        }
      }
      if (bad) {
        return TokenLexerError("Bad function name " + std::string(s, t)).parse(lexer_data);
      }
    }
  }

  if (s == t) {
    return TokenLexerError("Variable name expected").parse(lexer_data);
  }

  vk::string_view name(s, t);

  if (closing_curly) {
    if (t[0] != '}') {
      return TokenLexerError("Expected '}' after " + std::string(s, t)).parse(lexer_data);
    }
    t++; // consume '}'
  }

  if (type == tok_func_name) {
    const KeywordType *tp = KeywordsSet::get_type(name.begin(), name.size());
    if (tp != nullptr) {
      lexer_data->add_token((int)(t - st), tp->type, s, t);
      return true;
    }
  } else if (name == "GLOBALS") {
    return TokenLexerError("$GLOBALS is not supported").parse(lexer_data);
  }

  lexer_data->add_token((int)(t - st), type, name);
  return true;
}

bool TokenLexerNum::parse(LexerData *lexer_data) const {
  const char *s = lexer_data->get_code();

  const char *t = s;

  enum {
    before_dot,
    after_dot,
    after_e,
    after_e_and_sign,
    after_e_and_digit,
    finish,
    hex,
    binary,
  } state = before_dot;

  if (s[0] == '0' && s[1] == 'x') {
    t += 2;
    state = hex;
  } else if (s[0] == '0' && s[1] == 'b') {
    t += 2;
    state = binary;
  }

  bool with_separator = false;
  bool is_float = false;

  while (*t && state != finish) {
    if (*t == '_') {
      t++;
      with_separator = true;
      continue;
    }
    switch (state) {
      case hex: {
        switch (*t) {
          case '0' ... '9':
          case 'A' ... 'F':
          case 'a' ... 'f':
            t++;
            break;
          default:
            state = finish;
            break;
        }
        break;
      }
      case binary: {
        switch(*t) {
          case '0':
          case '1':
            t++;
            break;
          default:
            state = finish;
            break;
        }
        break;
      }
      case before_dot: {
        switch (*t) {
          case '0' ... '9': {
            t++;
            break;
          }
          case '.': {
            t++;
            is_float = true;
            state = after_dot;
            break;
          }
          case 'e':
          case 'E': {
            t++;
            is_float = true;
            state = after_e;
            break;
          }
          default: {
            state = finish;
            break;
          }
        }
        break;
      }
      case after_dot: {
        switch (*t) {
          case '0' ... '9': {
            t++;
            break;
          }
          case 'e':
          case 'E': {
            t++;
            state = after_e;
            break;
          }
          default: {
            state = finish;
            break;
          }
        }
        break;
      }
      case after_e: {
        switch (*t) {
          case '-':
          case '+': {
            t++;
            state = after_e_and_sign;
            break;
          }
          case '0' ... '9': {
            t++;
            state = after_e_and_digit;
            break;
          }
          default: {
            return TokenLexerError("Bad exponent").parse(lexer_data);
          }
        }
        break;
      }
      case after_e_and_sign: {
        switch (*t) {
          case '0' ... '9': {
            t++;
            state = after_e_and_digit;
            break;
          }
          default: {
            return TokenLexerError("Bad exponent").parse(lexer_data);
          }
        }
        break;
      }
      case after_e_and_digit: {
        switch (*t) {
          case '0' ... '9': {
            t++;
            break;
          }
          default: {
            state = finish;
            break;
          }
        }
        break;
      }

      case finish: {
        assert (0);
      }
    }
  }

  if (!is_float) {
    if (s[0] == '0' && s[1] != 'x' && s[1] != 'b') {
      for (int i = 0; i < t - s; i++) {
        if (s[i] < '0' || s[i] > '7') {
          return TokenLexerError("Bad octal number").parse(lexer_data);
        }
      }
    }
  }

  assert (t != s);

  auto token_type = is_float ? tok_float_const : tok_int_const;

  if (with_separator) {
    token_type = is_float ? tok_float_const_sep : tok_int_const_sep;
  }

  lexer_data->add_token(static_cast<int>(t - s), token_type, s, t);

  return true;
}


bool TokenLexerSimpleString::parse(LexerData *lexer_data) const {
  std::string str;

  const char *s = lexer_data->get_code();
  const char *t = s + 1;

  lexer_data->pass_raw(1);
  bool need = true;
  lexer_data->start_str();
  while (need) {
    switch (t[0]) {
      case 0:
        return TokenLexerError("Unexpected end of file").parse(lexer_data);

      case '\'':
        need = false;
        t++;
        continue;

      case '\\':
        if (t[1] == '\\') {
          t += 2;
          lexer_data->append_char('\\');
          lexer_data->pass_raw(2);
          continue;
        } else if (t[1] == '\'') {
          t += 2;
          lexer_data->append_char('\'');
          lexer_data->pass_raw(2);
          continue;
        }
        break;

      default:
        break;
    }
    lexer_data->append_char(-1);
    t++;
  }
  lexer_data->flush_str();
  lexer_data->pass_raw(1);
  return true;
}

bool TokenLexerAppendChar::parse(LexerData *lexer_data) const {
  lexer_data->append_char(c);
  lexer_data->pass_raw(pass);
  return true;
}

bool TokenLexerOctChar::parse(LexerData *lexer_data) const {
  const char *s = lexer_data->get_code(), *t = s;
  int val = t[1] - '0';
  t += 2;

  int add = conv_oct_digit(*t);
  if (add != -1) {
    val = (val << 3) + add;

    add = conv_oct_digit(*++t);
    if (add != -1) {
      val = (val << 3) + add;
      t++;
    }
  }

  //TODO: \777
  lexer_data->append_char(val);
  lexer_data->pass_raw((int)(t - s));
  return true;
}

bool TokenLexerHexChar::parse(LexerData *lexer_data) const {
  const char *s = lexer_data->get_code(), *t = s + 2;
  int val = conv_hex_digit(*t);
  if (val == -1) {
    return TokenLexerError("It is not hex char").parse(lexer_data);
  }

  int add = conv_hex_digit(*++t);
  if (add != -1) {
    val = (val << 4) + add;
    t++;
  }

  lexer_data->append_char(val);
  lexer_data->pass_raw((int)(t - s));
  return true;
}

void TokenLexerStringSimpleExpr::init() {
  kphp_assert(!h);
  h = std::make_unique<Helper<TokenLexer>>(new TokenLexerError("Can't parse string expression"));

  h->add_rule("[a-zA-Z_$\\]", &vk::singleton<TokenLexerName>::get());
  h->add_rule("[0-9]|.[0-9]", &vk::singleton<TokenLexerNum>::get());
  h->add_simple_rule("", &vk::singleton<TokenLexerCommon>::get());
}

bool TokenLexerStringSimpleExpr::parse_operand(LexerData *lexer_data, TokenType type = tok_func_name) const {
  // in places where we expect a "name" we call this method instead of parse_with_helper
  // as we don't want \\ to become a part of the name;
  // in strings, $x->y\n should produce a tok_var_name($x) tok_arrow tok_func_name(y) tok_str(\n),
  // not tok_var_name($x) tok_arrow tok_func_name(y\n) as it would do if we used
  // a TokenLexerName for that.

  const char *begin = lexer_data->get_code();
  const char *s = begin;

  if (!is_alpha(*s)) {
    return parse_with_helper(lexer_data, h);
  }

  ++s;
  while (is_alphanum(*s)) {
    ++s;
  }

  vk::string_view name(begin, s);
  lexer_data->add_token(name.size(), type, name);
  return true;
}

bool TokenLexerStringSimpleExpr::parse(LexerData *lexer_data) const {
  assert(h != nullptr);

  lexer_data->add_token(0, tok_expr_begin);

  if (!parse_operand(lexer_data)) {
    return false;
  }

  const char *s = lexer_data->get_code();
  bool ok = true;
  if (*s == 0) {
    ok = false;
  } else if (*s == '[') {
    // two forms:
    // 1. '[' offset ']'
    // 2. '[' '-' offset ']'
    ok = parse_with_helper(lexer_data, h) && // tok_clbrk
         parse_operand(lexer_data, tok_str); // offset expression (or tok_minus)
    if (ok && lexer_data->are_last_tokens(tok_minus)) {
      ok = parse_operand(lexer_data, tok_str); // offset expression
    }
    ok = ok && parse_with_helper(lexer_data, h); // tok_clbrk
  } else if (s[0] == '-' && s[1] == '>') {
    ok = parse_with_helper(lexer_data, h) && // tok_arrow
         parse_operand(lexer_data);          // accessed instance member
  }

  lexer_data->add_token(0, tok_expr_end);

  return ok;
}

void TokenLexerStringExpr::init() {
  assert(!h);
  h = std::make_unique<Helper<TokenLexer>>(new TokenLexerError("Can't parse"));

  h->add_simple_rule("\'", &vk::singleton<TokenLexerSimpleString>::get());
  h->add_simple_rule("\"", &vk::singleton<TokenLexerString>::get());
  h->add_rule("[a-zA-Z_$\\]", &vk::singleton<TokenLexerName>::get());

  //TODO: double (?)
  h->add_rule("[0-9]|.[0-9]", &vk::singleton<TokenLexerNum>::get());

  h->add_rule(" |\t|\n|\r", &vk::singleton<TokenLexerSkip>::get());
  h->add_simple_rule("", &vk::singleton<TokenLexerCommon>::get());
}

bool TokenLexerStringExpr::parse(LexerData *lexer_data) const {
  assert (h != nullptr);
  const char *s = lexer_data->get_code();
  assert (*s == '{');
  lexer_data->add_token(1, tok_expr_begin);

  int bal = 0;
  while (true) {
    const char *s = lexer_data->get_code();

    if (*s == 0) {
      return TokenLexerError("Unexpected end of file").parse(lexer_data);
    } else if (*s == '{') {
      bal++;
    } else if (*s == '}') {
      if (bal == 0) {
        lexer_data->add_token(1, tok_expr_end);
        break;
      }
      bal--;
    }

    if (!parse_with_helper(lexer_data, h)) {
      return false;
    }
  }
  return true;
}


void TokenLexerString::add_esc(const std::string &s, char c) {
  h->add_simple_rule(s, new TokenLexerAppendChar(c, (int)s.size()));
}

void TokenLexerHeredocString::add_esc(const std::string &s, char c) {
  h->add_simple_rule(s, new TokenLexerAppendChar(c, (int)s.size()));
}

void TokenLexerString::init() {
  assert(!h);
  h = std::make_unique<Helper<TokenLexer>>(new TokenLexerAppendChar(-1, 0));

  add_esc("\\f", '\f');
  add_esc("\\n", '\n');
  add_esc("\\r", '\r');
  add_esc("\\t", '\t');
  add_esc("\\v", '\v');
  add_esc("\\$", '$');
  add_esc("\\\\", '\\');
  add_esc("\\\"", '\"');

  h->add_rule("\\[0-7]", &vk::singleton<TokenLexerOctChar>::get());
  h->add_rule("\\x[0-9A-Fa-f]", &vk::singleton<TokenLexerHexChar>::get());

  h->add_rule("$[A-Za-z_{]", &vk::singleton<TokenLexerStringSimpleExpr>::get());
  h->add_simple_rule("{$", &vk::singleton<TokenLexerStringExpr>::get());
}

void TokenLexerHeredocString::init() {
  assert(!h);
  h = std::make_unique<Helper<TokenLexer>>(new TokenLexerAppendChar(-1, 0));

  add_esc("\\f", '\f');
  add_esc("\\n", '\n');
  add_esc("\\r", '\r');
  add_esc("\\t", '\t');
  add_esc("\\v", '\v');
  add_esc("\\$", '$');
  add_esc("\\\\", '\\');

  h->add_rule("\\[0-7]", &vk::singleton<TokenLexerOctChar>::get());
  h->add_rule("\\x[0-9A-Fa-f]", &vk::singleton<TokenLexerHexChar>::get());

  h->add_rule("$[A-Za-z{]", &vk::singleton<TokenLexerStringSimpleExpr>::get());
  h->add_simple_rule("{$", &vk::singleton<TokenLexerStringExpr>::get());
}

bool TokenLexerString::parse(LexerData *lexer_data) const {
  assert (h != nullptr);
  const char *s = lexer_data->get_code();
  int is_heredoc = s[0] == '<';
  assert (!is_heredoc);

  lexer_data->add_token(1, tok_str_begin);

  while (true) {
    const char *s = lexer_data->get_code();
    if (*s == '\"') {
      lexer_data->add_token(1, tok_str_end);
      break;
    }

    if (*s == 0) {
      return TokenLexerError("Unexpected end of file").parse(lexer_data);
    }

    if (!parse_with_helper(lexer_data, h)) {
      return false;
    }
  }
  return true;
}

bool TokenLexerHeredocString::parse(LexerData *lexer_data) const {
  const char *s = lexer_data->get_code();
  int is_heredoc = s[0] == '<';
  assert (is_heredoc);

  std::string tag;
  const char *st = s;
  assert (s[1] == '<' && s[2] == '<');

  s += 3;

  while (s[0] == ' ') {
    s++;
  }

  bool double_quote = s[0] == '"';
  bool single_quote = s[0] == '\'';

  if (double_quote || single_quote) {
    s++;
  }

  while (is_alpha(s[0])) {
    tag += *s++;
  }

  if (tag.empty()) {
    return TokenLexerError("TAG expected").parse(lexer_data);
  }
  if (double_quote && s[0] != '"') {
    return TokenLexerError("\" expected").parse(lexer_data);
  }
  if (single_quote && s[0] != '\'') {
    return TokenLexerError("' expected").parse(lexer_data);
  }
  if (double_quote || single_quote) {
    s++;
  }
  if (s[0] != '\n') {
    return TokenLexerError("'\\n' expected").parse(lexer_data);
  }
  s++;

  if (!single_quote) {
    lexer_data->add_token((int)(s - st), tok_str_begin);
    assert (s == lexer_data->get_code());
  } else {
    lexer_data->start_str();
    lexer_data->pass_raw((int)(s - st));
    assert (s == lexer_data->get_code());
  }
  bool first = true;
  while (true) {
    const char *s = lexer_data->get_code();
    const char *t = s, *st = s;
    if (t[0] == '\n' || first) {
      t += t[0] == '\n';
      const char *const spaces_start = t;
      std::size_t spaces_count = 0;
      std::size_t tabs_count = 0;
      for (; *t != '\0'; ++t) {
        if (*t == ' ') {
          ++spaces_count;
        } else if (*t == '\t') {
          ++tabs_count;
        } else {
          break;
        }
      }
      if (!strncmp(t, tag.c_str(), tag.size())) {
        t += tag.size();

        stage::set_line(lexer_data->get_line_num());
        kphp_error(!spaces_count || !tabs_count, "Invalid indentation - tabs and spaces cannot be mixed");

        int semicolon = 0;
        if (t[0] == ';') {
          t++;
          semicolon = 1;
        }
        if (t[0] == '\n' || t[0] == 0) {
          if (!single_quote) {
            lexer_data->add_token((int)(t - st - semicolon), tok_str_end);
          } else {
            lexer_data->flush_str();
            lexer_data->pass_raw((int)(t - st - semicolon));;
          }
          if (std::size_t indent = spaces_count ?: tabs_count) {
            lexer_data->add_token(0, tok_str_skip_indent, spaces_start, spaces_start + indent);
          }
          break;
        }
      }
    }

    if (*s == 0) {
      return TokenLexerError("Unexpected end of file").parse(lexer_data);
    }

    if (!single_quote) {
      if (!parse_with_helper(lexer_data, h)) {
        return false;
      }
    } else {
      lexer_data->append_char(-1);
    }
    first = false;
  }
  return true;
}

bool TokenLexerComment::parse(LexerData *lexer_data) const {
  const char *s = lexer_data->get_code(),
    *st = s;

  assert (s[0] == '/' || s[0] == '#');
  if (s[0] == '#' || s[1] == '/') {
    while (s[0] && s[0] != '\n') {
      s++;
    }
  } else {
    s += 2;
    if (s[0] && s[1] && s[0] == '*' && s[1] != '/') { // phpdoc
      char const *phpdoc_start = s;
      bool is_phpdoc = false;
      while (s[0] && (s[0] != '*' || s[1] != '/')) {
        // @return, @var, @param, @type, etc
        if (s[0] == '@') {
          is_phpdoc = true;
        }
        s++;
      }
      if (is_phpdoc) {
        lexer_data->add_token(0, tok_phpdoc, phpdoc_start, s);
      }
    } else if (s[0] && s[1] && s[0] == '<') { // generics commentTs: /*<T>*/
      char const *inst_start = ++s;
      while (s[0] && (s[0] != '*' || s[1] != '/')) {
        s++;
      }
      if (s[-1] != '>') {
        return TokenLexerError("Unclosed generics instantiation: /*< must be ended by >*/").parse(lexer_data);
      }
      lexer_data->add_token(0, tok_commentTs, inst_start, s - 1);
    } else {
      while (s[0] && (s[0] != '*' || s[1] != '/')) {
        s++;
      }
    }
    if (s[0]) {
      s += 2;
    } else {
      return TokenLexerError("Unclosed comment (*/ expected)").parse(lexer_data);
    }
  }

  lexer_data->pass((int)(s - st));
  return true;
}

bool TokenLexerIfndefComment::parse(LexerData *lexer_data) const {
  kphp_assert(lexer_data->get_code_view().starts_with("#ifndef KPHP")
              || lexer_data->get_code_view().starts_with("#ifndef K2"));

  auto endif_pos = lexer_data->get_code_view().find("#endif");
  if (endif_pos == vk::string_view::npos) {
    return TokenLexerError("Unclosed comment (#endif expected)").parse(lexer_data);
  }

  lexer_data->pass(endif_pos + strlen("#endif"));
  return true;
}

bool TokenLexerWithHelper::parse(LexerData *lexer_data) const {
  assert (h != nullptr);
  return parse_with_helper(lexer_data, h);
}

bool TokenLexerToken::parse(LexerData *lexer_data) const {
  lexer_data->add_token(len, tp);
  return true;
}

void TokenLexerCommon::add_rule(const std::unique_ptr<Helper<TokenLexer>> &h, const std::string &str, TokenType tp) {
  h->add_simple_rule(str, new TokenLexerToken(tp, (int)str.size()));
}

void TokenLexerCommon::init() {
  assert(!h);
  h = std::make_unique<Helper<TokenLexer>>(new TokenLexerError("No <common token> found"));

  add_rule(h, ":::", tok_triple_colon);   // used in functions.txt to express cast params

  add_rule(h, "=", tok_eq1);
  add_rule(h, "==", tok_eq2);
  add_rule(h, "===", tok_eq3);
  add_rule(h, "<>", tok_neq_lg);
  add_rule(h, "!=", tok_neq2);
  add_rule(h, "!==", tok_neq3);
  add_rule(h, "<=>", tok_spaceship);
  add_rule(h, "<", tok_lt);
  add_rule(h, "<=", tok_le);
  add_rule(h, ">", tok_gt);
  add_rule(h, ">=", tok_ge);

  add_rule(h, "(", tok_oppar);
  add_rule(h, ")", tok_clpar);
  add_rule(h, "[", tok_opbrk);
  add_rule(h, "]", tok_clbrk);
  add_rule(h, "{", tok_opbrc);
  add_rule(h, "}", tok_clbrc);
  add_rule(h, ":", tok_colon);
  add_rule(h, ";", tok_semicolon);
  add_rule(h, ".", tok_dot);
  add_rule(h, ",", tok_comma);

  add_rule(h, "**", tok_pow);
  add_rule(h, "++", tok_inc);
  add_rule(h, "--", tok_dec);
  add_rule(h, "+", tok_plus);
  add_rule(h, "-", tok_minus);
  add_rule(h, "*", tok_times);
  add_rule(h, "/", tok_divide);

  add_rule(h, "@", tok_at);

  add_rule(h, "%", tok_mod);
  add_rule(h, "&", tok_and);
  add_rule(h, "|", tok_or);
  add_rule(h, "^", tok_xor);
  add_rule(h, "~", tok_not);
  add_rule(h, "!", tok_log_not);
  add_rule(h, "?", tok_question);
  add_rule(h, "??", tok_null_coalesce);

  add_rule(h, "<<", tok_shl);
  add_rule(h, ">>", tok_shr);
  add_rule(h, "+=", tok_set_add);
  add_rule(h, "-=", tok_set_sub);
  add_rule(h, "*=", tok_set_mul);
  add_rule(h, "/=", tok_set_div);
  add_rule(h, "%=", tok_set_mod);
  add_rule(h, "**=", tok_set_pow);
  add_rule(h, "&=", tok_set_and);
  add_rule(h, "&&", tok_log_and);
  add_rule(h, "|=", tok_set_or);
  add_rule(h, "||", tok_log_or);
  add_rule(h, "^=", tok_set_xor);
  add_rule(h, ".=", tok_set_dot);
  add_rule(h, ">>=", tok_set_shr);
  add_rule(h, "<<=", tok_set_shl);
  add_rule(h, "\?\?=", tok_set_null_coalesce); // escaping ? to avoid trigraphs

  add_rule(h, "=>", tok_double_arrow);
  add_rule(h, "::", tok_double_colon);
  add_rule(h, "->", tok_arrow);
  add_rule(h, "...", tok_varg);
}

bool TokenLexerSkip::parse(LexerData *lexer_data) const {
  lexer_data->pass(n);
  return true;
}

void TokenLexerPHP::init() {
  assert(!h);
  h = std::make_unique<Helper<TokenLexer>>(new TokenLexerError("Can't parse"));

  h->add_rule("/*|//|#", &vk::singleton<TokenLexerComment>::get());
  h->add_simple_rule("#ifndef KPHP", &vk::singleton<TokenLexerIfndefComment>::get());
  h->add_simple_rule("\'", &vk::singleton<TokenLexerSimpleString>::get());
  h->add_simple_rule("\"", &vk::singleton<TokenLexerString>::get());
  h->add_simple_rule("<<<", &vk::singleton<TokenLexerHeredocString>::get());
  h->add_rule("[a-zA-Z_$\\]", &vk::singleton<TokenLexerName>::get());

  h->add_rule("[0-9]|.[0-9]", &vk::singleton<TokenLexerNum>::get());

  h->add_rule(" |\t|\n|\r", &vk::singleton<TokenLexerSkip>::get());
  h->add_simple_rule("", &vk::singleton<TokenLexerCommon>::get());
}

void TokenLexerPHPDoc::add_rule(const std::unique_ptr<Helper<TokenLexer>> &h, const std::string &str, TokenType tp) {
  h->add_simple_rule(str, new TokenLexerToken(tp, (int)str.size()));
}

void TokenLexerPHPDoc::init() {
  struct TokenLexerPHPDocStopParsing final : TokenLexer {
    bool parse(LexerData *lexer_data) const final {
      lexer_data->add_token(0, tok_end);
      lexer_data->pass(1);
      return false;
    }
  };

  assert(!h);
  h = std::make_unique<Helper<TokenLexer>>(new TokenLexerPHPDocStopParsing());

  h->add_rule("[a-zA-Z_$\\]", &vk::singleton<TokenLexerName>::get());
  h->add_rule("[0-9]|.[0-9]", &vk::singleton<TokenLexerNum>::get());
  h->add_rule(" |\t|\n|\r", &vk::singleton<TokenLexerSkip>::get());

  add_rule(h, "<", tok_lt);
  add_rule(h, ">", tok_gt);
  add_rule(h, "(", tok_oppar);
  add_rule(h, ")", tok_clpar);
  add_rule(h, "[", tok_opbrk);
  add_rule(h, "]", tok_clbrk);
  add_rule(h, "{", tok_opbrc);
  add_rule(h, "}", tok_clbrc);
  add_rule(h, ":", tok_colon);
  add_rule(h, ";", tok_semicolon);
  add_rule(h, ".", tok_dot);
  add_rule(h, ",", tok_comma);
  add_rule(h, "-", tok_minus);
  add_rule(h, "@", tok_at);
  add_rule(h, "&", tok_and);
  add_rule(h, "*", tok_times);
  add_rule(h, "|", tok_or);
  add_rule(h, "^", tok_xor);
  add_rule(h, "!", tok_log_not);
  add_rule(h, "?", tok_question);
  add_rule(h, "::", tok_double_colon);
  add_rule(h, "=>", tok_double_arrow);
  add_rule(h, "->", tok_arrow);
  add_rule(h, "...", tok_varg);
  add_rule(h, "=", tok_eq1);
}

bool TokenLexerGlobal::parse(LexerData *lexer_data) const {
  const char *s = lexer_data->get_code();
  const char *t = s;
  while (*t && strncmp(t, "<?", 2)) {
    t++;
  }

  if (s != t) {
    lexer_data->add_token((int)(t - s), tok_inline_html, s, t);
    return true;
  }

  if (*s == 0) {
    return TokenLexerError("End of file").parse(lexer_data);
  }

  if (!strncmp(s + 2, "php", 3)) {
    lexer_data->pass_raw(strlen("<?php"));
  } else if (s[2] == '=') {
    // since we don't emit dedicated tokens for different <?php tags, generate a tok_echo for "="
    lexer_data->pass_raw(strlen("<?"));
    lexer_data->add_token(strlen("="), tok_echo);
  } else {
    lexer_data->pass_raw(strlen("<?"));
  }


  while (true) {
    const char *s = lexer_data->get_code();
    const char *t = s;
    while (t[0] == ' ' || t[0] == '\t') {
      t++;
    }
    lexer_data->pass_raw((int)(t - s));
    s = t;
    if (s[0] == 0 || (s[0] == '?' && s[1] == '>')) {
      break;
    }
    if (!php_lexer->parse(lexer_data)) {
      return false;
    }
  }
  lexer_data->add_token(0, tok_semicolon);
  if (*lexer_data->get_code()) {
    lexer_data->pass(2);
  }
  if (*lexer_data->get_code() == '\n') {
    lexer_data->pass(1);
  } else {
    while (*lexer_data->get_code() == ' ') {
      lexer_data->pass(1);
    }
  }

  return true;
}

void lexer_init() {
  vk::singleton<TokenLexerCommon>::get().init();
  vk::singleton<TokenLexerStringSimpleExpr>::get().init();
  vk::singleton<TokenLexerStringExpr>::get().init();
  vk::singleton<TokenLexerString>::get().init();
  vk::singleton<TokenLexerHeredocString>::get().init();
  vk::singleton<TokenLexerPHP>::get().init();
  vk::singleton<TokenLexerPHPDoc>::get().init();
}

void k2_lexer_init() {
  auto & h = vk::singleton<TokenLexerPHP>::get().h;
  h->add_simple_rule("#ifndef K2", &vk::singleton<TokenLexerIfndefComment>::get());
}

std::vector<Token> php_text_to_tokens(vk::string_view text) {
  static TokenLexerGlobal lexer;

  LexerData lexer_data{text};

  while (*lexer_data.get_code()) {
    if (!lexer.parse(&lexer_data)) {
      kphp_error(false, "failed to parse");
      return {};
    }
  }

  auto tokens = lexer_data.move_tokens();
  tokens.emplace_back(tok_end);
  return tokens;
}

std::vector<Token> phpdoc_to_tokens(vk::string_view text) {
  LexerData lexer_data{text};
  lexer_data.set_dont_hack_last_tokens(); // like in op_conv_int, future(int) doesn't need (int)
  while (!lexer_data.get_code_view().empty()) {
    if (!vk::singleton<TokenLexerPHPDoc>::get().parse(&lexer_data)) {
      break;
    }

    // the usual pattern for the phpdoc variable is "some_type|(or | complex) $var any comment ..."
    // i.e. there is no $var inside the phpdoc-type, so when we encounter $var we can stop;
    // "$var some_type|(or | complex) any comment ..." is an exception from this (we tokenize the entire comment in this case)
    if (lexer_data.are_last_tokens(tok_var_name) && lexer_data.get_num_tokens() > 1) {
      break;
    }
  }

  if (!lexer_data.are_last_tokens(tok_end)) {
    lexer_data.add_token(0, tok_end);
  }
  return lexer_data.move_tokens();
}

