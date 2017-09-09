#pragma once

#include <string>
#include <vector>

#include "compiler/data_ptr.h"

struct php_doc_tag {
  std::string name;
  std::string value;
};

std::vector<php_doc_tag> parse_php_doc(const std::string &phpdoc);
VertexPtr phpdoc_parse_type(const std::string &s, size_t pos = 0);