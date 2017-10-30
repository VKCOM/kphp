#include "compiler/phpdoc.h"

#include <cstdio>

#include "compiler/data.h"
#include "compiler/stage.h"
#include "compiler/vertex.h"

using std::vector;
using std::string;

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

VertexPtr phpdoc_parse_type_expression(const string &s, size_t &pos);

VertexPtr phpdoc_create_simple_vertex(PrimitiveType type) {
  CREATE_VERTEX(type_rule, op_type_rule);
  type_rule->type_help = type;
  return type_rule;
}

VertexPtr phpdoc_parse_simple_type(const string &s, size_t &pos) {
  CHECK(pos < s.size(), "Failed to parse phpdoc type: unexpected end");
  switch (s[pos]) {
    case '(': {
      pos++;
      VertexPtr v = phpdoc_parse_type_expression(s, pos);
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
        return phpdoc_create_simple_vertex(tp_string);
      }
      break;
    }
    case 'i': {
      if (s.substr(pos, 3) == "int") {
        pos += 3;
        return phpdoc_create_simple_vertex(tp_int);
      }
      if (s.substr(pos, 7) == "integer") {
        pos += 7;
        return phpdoc_create_simple_vertex(tp_int);
      }
      break;
    }
    case 'b': {
      if (s.substr(pos, 4) == "bool") {
        pos += 4;
        return phpdoc_create_simple_vertex(tp_bool);
      }
      if (s.substr(pos, 7) == "boolean") {
        pos += 8;
        return phpdoc_create_simple_vertex(tp_bool);
      }
      break;
    }
    case 'f': {
      if (s.substr(pos, 5) == "float") {
        pos += 5;
        return phpdoc_create_simple_vertex(tp_float);
      }
      if (s.substr(pos, 5) == "false") {
        pos += 5;
        return phpdoc_create_simple_vertex(tp_False);
      }
      break;
    }
    case 'd': {
      if (s.substr(pos, 6) == "double") {
        pos += 6;
        return phpdoc_create_simple_vertex(tp_float);
      }
      break;
    }
    case 'm': {
      if (s.substr(pos, 5) == "mixed") {
        pos += 5;
        return phpdoc_create_simple_vertex(tp_var);
      }
      break;
    }
    case 'n': {
      if (s.substr(pos, 4) == "null") {
        pos += 4;
        return phpdoc_create_simple_vertex(tp_var);
      }
      break;
    }
    case 't': {
      if (s.substr(pos, 4) == "true") {
        pos += 4;
        return phpdoc_create_simple_vertex(tp_bool);
      }
      break;
    }
    case 'a': {
      if (s.substr(pos, 5) == "array") {
        pos += 5;
        CREATE_VERTEX(res, op_type_rule, phpdoc_create_simple_vertex(tp_var));
        res->type_help = tp_array;
        return res;
      }
      break;
    }
  }
  CHECK(0, "Failed to parse phpdoc type: Unknown type name");
  return VertexPtr();
}

VertexPtr phpdoc_parse_type_array(const string &s, size_t &pos) {
  VertexPtr res = phpdoc_parse_simple_type(s, pos);
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

VertexPtr phpdoc_parse_type_expression(const string &s, size_t &pos) {
  VertexPtr res = phpdoc_parse_type_array(s, pos);
  if (res.is_null()) {
    return res;
  }
  while (pos < s.size() && s[pos] == '|') {
    pos++;
    VertexPtr next = phpdoc_parse_type_array(s, pos);
    if (next.is_null()) {
      return next;
    }
    CREATE_VERTEX (rule, op_type_rule_func, res, next);
    rule->str_val = "lca";
    res = rule;
  }
  return res;
}

VertexPtr phpdoc_parse_type(const string &s, size_t pos) {
//  fprintf(stderr, "Parsing s = |%s|\n", s.c_str());
  VertexPtr res = phpdoc_parse_type_expression(s, pos);
  if (res.is_null()) {
    return res;
  }
  CHECK(pos == s.size(), "Failed to parse phpdoc type: something left at the end after parsing");
  return res;
}
