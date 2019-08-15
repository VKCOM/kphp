#pragma once

#include <map>
#include <string>
#include <vector>

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

  std::string get_value_token(size_t chars_offset = 0) const;

public:
  std::string name;
  std::string value;
  doc_type type = unknown;
  int line_num = -1;

private:
  static const std::map<std::string, doc_type> str2doc_type;
};

class PhpDocTypeRuleParser {
  FunctionPtr current_function;
  std::vector<std::string> unknown_classes_list;

  VertexPtr create_type_help_vertex(PrimitiveType type);
  vk::string_view extract_classname_from_pos(const vk::string_view &str, size_t pos);
  VertexPtr parse_simple_type(const vk::string_view &s, size_t &pos);
  VertexPtr parse_type_array(const vk::string_view &s, size_t &pos);
  VertexPtr parse_nested_type_rule(const vk::string_view &s, size_t &pos, PrimitiveType type_help);
  VertexPtr parse_type_expression(const vk::string_view &s, size_t &pos);

public:
  explicit PhpDocTypeRuleParser(FunctionPtr current_function) :
    current_function(current_function) {}

  VertexPtr parse_from_type_string(const vk::string_view &type_str);

  const std::vector<std::string> &get_unknown_classes() const { return unknown_classes_list; }

  static bool find_tag_in_phpdoc(const vk::string_view &phpdoc, php_doc_tag::doc_type doc_type, std::string &out_var_name, std::string &out_type_str, int offset = 0);
  static bool is_tag_in_phpdoc(const vk::string_view &phpdoc, php_doc_tag::doc_type doc_type);
  static void run_tipa_unit_tests_parsing_tags();
};

std::vector<php_doc_tag> parse_php_doc(const vk::string_view &phpdoc);
VertexPtr phpdoc_parse_type(const vk::string_view &type_str, FunctionPtr current_function);
