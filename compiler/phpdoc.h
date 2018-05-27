#pragma once

#include <string>
#include <vector>

#include "compiler/data_ptr.h"
#include "compiler/types.h"

struct php_doc_tag {
  enum doc_type {
    unknown,
    kphp_inline,
    kphp_infer,
    kphp_disable_warnings,
    param,
    returns,
    var,
    kphp_required,
    kphp_sync
  };

public:
  static doc_type get_doc_type(const std::string &str) {
    __typeof(str2doc_type.begin()) it = str2doc_type.find(str);

    if (it == str2doc_type.end()) {
      return unknown;
    }

    return it->second;
  }

private:
  static std::map<std::string, doc_type> init_str2doc_type() {
    std::map<std::string, doc_type> res;

    res["@param"] = param;
    res["@kphp-inline"] = kphp_inline;
    res["@kphp-infer"] = kphp_infer;
    res["@kphp-disable-warnings"] = kphp_disable_warnings;
    res["@kphp-required"] = kphp_required;
    res["@kphp-sync"] = kphp_sync;
    res["@type"] = var;
    res["@var"] = var;
    res["@return"] = returns;
    res["@returns"] = returns;

    return res;
  }

public:
  std::string name;
  std::string value;
  doc_type type;

  const std::string get_value_token(unsigned long chars_offset = 0) const;

private:
  static const std::map<std::string, doc_type> str2doc_type;
};

class PhpDocTypeRuleParser {
  FunctionPtr current_function;

  VertexPtr create_type_help_vertex (PrimitiveType type);
  VertexPtr create_type_help_class_vertex (ClassPtr klass);
  std::string extract_classname_from_pos (const std::string &str, size_t pos);
  VertexPtr parse_simple_type (const string &s, size_t &pos);
  VertexPtr parse_type_array (const string &s, size_t &pos);
  VertexPtr parse_type_expression (const string &s, size_t &pos);

public:
  PhpDocTypeRuleParser (FunctionPtr current_function) :
      current_function(current_function) {}

  VertexPtr parse_from_type_string (const string &type_str);

  static bool find_tag_in_phpdoc (const string &phpdoc, php_doc_tag::doc_type doc_type, string &out_var_name, string &out_type_str, int offset = 0);
  static void run_tipa_unit_tests_parsing_tags ();
};

std::vector <php_doc_tag> parse_php_doc (const std::string &phpdoc);
VertexPtr phpdoc_parse_type (const std::string &type_str, FunctionPtr current_function);
