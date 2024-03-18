// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <ostream>

#include "common/tl2php/php-classes.h"

namespace vk {
namespace tl {

struct SkipLine {
  friend std::ostream &operator<<(std::ostream &os, const SkipLine &) {
    return os << std::endl << std::endl;
  }
};

struct PhpTag {
  friend std::ostream &operator<<(std::ostream &os, const PhpTag &) {
    return os << "<?php" << SkipLine{};
  }
};

struct ClassNameWithNamespace {
  const PhpClassRepresentation &class_repr;

  friend std::ostream &operator<<(std::ostream &os, const ClassNameWithNamespace &self) {
    return os << self.class_repr.php_class_namespace << '\\' << self.class_repr.php_class_name;
  }
};

struct PhpVariable {
  // implicit conversation is ok
  PhpVariable(const PhpClassField &field)
    : name(field.field_name)
    , type(field.field_value_type)
    , php_doc_type(field.php_doc_type)
    , under_field_mask(!field.field_mask_name.empty()) {}

  PhpVariable(std::string field_name, php_field_type field_type, vk::string_view field_php_doc_type)
    : name(std::move(field_name))
    , type(field_type)
    , php_doc_type(field_php_doc_type) {}

  std::string name;
  php_field_type type;
  vk::string_view php_doc_type;
  bool under_field_mask{false};
};

struct DefaultValue {
  const PhpVariable &variable;

  friend std::ostream &operator<<(std::ostream &os, const DefaultValue &self) {
    if (self.variable.under_field_mask) {
      return os << (self.variable.type == php_field_type::t_bool_true ? "false" : "null");
    }
    switch (self.variable.type) {
      case php_field_type::t_string:
        return os << "''";
      case php_field_type::t_int:
      case php_field_type::t_mixed:
        return os << "0";
      case php_field_type::t_double:
        return os << "0.0";
      case php_field_type::t_array:
        return os << "[]";
      case php_field_type::t_bool:
        return os << "false";
      case php_field_type::t_bool_true:
        return os << "true";
      case php_field_type::t_maybe:
      case php_field_type::t_class:
        return os << "null";
      default:
        assert(0);
    }
    return os;
  }
};

} // namespace tl
} // namespace vk
