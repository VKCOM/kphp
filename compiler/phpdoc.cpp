#include "compiler/phpdoc.h"

#include <cstdio>

#include "compiler/compiler-core.h"
#include "compiler/stage.h"
#include "compiler/vertex.h"

using std::vector;
using std::string;

const std::map<string, php_doc_tag::doc_type> php_doc_tag::str2doc_type = {
  {"@param",                 param},
  {"@kphp-inline",           kphp_inline},
  {"@kphp-infer",            kphp_infer},
  {"@kphp-required",         kphp_required},
  {"@kphp-sync",             kphp_sync},
  {"@type",                  var},
  {"@var",                   var},
  {"@return",                returns},
  {"@returns",               returns},
  {"@kphp-disable-warnings", kphp_disable_warnings},
  {"@kphp-template",         kphp_template},
  {"@kphp-return",           kphp_return}
};

/*
 * Имея '@param $a A[] some description' — где this->value = '$a A[] some description' —
 * уметь вычленить первый токен '$a', потом по offset'у следующий 'A[]' и т.п. — до ближайшего пробела
 */
const std::string php_doc_tag::get_value_token(unsigned long chars_offset) const {
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

vector<php_doc_tag> parse_php_doc(const vk::string_view &phpdoc) {
  int line_num_of_function_declaration = stage::get_line();

  vector<string> lines(1);
  bool have_star = false;
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
      result.back().type = php_doc_tag::get_doc_type(result.back().name);
      if (pos != string::npos) {
        result.back().value = lines[i].substr(pos + 1);
      }
    }

    if (line_num_of_function_declaration > 0) {
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
  const std::vector<php_doc_tag> &tags = parse_php_doc(phpdoc);
  int idx = 0;
  for (std::vector<php_doc_tag>::const_iterator tag = tags.begin(); tag != tags.end(); ++tag) {

    if (php_doc_tag::get_doc_type(tag->name) == doc_type && idx++ >= offset) {  // для @param имеет смысл offset
      const std::string &a1 = tag->get_value_token();
      const std::string &a2 = tag->get_value_token(a1.size());

      if (!a1.empty() && a1[0] == '$') {
        out_var_name = a1.substr(1);
        out_type_str = a2;
      } else if (!a2.empty() && a2[0] == '$') {
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

VertexPtr PhpDocTypeRuleParser::create_type_help_vertex(PrimitiveType type) {
  auto type_rule = VertexAdaptor<op_type_rule>::create();
  type_rule->type_help = type;
  return type_rule;
}

VertexPtr PhpDocTypeRuleParser::create_type_help_class_vertex(ClassPtr klass) {
  auto type_rule = VertexAdaptor<op_class_type_rule>::create();
  type_rule->type_help = tp_Class;
  type_rule->class_ptr = klass;
  return type_rule;
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
        auto res = VertexAdaptor<op_type_rule>::create(create_type_help_vertex(tp_var));
        res->type_help = tp_array;
        return res;
      }
      break;
    }
    default: {
      if (s[pos] == '\\' || (s[pos] >= 'A' && s[pos] <= 'Z')) {
        const vk::string_view &class_name = extract_classname_from_pos(s, pos);
        pos += class_name.size();
        ClassPtr klass = G->get_class(resolve_uses(current_function, static_cast<string>(class_name), '\\'));
        kphp_error(klass, dl_pstr("Could not find class in phpdoc: %s\nProbably, this class is used only in phpdoc and never created in reachable code", string(class_name).c_str()));
        return create_type_help_class_vertex(klass);
      }
    }
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
    auto new_res = VertexAdaptor<op_type_rule>::create(res);
    new_res->type_help = tp_array;
    res = new_res;
    pos += 2;
  }
  return res;
}

VertexPtr PhpDocTypeRuleParser::parse_type_expression(const vk::string_view &s, size_t &pos) {
  VertexPtr res = parse_type_array(s, pos);
  if (!res) {
    return res;
  }
  while (pos < s.size() && s[pos] == '|') {
    pos++;
    VertexPtr next = parse_type_array(s, pos);
    if (!next) {
      return next;
    }
    auto rule = VertexAdaptor<op_type_rule_func>::create(res, next);
    rule->str_val = "lca";
    res = rule;
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
  CHECK(pos == type_str.size(), "Failed to parse phpdoc type: something left at the end after parsing");
  return res;
}

VertexPtr phpdoc_parse_type(const vk::string_view &type_str, FunctionPtr current_function) {
  PhpDocTypeRuleParser parser(current_function);
  return parser.parse_from_type_string(type_str);
}


void PhpDocTypeRuleParser::run_tipa_unit_tests_parsing_tags() {
  struct ParsingTagsTest {
    std::string phpdoc;
    std::string var_name;
    std::string type_str;
    int offset;
    bool is_valid;

    ParsingTagsTest(std::string phpdoc, std::string var_name, std::string type_str, int offset, bool is_valid) :
      phpdoc(phpdoc),
      var_name(var_name),
      type_str(type_str),
      offset(offset),
      is_valid(is_valid) {}

    static ParsingTagsTest test_pass(std::string phpdoc, std::string var_name, std::string type_str, int offset = 0) {
      return ParsingTagsTest(phpdoc, var_name, type_str, offset, true);   // CLion тут показывает ошибку, но это его косяк
    }

    static ParsingTagsTest test_fail(std::string phpdoc, std::string var_name, std::string type_str, int offset = 0) {
      return ParsingTagsTest(phpdoc, var_name, type_str, offset, false);
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
