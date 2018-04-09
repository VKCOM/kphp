#pragma once

#include <string>
#include <vector>

#include "compiler/data_ptr.h"

struct php_doc_tag {
  enum doc_type {
    unknown,
    kphp_inline,
    kphp_infer,
    kphp_disable_warnings,
    param,
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

    return res;
  }

public:
  std::string name;
  std::string value;
  doc_type type;

private:
  static const std::map<std::string, doc_type> str2doc_type;
};

std::vector<php_doc_tag> parse_php_doc(const std::string &phpdoc);
VertexPtr phpdoc_parse_type(const std::string &s, size_t pos = 0);
