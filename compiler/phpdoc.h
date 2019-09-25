#pragma once

#include <map>
#include <string>
#include <vector>

#include "common/wrappers/optional.h"

#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/inferring/primitive-type.h"

struct php_doc_tag {
  enum doc_type {
    unknown,
    kphp_inline,
    kphp_infer,
    kphp_disable_warnings,
    kphp_extern_func_info,
    kphp_pure_function,
    param,
    returns,
    var,
    kphp_required,
    kphp_lib_export,
    kphp_sync,
    kphp_template,
    kphp_return,
    kphp_memcache_class,
    kphp_immutable_class,
    kphp_tl_class,
    kphp_const,
  };

public:
  static doc_type get_doc_type(const std::string &str) {
    auto it = str2doc_type.find(str);
    bool found = (it != str2doc_type.end());

    return found ? it->second : unknown;
  }

public:
  std::string name;
  std::string value;
  doc_type type = unknown;
  int line_num = -1;

private:
  static const std::map<std::string, doc_type> str2doc_type;
};

struct PhpDocTagParseResult {
  VertexPtr type_expr;      // op_type_expr_*
  std::string var_name;     // без начального "$", может быть пустым, если отсутствует в phpdoc

  operator bool() const { return !!type_expr; }
};

class PhpDocTypeRuleParserUsingLexer {
  FunctionPtr current_function;
  std::vector<Token> tokens;
  std::vector<Token>::const_iterator cur_tok;
  std::vector<std::string> unknown_classes_list;

  VertexPtr parse_classname(const std::string &phpdoc_class_name);
  VertexPtr parse_simple_type();
  VertexPtr parse_arg_ref();
  VertexPtr parse_type_array();
  std::vector<VertexPtr> parse_nested_type_rules();
  VertexPtr parse_nested_one_type_rule();
  VertexPtr parse_type_expression();

public:
  explicit PhpDocTypeRuleParserUsingLexer(FunctionPtr current_function) :
    current_function(current_function) {}

  VertexPtr parse_from_tokens(const std::vector<Token> &phpdoc_tokens, std::vector<Token>::const_iterator &tok_iter);

  const std::vector<std::string> &get_unknown_classes() const { return unknown_classes_list; }
};

std::vector<php_doc_tag> parse_php_doc(const vk::string_view &phpdoc);
PhpDocTagParseResult phpdoc_parse_type_and_var_name(const vk::string_view &phpdoc_tag_str, FunctionPtr current_function);

PhpDocTagParseResult phpdoc_find_tag(const vk::string_view &phpdoc, php_doc_tag::doc_type tag_type, FunctionPtr current_function);
vk::optional<std::string> phpdoc_find_tag_as_string(const vk::string_view &phpdoc, php_doc_tag::doc_type tag_type);

std::vector<PhpDocTagParseResult> phpdoc_find_tag_multi(const vk::string_view &phpdoc, php_doc_tag::doc_type tag_type, FunctionPtr current_function);
std::vector<std::string> phpdoc_find_tag_as_string_multi(const vk::string_view &phpdoc, php_doc_tag::doc_type tag_type);

bool phpdoc_tag_exists(const vk::string_view &phpdoc, php_doc_tag::doc_type tag_type);
