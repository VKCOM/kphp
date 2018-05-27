#include "compiler/phpdoc.h"

#include <cstdio>

#include "compiler/data.h"
#include "compiler/stage.h"
#include "compiler/vertex.h"
#include "compiler/compiler-core.h"

using std::vector;
using std::string;

/*
 * Имея '@param $a A[] some description' — где this->value = '$a A[] some description' —
 * уметь вычленить первый токен '$a', потом по offset'у следующий 'A[]' и т.п. — до ближайшего пробела
 */
const std::string php_doc_tag::get_value_token (unsigned long chars_offset) const {
  unsigned long pos = 0;
  unsigned long len = value.size();
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

  return value.substr(chars_offset, pos - chars_offset);
}

vector<php_doc_tag> parse_php_doc(const string &phpdoc) {
  vector<string> lines(1);
  int have_star = false;
  for (int i = 0; i < phpdoc.size(); i++) {
    if (!have_star) {
      if (phpdoc[i] == ' ' || phpdoc[i] == '\t') {
        continue;
      }
      if (phpdoc[i] == '*') {
        have_star = true;
        continue;
      }
      kphp_error(0, "failed to parse php_doc");
      return vector<php_doc_tag>();
    }
    if (phpdoc[i] == '\n') {
      lines.push_back("");
      have_star = false;
      continue;
    }
    if (lines.back() == "" && (phpdoc[i] == ' ' || phpdoc[i] == '\t')) {
      continue;
    }
    lines.back() += phpdoc[i];
  }
  vector<php_doc_tag> result;
  result.push_back(php_doc_tag());
  for (int i = 0; i < lines.size(); i++) {
    if (lines[i][0] != '@') {
      result.back().value += ' ' + lines[i];
    } else {
      result.push_back(php_doc_tag());
      size_t pos = lines[i].find(' ');
      result.back().name = lines[i].substr(0, pos);
      if (pos != string::npos) {
        result.back().value = lines[i].substr(pos + 1);
      }
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
bool PhpDocTypeRuleParser::find_tag_in_phpdoc (const string &phpdoc, php_doc_tag::doc_type doc_type, string &out_var_name, string &out_type_str, int offset) {
  const std::vector <php_doc_tag> &tags = parse_php_doc(phpdoc);
  int idx = 0;
  for (std::vector <php_doc_tag>::const_iterator tag = tags.begin(); tag != tags.end(); ++tag) {

    if (php_doc_tag::get_doc_type(tag->name) == doc_type && idx++ >= offset) {  // для @param имеет смысл offset
      const std::string &a1 = tag->get_value_token();
      const std::string &a2 = tag->get_value_token(a1.size());

      if (a1[0] == '$') {
        out_var_name = a1.substr(1);
        out_type_str = a2;
      } else if (a2[0] == '$') {
        out_var_name = a2.substr(1);
        out_type_str = a1;
      } else {
        out_type_str = a1;
        out_var_name = "";
      }
      return true;
    }

  }
  return false;
}

VertexPtr PhpDocTypeRuleParser::create_type_help_vertex (PrimitiveType type) {
  CREATE_VERTEX(type_rule, op_type_rule);
  type_rule->type_help = type;
  return type_rule;
}

VertexPtr PhpDocTypeRuleParser::create_type_help_class_vertex (ClassPtr klass) {
  CREATE_VERTEX(type_rule, op_class_type_rule);
  type_rule->type_help = tp_Class;
  type_rule->class_ptr = klass;
  return type_rule;
}

/*
 * Имея строку '(\VK\A|false)[]' и pos=1 — найти, где заканчивается имя класса. ('\VK\A' оно в данном случае)
 */
std::string PhpDocTypeRuleParser::extract_classname_from_pos (const std::string &str, size_t pos) {
  size_t pos_end = pos;
  for (; isalnum(str[pos_end]) || str[pos_end] == '\\' || str[pos_end] == '_'; ++pos_end);

  return std::string(str, pos, pos_end - pos);
}

VertexPtr PhpDocTypeRuleParser::parse_simple_type (const string &s, size_t &pos) {
  CHECK(pos < s.size(), "Failed to parse phpdoc type: unexpected end");
  switch (s[pos]) {
    case '(': {
      pos++;
      VertexPtr v = parse_type_expression(s, pos);
      if (v.is_null()) {
        return v;
      }
      CHECK(pos < s.size() && s[pos] == ')', "Failed to parse phpdoc type: unmatching ()");
      pos++;
      return v;
    }
    case 's': {
      if (s.substr(pos, 6) == "string") {
        pos += 6;
        return create_type_help_vertex(tp_string);
      }
      if (s.substr(pos, 4) == "self") {
        pos += 4;
        return create_type_help_class_vertex(current_function->class_id);
      }
      break;
    }
    case 'i': {
      if (s.substr(pos, 7) == "integer") {
        pos += 7;
        return create_type_help_vertex(tp_int);
      }
      if (s.substr(pos, 3) == "int") {
        pos += 3;
        return create_type_help_vertex(tp_int);
      }
      break;
    }
    case 'b': {
      if (s.substr(pos, 7) == "boolean") {
        pos += 8;
        return create_type_help_vertex(tp_bool);
      }
      if (s.substr(pos, 4) == "bool") {
        pos += 4;
        return create_type_help_vertex(tp_bool);
      }
      break;
    }
    case 'f': {
      if (s.substr(pos, 5) == "float") {
        pos += 5;
        return create_type_help_vertex(tp_float);
      }
      if (s.substr(pos, 5) == "false") {
        pos += 5;
        return create_type_help_vertex(tp_False);
      }
      break;
    }
    case 'd': {
      if (s.substr(pos, 6) == "double") {
        pos += 6;
        return create_type_help_vertex(tp_float);
      }
      break;
    }
    case 'm': {
      if (s.substr(pos, 5) == "mixed") {
        pos += 5;
        return create_type_help_vertex(tp_var);
      }
      break;
    }
    case 'n': {
      if (s.substr(pos, 4) == "null") {
        pos += 4;
        return create_type_help_vertex(tp_var);
      }
      break;
    }
    case 't': {
      if (s.substr(pos, 4) == "true") {
        pos += 4;
        return create_type_help_vertex(tp_bool);
      }
      break;
    }
    case 'a': {
      if (s.substr(pos, 5) == "array") {
        pos += 5;
        CREATE_VERTEX(res, op_type_rule, create_type_help_vertex(tp_var));
        res->type_help = tp_array;
        return res;
      }
      break;
    }
    default:
      if (s[pos] == '\\' || (s[pos] >= 'A' && s[pos] <= 'Z')) {
        const std::string &class_name = extract_classname_from_pos(s, pos);
        pos += class_name.size();
        ClassPtr klass = G->get_class(resolve_uses(current_function, class_name, '\\'));
        kphp_error(klass.not_null(),
            dl_pstr("Could not find class in phpdoc: %s", s.c_str()));
        return create_type_help_class_vertex(klass);
      }
  }
  CHECK(0, "Failed to parse phpdoc type: Unknown type name");
  return VertexPtr();
}

VertexPtr PhpDocTypeRuleParser::parse_type_array (const string &s, size_t &pos) {
  VertexPtr res = parse_simple_type(s, pos);
  if (res.is_null()) {
    return res;
  }
  while (pos < s.size() && s[pos] == '[') {
    CHECK(pos + 1 < s.size() && s[pos + 1] == ']', "Failed to parse phpdoc type: unmatching []");
    CREATE_VERTEX(new_res, op_type_rule, res);
    new_res->type_help = tp_array;
    res = new_res;
    pos += 2;
  }
  return res;
}

VertexPtr PhpDocTypeRuleParser::parse_type_expression (const string &s, size_t &pos) {
  VertexPtr res = parse_type_array(s, pos);
  if (res.is_null()) {
    return res;
  }
  while (pos < s.size() && s[pos] == '|') {
    pos++;
    VertexPtr next = parse_type_array(s, pos);
    if (next.is_null()) {
      return next;
    }
    CREATE_VERTEX (rule, op_type_rule_func, res, next);
    rule->str_val = "lca";
    res = rule;
  }
  return res;
}

VertexPtr PhpDocTypeRuleParser::parse_from_type_string (const string &type_str) {
//  fprintf(stderr, "Parsing s = |%s|\n", s.c_str());
  size_t pos = 0;
  VertexPtr res = parse_type_expression(type_str, pos);
  if (res.is_null()) {
    return res;
  }
  CHECK(pos == type_str.size(), "Failed to parse phpdoc type: something left at the end after parsing");
  return res;
}

VertexPtr phpdoc_parse_type (const std::string &type_str, FunctionPtr current_function) {
  PhpDocTypeRuleParser parser(current_function);
  return parser.parse_from_type_string(type_str);
}


void PhpDocTypeRuleParser::run_tipa_unit_tests_parsing_tags () {
  struct ParsingTagsTest {
    std::string phpdoc;
    std::string var_name;
    std::string type_str;
    int offset;
    bool is_valid;

    ParsingTagsTest (std::string phpdoc, std::string var_name, std::string type_str, int offset, bool is_valid) :
        phpdoc(phpdoc),
        var_name(var_name),
        type_str(type_str),
        offset(offset),
        is_valid(is_valid) {}

    static ParsingTagsTest test_pass (std::string phpdoc, std::string var_name, std::string type_str, int offset = 0) {
      return ParsingTagsTest(phpdoc, var_name, type_str, offset, true);   // CLion тут показывает ошибку, но это его косяк
    }

    static ParsingTagsTest test_fail (std::string phpdoc, std::string var_name, std::string type_str, int offset = 0) {
      return ParsingTagsTest(phpdoc, var_name, type_str, offset, false);
    }

    static void run_tests () {
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
      for (int i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
        std::string var_name, type_str;
        bool found = find_tag_in_phpdoc(tests[i].phpdoc, php_doc_tag::var, var_name, type_str, tests[i].offset);
        bool correct = found == tests[i].is_valid
                       && (!found || (var_name == tests[i].var_name && type_str == tests[i].type_str));
        if (!correct) {
          n_not_passed++;
        }

        std::string ok_str = correct
            ? tests[i].is_valid ? "ok" : "ok (was not parsed)"
            : "error";
        printf("%-50s %s\n", tests[i].phpdoc.c_str(), ok_str.c_str());
      }
      printf("Not passed count: %d\n", n_not_passed);
    }
  };

  ParsingTagsTest::run_tests();
}
