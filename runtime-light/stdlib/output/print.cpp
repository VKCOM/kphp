// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/output/print.h"

#include <cstdint>

#include "runtime-core/runtime-core.h"
#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/component/component.h"
#include "runtime-light/stdlib/output/output-buffer.h"
#include "runtime-light/utils/context.h"

namespace {

void do_print_r(const mixed &v, int32_t depth) noexcept {
  if (depth == 10) {
    php_warning("Depth %d reached. Recursion?", depth);
    return;
  }

  Response &httpResponse{get_component_context()->response};
  string_buffer &coub{httpResponse.output_buffers[httpResponse.current_buffer]};
  switch (v.get_type()) {
    case mixed::type::NUL:
      break;
    case mixed::type::BOOLEAN:
      if (v.as_bool()) {
        coub << '1';
      }
      break;
    case mixed::type::INTEGER:
      coub << v.as_int();
      break;
    case mixed::type::FLOAT:
      coub << v.as_double();
      break;
    case mixed::type::STRING:
      coub << v.as_string();
      break;
    case mixed::type::ARRAY: {
      coub << "Array\n";

      string shift(depth << 3, ' ');
      coub << shift << "(\n";

      for (array<mixed>::const_iterator it = v.as_array().begin(); it != v.as_array().end(); ++it) {
        coub << shift << "    [" << it.get_key() << "] => ";
        do_print_r(it.get_value(), depth + 1);
        coub << '\n';
      }

      coub << shift << ")\n";
      break;
    }
    case mixed::type::OBJECT: {
      php_warning("print_r used on object");
      coub << v.as_object()->get_class();
      break;
    }
    default:
      php_critical_error("non-exhaustive switch");
  }
}

void do_var_dump(const mixed &v, int32_t depth) noexcept {
  if (depth == 10) {
    php_warning("Depth %d reached. Recursion?", depth);
    return;
  }

  string shift(depth * 2, ' ');
  Response &httpResponse{get_component_context()->response};
  string_buffer &coub{httpResponse.output_buffers[httpResponse.current_buffer]};
  switch (v.get_type()) {
    case mixed::type::NUL:
      coub << shift << "NULL";
      break;
    case mixed::type::BOOLEAN:
      coub << shift << "bool(" << (v.as_bool() ? "true" : "false") << ')';
      break;
    case mixed::type::INTEGER:
      coub << shift << "int(" << v.as_int() << ')';
      break;
    case mixed::type::FLOAT:
      coub << shift << "float(" << v.as_double() << ')';
      break;
    case mixed::type::STRING:
      coub << shift << "string(" << static_cast<int32_t>(v.as_string().size()) << ") \"" << v.as_string() << '"';
      break;
    case mixed::type::ARRAY: {
      coub << shift << "array(" << v.as_array().count() << ") {\n";

      for (array<mixed>::const_iterator it = v.as_array().begin(); it != v.as_array().end(); ++it) {
        coub << shift << "  [";
        if (array<mixed>::is_int_key(it.get_key())) {
          coub << it.get_key();
        } else {
          coub << '"' << it.get_key() << '"';
        }
        coub << "]=>\n";
        do_var_dump(it.get_value(), depth + 1);
      }

      coub << shift << "}";
      break;
    }
    case mixed::type::OBJECT: {
      php_warning("var_dump used on object");
      auto s = string(v.as_object()->get_class(), static_cast<string::size_type>(strlen(v.as_object()->get_class())));
      coub << shift << "string(" << static_cast<int32_t>(s.size()) << ") \"" << s << '"';
      break;
    }
    default:
      php_critical_error("non-exhaustive switch");
  }
  coub << '\n';
}

void var_export_escaped_string(const string &s) noexcept {
  Response &httpResponse{get_component_context()->response};
  string_buffer &coub{httpResponse.output_buffers[httpResponse.current_buffer]};
  for (string::size_type i = 0; i < s.size(); i++) {
    switch (s[i]) {
      case '\'':
      case '\\':
        coub << "\\" << s[i];
        break;
      case '\0':
        coub << R"(' . "\0" . ')";
        break;
      default:
        coub << s[i];
    }
  }
}

void do_var_export(const mixed &v, int32_t depth, char endc = 0) noexcept {
  if (depth == 10) {
    php_warning("Depth %d reached. Recursion?", depth);
    return;
  }

  string shift(depth * 2, ' ');
  Response &httpResponse{get_component_context()->response};
  string_buffer &coub{httpResponse.output_buffers[httpResponse.current_buffer]};
  switch (v.get_type()) {
    case mixed::type::NUL:
      coub << shift << "NULL";
      break;
    case mixed::type::BOOLEAN:
      coub << shift << (v.as_bool() ? "true" : "false");
      break;
    case mixed::type::INTEGER:
      coub << shift << v.as_int();
      break;
    case mixed::type::FLOAT:
      coub << shift << v.as_double();
      break;
    case mixed::type::STRING:
      coub << shift << '\'';
      var_export_escaped_string(v.as_string());
      coub << '\'';
      break;
    case mixed::type::ARRAY: {
      const bool is_vector = v.as_array().is_vector();
      coub << shift << "array(\n";

      for (array<mixed>::const_iterator it = v.as_array().begin(); it != v.as_array().end(); ++it) {
        if (!is_vector) {
          coub << shift;
          if (array<mixed>::is_int_key(it.get_key())) {
            coub << it.get_key();
          } else {
            coub << '\'' << it.get_key() << '\'';
          }
          coub << " =>";
          if (it.get_value().is_array()) {
            coub << "\n";
            do_var_export(it.get_value(), depth + 1, ',');
          } else {
            do_var_export(it.get_value(), 1, ',');
          }
        } else {
          do_var_export(it.get_value(), depth + 1, ',');
        }
      }

      coub << shift << ")";
      break;
    }
    case mixed::type::OBJECT: {
      coub << shift << v.get_type_or_class_name();
      break;
    }
    default:
      php_critical_error("non-exhaustive switch");
  }
  if (endc != 0) {
    coub << endc;
  }
  coub << '\n';
}

} // namespace

string f$var_export(const mixed &v, bool buffered) noexcept {
  if (buffered) {
    f$ob_start();
    do_var_export(v, 0);
    return f$ob_get_clean().val();
  }
  do_var_export(v, 0);
  return {};
}

string f$print_r(const mixed &v, bool buffered) noexcept {
  if (buffered) {
    f$ob_start();
    do_print_r(v, 0);
    return f$ob_get_clean().val();
  }

  do_print_r(v, 0);
  return {};
}

void f$var_dump(const mixed &v) noexcept {
  do_var_dump(v, 0);
}
