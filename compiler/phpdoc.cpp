#include "compiler/phpdoc.h"

#include <cstdio>
#include <utility>

#include "compiler/compiler-core.h"
#include "compiler/data/function-data.h"
#include "compiler/gentree.h"
#include "compiler/lexer.h"
#include "compiler/name-gen.h"
#include "compiler/stage.h"
#include "compiler/utils/string-utils.h"

using std::vector;
using std::string;

const std::map<string, php_doc_tag::doc_type> php_doc_tag::str2doc_type = {
  {"@param",                 param},
  {"@kphp-inline",           kphp_inline},
  {"@kphp-infer",            kphp_infer},
  {"@kphp-required",         kphp_required},
  {"@kphp-lib-export",       kphp_lib_export},
  {"@kphp-sync",             kphp_sync},
  {"@type",                  var},
  {"@var",                   var},
  {"@return",                returns},
  {"@returns",               returns},
  {"@kphp-disable-warnings", kphp_disable_warnings},
  {"@kphp-extern-func-info", kphp_extern_func_info},
  {"@kphp-pure-function",    kphp_pure_function},
  {"@kphp-template",         kphp_template},
  {"@kphp-return",           kphp_return},
  {"@kphp-memcache-class",   kphp_memcache_class},
  {"@kphp-immutable-class",  kphp_immutable_class},
  {"@kphp-tl-class",         kphp_tl_class},
  {"@kphp-const",            kphp_const},
};

/*
 * Имея '@param $a A[] some description' — где this->value = '$a A[] some description' —
 * уметь вычленить первый токен '$a', потом по offset'у следующий 'A[]' и т.п. — до ближайшего пробела
 * Также понимает конструкции вида @param A ...$a для variadic аргументов
 */
std::string php_doc_tag::get_value_token(size_t chars_offset) const {
  size_t pos = 0;
  size_t len = value.size();
  while (pos < len && value[pos] == ' ') {     // типа left trim: с самого начала пробелы не учитываем
    ++pos;
    ++chars_offset;
  }

  while (chars_offset < len && value[chars_offset] == ' ') {
    ++chars_offset;
  }
  if (chars_offset >= len) {
    return "";
  }

  pos = value.find(' ', chars_offset);
  if (pos == std::string::npos) {
    return value.substr(chars_offset);
  }

  const vk::string_view varg_dots${" ...$"};
  if (value.size() > pos + varg_dots$.size() && value.compare(pos, varg_dots$.size(), varg_dots$.data()) == 0) {
    // оставляем '$' для следующего токена
    pos += varg_dots$.size() - 1;
  }

  return value.substr(chars_offset, pos - chars_offset);
}

vector<php_doc_tag> parse_php_doc(const vk::string_view &phpdoc) {
  if (phpdoc.empty()) {
    return {};
  }

  int line_num_of_function_declaration = stage::get_line();

  vector<string> lines(1);
  bool have_star = false;
  for (char c : phpdoc) {
    if (!have_star) {
      if (c == ' ' || c == '\t') {
        continue;
      }
      if (c == '*') {
        have_star = true;
        continue;
      }
      kphp_error(0, "failed to parse php_doc");
      return vector<php_doc_tag>();
    }
    if (c == '\n') {
      lines.push_back("");
      have_star = false;
      continue;
    }
    if (lines.back().empty() && (c == ' ' || c == '\t')) {
      continue;
    }
    lines.back() += c;
  }
  vector<php_doc_tag> result;
  for (int i = 0; i < lines.size(); i++) {
    if (lines[i][0] == '@') {
      result.emplace_back(php_doc_tag());
      size_t pos = lines[i].find(' ');
      result.back().name = lines[i].substr(0, pos);
      result.back().type = php_doc_tag::get_doc_type(result.back().name);
      if (pos != string::npos) {
        int ltrim_pos = pos + 1;
        while (lines[i][ltrim_pos] == ' ') {
          ltrim_pos++;
        }
        result.back().value = lines[i].substr(ltrim_pos);
      }
    }

    if (line_num_of_function_declaration > 0 && !result.empty()) {
      int new_line_num = line_num_of_function_declaration - (static_cast<int>(lines.size()) - i);
      // We have one line with closing php-doc before function declaration
      // e.g. ....
      //      * @param int $a
      //      */
      //      function f() {}
      result.back().line_num = std::min(new_line_num, line_num_of_function_declaration - 2);
    }
  }
//  for (int i = 0; i < result.size(); i++) {
//    fprintf(stderr, "|%s| : |%s|\n", result[i].name.c_str(), result[i].value.c_str());
//  }
  return result;
}

#define CHECK(x, y) if (kphp_error(x, y)) {return VertexPtr();}

/*
 * Часто в phpdoc после @param/@var или т.п. идёт '$var_name type [comment]' или просто 'type [comment]'
 * и нужно распарсить phpdoc и найти этот $var_name и type возле нужного @doc_type.
 * Примеры: '@param $a A[]', '@param A[] $a', '@var string|false', '@return \Exception', '@param (string|int)[] $arr comment'
 * Внутри полного phpdoc-а встречаются разные теги, причём тех же @param'ов может быть много.
 * Эта функция имеет на вход полный phpdoc как строку и находит нужный тег по типу @return/@param и парсит, что находится возле него.
 * Считается, что даже сложный тип '(string|(int|false))[]' написан без пробелов; он возвращается как строка.
 * Возвращает bool — нашёлся ли; если да, то out_type_str заполнен, а out_var_name либо заполен, либо пустая строка.
 */
bool PhpDocTypeRuleParser::find_tag_in_phpdoc(const vk::string_view &phpdoc, php_doc_tag::doc_type doc_type, string &out_var_name, string &out_type_str, int offset) {
  std::vector<php_doc_tag> tags = parse_php_doc(phpdoc);
  int idx = 0;
  for (const auto &tag : tags) {
    if (tag.type == doc_type && idx++ >= offset) {  // для @param имеет смысл offset
      std::string a1 = tag.get_value_token();
      std::string a2 = tag.get_value_token(a1.size());

      if (!a1.empty() && a1[0] == '$') {
        out_var_name = a1.substr(1);
        out_type_str = std::move(a2);
      } else if (!a2.empty() && a2[0] == '$') {
        out_var_name = a2.substr(1);
        out_type_str = std::move(a1);
      } else {
        out_type_str = std::move(a1);
        out_var_name.clear();
      }
      return true;
    }
  }
  return false;
}

bool PhpDocTypeRuleParser::is_tag_in_phpdoc(const vk::string_view &phpdoc, php_doc_tag::doc_type doc_type) {
  auto tags = parse_php_doc(phpdoc);
  return std::any_of(tags.begin(), tags.end(),
                     [doc_type](const php_doc_tag &tag) {
                       return tag.type == doc_type;
                     });
}

/*
 * Имея строку '(\VK\A|false)[]' и pos=1 — найти, где заканчивается имя класса. ('\VK\A' оно в данном случае)
 */
vk::string_view PhpDocTypeRuleParser::extract_classname_from_pos(const vk::string_view &str, size_t pos) {
  size_t pos_end = pos;
  while (pos_end < str.size() && (isalnum(str[pos_end]) || str[pos_end] == '\\' || str[pos_end] == '_')) {
    ++pos_end;
  }

  return str.substr(pos, pos_end - pos);
}

VertexPtr PhpDocTypeRuleParser::parse_classname(const vk::string_view &s, size_t &pos) {
  bool has_tl_namespace_prefix = false;
  if (s[pos] == '@') {
    vk::string_view tl_namespace_prefix{"@tl\\"};
    if (!s.substr(pos).starts_with(tl_namespace_prefix)) {
      return {};
    }
    pos += tl_namespace_prefix.size();
    has_tl_namespace_prefix = true;
  }

  if (s[pos] == '\\' || is_alpha(s[pos])) {
    std::string relative_class_name = static_cast<std::string>(extract_classname_from_pos(s, pos));
    pos += relative_class_name.size();
    if (has_tl_namespace_prefix) {
      relative_class_name.insert(0, G->env().get_tl_namespace_prefix());
    }
    const std::string &class_name = resolve_uses(current_function, relative_class_name, '\\');
    ClassPtr klass = G->get_class(class_name);
    if (!klass) {
      unknown_classes_list.push_back(class_name);
    }
    kphp_error(!(klass && klass->is_trait()), format("You may not use trait(%s) as a type-hint", klass->get_name()));
    return GenTree::create_type_help_class_vertex(klass);
  }

  return {};
}

VertexPtr PhpDocTypeRuleParser::parse_simple_type(const vk::string_view &s, size_t &pos) {
  CHECK(pos < s.size(), "Failed to parse phpdoc type: unexpected end");
  switch (s[pos]) {
    case '(': {
      pos++;
      VertexPtr v = parse_type_expression(s, pos);
      if (!v) {
        return v;
      }
      CHECK(pos < s.size() && s[pos] == ')', "Failed to parse phpdoc type: unmatching ()");
      pos++;
      return v;
    }
    case 's': {
      if (s.substr(pos, 6) == "string") {
        pos += 6;
        return GenTree::create_type_help_vertex(tp_string);
      }
      if (s.substr(pos, 4) == "self") {
        pos += 4;
        return GenTree::create_type_help_class_vertex(current_function->class_id);
      }
      break;
    }
    case 'i': {
      if (s.substr(pos, 7) == "integer") {
        pos += 7;
        return GenTree::create_type_help_vertex(tp_int);
      }
      if (s.substr(pos, 3) == "int") {
        pos += 3;
        return GenTree::create_type_help_vertex(tp_int);
      }
      break;
    }
    case 'b': {
      if (s.substr(pos, 7) == "boolean") {
        pos += 7;
        return GenTree::create_type_help_vertex(tp_bool);
      }
      if (s.substr(pos, 4) == "bool") {
        pos += 4;
        return GenTree::create_type_help_vertex(tp_bool);
      }
      break;
    }
    case 'f': {
      if (s.substr(pos, 5) == "float") {
        pos += 5;
        return GenTree::create_type_help_vertex(tp_float);
      }
      if (s.substr(pos, 5) == "false") {
        pos += 5;
        return GenTree::create_type_help_vertex(tp_False);
      }
      if (s.substr(pos, 6) == "future") {
        pos += 6;
        return parse_nested_type_rule(s, pos, tp_future);
      }
      break;
    }
    case 'd': {
      if (s.substr(pos, 6) == "double") {
        pos += 6;
        return GenTree::create_type_help_vertex(tp_float);
      }
      break;
    }
    case 'm': {
      if (s.substr(pos, 5) == "mixed") {
        pos += 5;
        return GenTree::create_type_help_vertex(tp_var);
      }
      break;
    }
    case 'n': {
      if (s.substr(pos, 4) == "null") {
        pos += 4;
        return GenTree::create_type_help_vertex(tp_var);
      }
      break;
    }
    case 't': {
      if (s.substr(pos, 4) == "true") {
        pos += 4;
        return GenTree::create_type_help_vertex(tp_bool);
      }
      if (s.substr(pos, 5) == "tuple") {
        pos += 5;
        return parse_nested_type_rule(s, pos, tp_tuple);
      }
      break;
    }
    case 'a': {
      if (s.substr(pos, 5) == "array") {
        pos += 5;
        return GenTree::create_type_help_vertex(tp_array, {GenTree::create_type_help_vertex(tp_Unknown)});
      }
      break;
    }
    case 'v': {
      if (s.substr(pos, 4) == "void") {
        pos += 4;
        return GenTree::create_type_help_vertex(tp_void);
      }
      break;
    }
    case '\\': {
      if (s.substr(pos, 6) == "\\tuple") {
        pos += 6;
        return parse_nested_type_rule(s, pos, tp_tuple);
      }
      if (s.substr(pos, 7) == "\\future") {
        pos += 7;
        return parse_nested_type_rule(s, pos, tp_future);
      }
      break;
    }
  }

  VertexPtr class_type_rule = parse_classname(s, pos);
  if (class_type_rule) {
    return class_type_rule;
  }

  string error_msg = "Failed to parse phpdoc type: Unknown type name [" + static_cast<string>(s) + "]";
  CHECK(0, error_msg.c_str());
  return VertexPtr();
}

VertexPtr PhpDocTypeRuleParser::parse_type_array(const vk::string_view &s, size_t &pos) {
  VertexPtr res = parse_simple_type(s, pos);
  if (!res) {
    return res;
  }
  while (pos < s.size() && s[pos] == '[') {
    CHECK(pos + 1 < s.size() && s[pos + 1] == ']', "Failed to parse phpdoc type: unmatching []");
    res = GenTree::create_type_help_vertex(tp_array, {res});
    pos += 2;
  }
  return res;
}

VertexPtr PhpDocTypeRuleParser::parse_nested_type_rule(const vk::string_view &s, size_t &pos, PrimitiveType type_help) {
  CHECK(pos < s.size() && (s[pos] == '<' || s[pos] == '('),
    "Failed to parse phpdoc type: expected '<' or '('");
  ++pos;
  std::vector<VertexPtr> sub_types;
  while (true) {
    VertexPtr v = parse_type_expression(s, pos);
    if (!v) {
      return v;
    }
    sub_types.emplace_back(v);
    CHECK(pos < s.size(), "Failed to parse phpdoc type: unexpected end");
    if (s[pos] == '>' || s[pos] == ')') {
      ++pos;
      break;
    }
    CHECK(s[pos] == ',', "Failed to parse phpdoc type: expected ','");
    ++pos;
  }
  return GenTree::create_type_help_vertex(type_help, sub_types);
}

VertexPtr PhpDocTypeRuleParser::parse_type_expression(const vk::string_view &s, size_t &pos) {
  size_t old_pos = pos;
  VertexPtr res = parse_type_array(s, pos);
  if (!res) {
    return res;
  }
  bool has_raw_bool = s.substr(old_pos, pos - old_pos) == "bool";
  while (pos < s.size() && s[pos] == '|') {
    pos++;
    old_pos = pos;
    VertexPtr next = parse_type_array(s, pos);
    if (!next) {
      return next;
    }
    has_raw_bool |= s.substr(old_pos, pos - old_pos) == "bool";
    auto rule = VertexAdaptor<op_type_expr_lca>::create(res, next);
    res = rule;
  }
  if (res->type() == op_type_expr_lca) {
    if (has_raw_bool) {
      stage::set_location(current_function->root->location);
      kphp_error(0, format("Do not use |bool in phpdoc, use |false instead\n(if you really need bool, specify |boolean)"));
    }
  }
  return res;
}

VertexPtr PhpDocTypeRuleParser::parse_from_type_string(const vk::string_view &type_str) {
//  fprintf(stderr, "Parsing s = |%s|\n", s.c_str());
  size_t pos = 0;
  VertexPtr res = parse_type_expression(type_str, pos);
  if (!res) {
    return res;
  }

  const vk::string_view varg_dots{" ..."};
  if (type_str.size() == pos + varg_dots.size() && type_str.ends_with(varg_dots)) {
    pos += varg_dots.size();
    res = GenTree::create_type_help_vertex(tp_array, {res});
  }

  CHECK(pos == type_str.size(), "Failed to parse phpdoc type: something left at the end after parsing");
  return res;
}

VertexPtr PhpDocTypeRuleParserUsingLexer::parse_classname(const std::string &phpdoc_class_name) {
  const std::string &class_name = resolve_uses(current_function, phpdoc_class_name, '\\');
  ClassPtr klass = G->get_class(class_name);
  if (!klass) {
    unknown_classes_list.push_back(class_name);
  }
  if (klass && klass->is_trait()) {
    throw std::runtime_error("You may not use trait as a type-hint");
  }

  cur_tok++;
  return GenTree::create_type_help_class_vertex(klass);
}

VertexPtr PhpDocTypeRuleParserUsingLexer::parse_simple_type() {
  TokenType cur_type = cur_tok->type();
  // некоторые слова не являются ключевыми словами (токенами), но трактуются именно так в phpdoc
  if (cur_type == tok_func_name) {
    if (cur_tok->str_val == "integer") {
      cur_type = tok_int;
    } else if (cur_tok->str_val == "double") {
      cur_type = tok_float;
    } else if (cur_tok->str_val == "boolean") {
      cur_type = tok_bool;
    } else if (cur_tok->str_val == "\\tuple") {
      cur_type = tok_tuple;
    }
  }

  switch (cur_type) {
    case tok_end:
      throw std::runtime_error("unexpected end");
    case tok_oppar: {
      cur_tok++;
      VertexPtr v = parse_type_expression();
      if (cur_tok->type() != tok_clpar) {
        throw std::runtime_error("unmatching ()");
      }
      cur_tok++;
      return v;
    }
    case tok_int:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_int);
    case tok_bool:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_bool);
    case tok_float:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_float);
    case tok_string:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_string);
    case tok_false:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_False);
    case tok_true:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_bool);
    case tok_null:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_var);
    case tok_mixed:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_var);
    case tok_var:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_var);
    case tok_void:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_void);
    case tok_tuple:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_tuple, parse_nested_type_rules());
    case tok_callable:
      cur_tok++;
      return VertexAdaptor<op_type_expr_callable>::create(VertexPtr{});
    case tok_array:
      cur_tok++;
      if (vk::any_of_equal(cur_tok->type(), tok_lt, tok_oppar)) {   // array<...>
        return GenTree::create_type_help_vertex(tp_array, {parse_nested_one_type_rule()});
      }
      return GenTree::create_type_help_vertex(tp_array, {GenTree::create_type_help_vertex(tp_Unknown)});
    case tok_at: {      // @tl\...
      cur_tok++;
      if (!cur_tok->str_val.starts_with("tl\\")) {
        throw std::runtime_error("Invalid magic namespace after '@'");
      }
      return parse_classname(G->env().get_tl_namespace_prefix() + std::string(cur_tok->str_val).substr(3));
    }
    case tok_xor:       // ^1, ^2[*] (для functions.txt)
      cur_tok++;
      return parse_arg_ref();

    case tok_func_name:
      // tok_future не существует, это строка
      if (vk::any_of_equal(cur_tok->str_val, "future", "\\future")) {
        cur_tok++;
        return GenTree::create_type_help_vertex(tp_future, {parse_nested_one_type_rule()});
      }
      // аналогично future_queue
      if (cur_tok->str_val == "future_queue") {
        cur_tok++;
        return GenTree::create_type_help_vertex(tp_future_queue, {parse_nested_one_type_rule()});
      }
      // плюс некоторые специальные типы, которые не являются токенами, но имеют смысл в phpdoc / functions.txt
      if (cur_tok->str_val == "Any") {
        cur_tok++;
        return GenTree::create_type_help_vertex(tp_Any);
      }
      if (cur_tok->str_val == "UInt") {
        cur_tok++;
        return GenTree::create_type_help_vertex(tp_UInt);
      }
      if (cur_tok->str_val == "Long") {
        cur_tok++;
        return GenTree::create_type_help_vertex(tp_Long);
      }
      if (cur_tok->str_val == "ULong") {
        cur_tok++;
        return GenTree::create_type_help_vertex(tp_ULong);
      }
      if (cur_tok->str_val == "RPC") {
        cur_tok++;
        return GenTree::create_type_help_vertex(tp_RPC);
      }
      if (cur_tok->str_val == "regexp") {
        cur_tok++;
        return GenTree::create_type_help_vertex(tp_regexp);
      }
      // (для functions.txt) OrFalse<int>
      if (cur_tok->str_val == "OrFalse") {    // todo нужно ли? (или |)
        cur_tok++;
        return VertexAdaptor<op_type_expr_or_false>::create(parse_nested_one_type_rule());
      }
      // (для functions.txt) lca<int, ^1, ...> / lca(int, ^1, ...)  // todo нужно ли? (или |)
      if (cur_tok->str_val == "lca") {
        cur_tok++;
        return VertexAdaptor<op_type_expr_lca>::create(parse_nested_type_rules());
      }
      // (для functions.txt) instance<^2>
      if (cur_tok->str_val == "instance") {
        cur_tok++;
        return VertexAdaptor<op_type_expr_instance>::create(parse_nested_one_type_rule());
      }
      // иначе это трактуем как имя класса (в т.ч. с маленькой буквы)
      // работают абсолютное, относительное имя, self (учитывает use'ы файла current_function)
      return parse_classname(std::string(cur_tok->str_val));

    default:
      throw std::runtime_error(format("can't parse '%s'", std::string(cur_tok->str_val).c_str()));
  }
}

VertexPtr PhpDocTypeRuleParserUsingLexer::parse_arg_ref() {   // ^1, ^2[]
  if (cur_tok->type() != tok_int_const) {
    throw std::runtime_error("Invalid number after ^");
  }
  auto v = VertexAdaptor<op_type_expr_arg_ref>::create();
  v->int_val = std::stoi(std::string(cur_tok->str_val));

  VertexPtr res = v;
  cur_tok++;
  while (cur_tok->type() == tok_opbrk && (cur_tok + 1)->type() == tok_clbrk) {
    res = VertexAdaptor<op_index>::create(res);
    cur_tok += 2;
  }
  while (cur_tok->type() == tok_oppar && (cur_tok + 1)->type() == tok_clpar) {
    res = VertexAdaptor<op_type_expr_callback_call>::create(res);
    cur_tok += 2;
  }

  return res;
}

VertexPtr PhpDocTypeRuleParserUsingLexer::parse_type_array() {
  VertexPtr res = parse_simple_type();

  while (cur_tok->type() == tok_opbrk && (cur_tok + 1)->type() == tok_clbrk) {
    res = GenTree::create_type_help_vertex(tp_array, {res});
    cur_tok += 2;
  }
  if (cur_tok->type() == tok_varg) {      // "int ...$args" аналогично "int [] $args", даже парсит "(int|false) ..."
    cur_tok++;
    res = GenTree::create_type_help_vertex(tp_array, {res});
  }

  return res;
}

std::vector<VertexPtr> PhpDocTypeRuleParserUsingLexer::parse_nested_type_rules() {
  if (vk::none_of_equal(cur_tok->type(), tok_lt, tok_oppar)) {
    throw std::runtime_error("expected '<' or '('");
  }
  cur_tok++;
  std::vector<VertexPtr> sub_types;
  while (true) {
    sub_types.emplace_back(parse_type_expression());

    if (vk::any_of_equal(cur_tok->type(), tok_gt, tok_clpar)) {
      cur_tok++;
      break;
    } else if (cur_tok->type() == tok_comma) {
      cur_tok++;
    } else {
      throw std::runtime_error("expected '>' or ')' or ','");
    }
  }
  return sub_types;
}

VertexPtr PhpDocTypeRuleParserUsingLexer::parse_nested_one_type_rule() {
  if (vk::none_of_equal(cur_tok->type(), tok_lt, tok_oppar)) {
    throw std::runtime_error("expected '<' or '('");
  }
  cur_tok++;

  VertexPtr sub_type = parse_type_expression();
  if (vk::none_of_equal(cur_tok->type(), tok_gt, tok_clpar)) {
    throw std::runtime_error("expected '>' or ')'");
  }
  cur_tok++;

  return sub_type;
}

VertexPtr PhpDocTypeRuleParserUsingLexer::parse_type_expression() {
  VertexPtr result = parse_type_array();
  bool is_raw_bool = result->type() == op_type_expr_type && result->type_help == tp_bool && (cur_tok - 1)->type() == tok_bool;
  while (cur_tok->type() == tok_or) {
    cur_tok++;
    // lhs|rhs => lca(lhs,rhs)
    VertexPtr rhs = parse_type_array();
    result = VertexAdaptor<op_type_expr_lca>::create(result, rhs);

    is_raw_bool |= rhs->type() == op_type_expr_type && rhs->type_help == tp_bool && (cur_tok - 1)->type() == tok_bool;
    if (is_raw_bool) {
      throw std::runtime_error("Do not use |bool in phpdoc, use |false instead\n(if you really need bool, specify |boolean)");
    }
  }
  return result;
}

VertexPtr PhpDocTypeRuleParserUsingLexer::parse_from_phpdoc_tag_string(const vk::string_view &phpdoc_tag_str) {
  tokens = phpdoc_to_tokens(const_cast<char*>(phpdoc_tag_str.data()), phpdoc_tag_str.size());
  kphp_assert(tokens.back().type() == tok_end);
  cur_tok = tokens.begin();

  try {
    return parse_type_expression();
  } catch (std::runtime_error &ex) {
    stage::set_location(current_function->root->location);
    kphp_error(0, format("Failed to parse phpdoc: %s\n%s", std::string(phpdoc_tag_str).c_str(), ex.what()));
    return {};
  }
}

VertexPtr PhpDocTypeRuleParserUsingLexer::parse_from_tokens(const std::vector<Token> &phpdoc_tokens, std::vector<Token>::const_iterator &tok_iter) {
  tokens = phpdoc_tokens;
  kphp_assert(tokens.back().type() == tok_end);
  cur_tok = tok_iter;

  try {
    VertexPtr v = parse_type_expression();
    tok_iter = cur_tok;
    return v;
  } catch (std::runtime_error &ex) {
    stage::set_location(current_function->root->location);
    // todo куда сообщение об ошибке, плюс удалить дубликат
    kphp_error(0, format("Failed to parse phpdoc: %s\n%s", "todo", ex.what()));
    return {};
  }
}

VertexPtr phpdoc_parse_type(const vk::string_view &type_str, FunctionPtr current_function) {
  return phpdoc_parse_type_using_lexer(type_str, current_function);
  PhpDocTypeRuleParser parser(current_function);
  VertexPtr doc_type = parser.parse_from_type_string(type_str);

  kphp_error_act(parser.get_unknown_classes().empty(),
                 format("Could not find class in phpdoc: %s\nProbably, this class is used only in phpdoc and never created in reachable code",
                        parser.get_unknown_classes().begin()->c_str()),
                 return {});

  return doc_type;
}

VertexPtr phpdoc_parse_type_using_lexer(const vk::string_view &type_str, FunctionPtr current_function) {
  PhpDocTypeRuleParserUsingLexer parser(current_function);
  VertexPtr doc_type = parser.parse_from_phpdoc_tag_string(std::string(type_str));

  kphp_error_act(parser.get_unknown_classes().empty(),
                 format("Could not find class in phpdoc: %s", parser.get_unknown_classes().begin()->c_str()),
                 return {});

  return doc_type;
}

/*
 * Имея phpdoc-строку после тега @param/@return/@var, т.е. вида
 * "int|false $a maybe comment" или "$var tuple(int, string) maybe comment" или "A[] maybe comment"
 * распарсить тип (превратив в дерево для type_rule) и имя переменной, если оно есть
 */
PhpDocTagParseResult phpdoc_parse_type_and_var_name(const vk::string_view &phpdoc_tag_str, FunctionPtr current_function) {
  PhpDocTypeRuleParserUsingLexer parser(current_function);
  std::vector<Token> tokens = phpdoc_to_tokens(phpdoc_tag_str.data(), phpdoc_tag_str.size());
  std::string var_name;

  // $var_name phpdoc|type maybe comment
  if (tokens.front().type() == tok_var_name) {
    var_name = std::string(tokens.front().str_val);
    if (tokens.size() <= 2) {     // tok_end и всё
      return {VertexPtr{}, std::move(var_name)};
    }
  }

  stage::set_location(current_function->root->location);
  std::vector<Token>::const_iterator tok_iter = var_name.empty() ? tokens.begin() : tokens.begin() + 1;
  VertexPtr doc_type = parser.parse_from_tokens(tokens, tok_iter);

  kphp_error(parser.get_unknown_classes().empty(),
             format("Could not find class in phpdoc: %s", parser.get_unknown_classes().begin()->c_str()));

  if (var_name.empty()) {
    // phpdoc|type $var_name maybe comment
    if (tok_iter->type() == tok_var_name) {   // tok_iter — сразу после окончания парсинга
      var_name = std::string(tok_iter->str_val);
    }
  }

  return {doc_type, std::move(var_name)};
}

/*
 * Имея на входе полный phpdoc / ** ... * / с кучей тегов внутри,
 * найти первый @tag и распарсить всё что справа.
 * Возвращает result — у него есть operator bool, нашёлся/распарсился ли такой тег
 */
PhpDocTagParseResult phpdoc_find_tag(const vk::string_view &phpdoc, php_doc_tag::doc_type tag_type, FunctionPtr current_function) {
  if (auto found_tag = phpdoc_find_tag_as_string(phpdoc, tag_type)) {
    return phpdoc_parse_type_and_var_name(*found_tag, current_function);
  }
  return {VertexPtr{}, std::string()};
}

/*
 * Имея на входе полный phpdoc / ** ... * /,
 * найти все @tag и распарсить всё что справа (имеет смысл для @param, т.е. которых много).
 */
std::vector<PhpDocTagParseResult> phpdoc_find_tag_multi(const vk::string_view &phpdoc, php_doc_tag::doc_type tag_type, FunctionPtr current_function) {
  std::vector<PhpDocTagParseResult> result;
  for (const auto &tag : parse_php_doc(phpdoc)) {
    if (tag.type == tag_type) {
      if (auto parsed = phpdoc_parse_type_and_var_name(tag.value, current_function)) {
        result.emplace_back(std::move(parsed));
      }
    }
  }
  return result;
}

/*
 * Имея на входе полный phpdoc / ** ... * /,
 * найти первый @tag и просто вернуть то, что справа, как строку.
 */
vk::optional<std::string> phpdoc_find_tag_as_string(const vk::string_view &phpdoc, php_doc_tag::doc_type tag_type) {
  for (const auto &tag : parse_php_doc(phpdoc)) {
    if (tag.type == tag_type) {
      return tag.value;
    }
  }
  return {};
}

/*
 * Имея на входе полный phpdoc / ** ... * /,
 * найти все @tag и вернуть то, что справа, как строки (имеет смысл для @kphp-template, т.е. которых много)
 */
std::vector<std::string> phpdoc_find_tag_as_string_multi(const vk::string_view &phpdoc, php_doc_tag::doc_type tag_type) {
  std::vector<std::string> result;
  for (const auto &tag : parse_php_doc(phpdoc)) {
    if (tag.type == tag_type) {
      result.emplace_back(tag.value);
    }
  }
  return result;
}

bool phpdoc_tag_exists(const vk::string_view &phpdoc, php_doc_tag::doc_type tag_type) {
  return static_cast<bool>(phpdoc_find_tag_as_string(phpdoc, tag_type));
}

void PhpDocTypeRuleParser::run_tipa_unit_tests_parsing_tags() {
  struct ParsingTagsTest {
    std::string phpdoc;
    std::string var_name;
    std::string type_str;
    int offset;
    bool is_valid;

    ParsingTagsTest(std::string phpdoc, std::string var_name, std::string type_str, int offset, bool is_valid) :
      phpdoc(std::move(phpdoc)),
      var_name(std::move(var_name)),
      type_str(std::move(type_str)),
      offset(offset),
      is_valid(is_valid) {}

    static ParsingTagsTest test_pass(std::string phpdoc, std::string var_name, std::string type_str, int offset = 0) {
      return ParsingTagsTest(std::move(phpdoc), std::move(var_name), std::move(type_str), offset, true);   // CLion тут показывает ошибку, но это его косяк
    }

    static ParsingTagsTest test_fail(std::string phpdoc, std::string var_name, std::string type_str, int offset = 0) {
      return ParsingTagsTest(std::move(phpdoc), std::move(var_name), std::move(type_str), offset, false);
    }

    static void run_tests() {
      ParsingTagsTest tests[] = {
        test_pass("* @var $a bool ", "a", "bool"),
        test_pass("* @var bool $a ", "a", "bool"),
        test_pass(" *@var    bool    $a   ", "a", "bool"),
        test_pass(" *@var    $a    bool   ", "a", "bool"),
        test_pass("* @type $variable int|string comment ", "variable", "int|string"),
        test_fail("* @nothing $variable int|string comment", "", ""),
        test_fail("* only comment", "", ""),
        test_pass("* @deprecated \n* @var $k Exception|false", "k", "Exception|false"),
        test_pass("* @var mixed some comment", "", "mixed"),
        test_pass("* @var string|(false|int)[]?", "", "string|(false|int)[]?"),
        test_pass("* @var $a", "a", ""),
        test_pass("* @type hello world", "", "hello"),
        test_pass("*   @type   ", "", ""),
        test_pass("* @param $aa A \n* @var $a A  \n* @param BB $b \n* @var $b B   ", "a", "A", 0),
        test_pass("* @param $aa A \n* @var $a A  \n* @param BB $b \n* @var $b B   ", "b", "B", 1),
      };

      int n_not_passed = 0;
      for (auto &test : tests) {
        std::string var_name, type_str;
        bool found = find_tag_in_phpdoc(test.phpdoc, php_doc_tag::var, var_name, type_str, test.offset);
        bool correct = found == test.is_valid
                       && (!found || (var_name == test.var_name && type_str == test.type_str));
        if (!correct) {
          n_not_passed++;
        }

        std::string ok_str = correct
                             ? test.is_valid ? "ok" : "ok (was not parsed)"
                             : "error";
        printf("%-50s %s\n", test.phpdoc.c_str(), ok_str.c_str());
      }
      printf("Not passed count: %d\n", n_not_passed);
    }
  };

  ParsingTagsTest::run_tests();
}
