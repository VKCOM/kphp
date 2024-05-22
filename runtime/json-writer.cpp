// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/json-writer.h"

#include "runtime/array_functions.h"
#include "runtime/math_functions.h"
#include "runtime/kphp-runtime-context.h"

// note: json-writer.cpp is used for classes, e.g. `JsonEncoder::encode(new A)` (also see from/to visitors)
// for untyped json_encode() and json_decode(), see json-functions.cpp
namespace impl_ {

static void escape_json_string(string_buffer &buffer, std::string_view s) noexcept {
  for (auto c : s) {
    switch (c) {
      case '"':
        buffer.append_char('\\');
        buffer.append_char('"');
        break;
      case '\\':
        buffer.append_char('\\');
        buffer.append_char('\\');
        break;
      case '/':
        buffer.append_char('\\');
        buffer.append_char('/');
        break;
      case '\b':
        buffer.append_char('\\');
        buffer.append_char('b');
        break;
      case '\f':
        buffer.append_char('\\');
        buffer.append_char('f');
        break;
      case '\n':
        buffer.append_char('\\');
        buffer.append_char('n');
        break;
      case '\r':
        buffer.append_char('\\');
        buffer.append_char('r');
        break;
      case '\t':
        buffer.append_char('\\');
        buffer.append_char('t');
        break;
      default:
        buffer.append_char(c);
        break;
    }
  }
}

JsonWriter::JsonWriter(bool pretty_print, bool preserve_zero_fraction) noexcept
  : pretty_print_(pretty_print)
  , preserve_zero_fraction_(preserve_zero_fraction) {
  vk::singleton<KphpRuntimeContext>::get().static_SB.clean();
}

JsonWriter::~JsonWriter() noexcept {
  vk::singleton<KphpRuntimeContext>::get().static_SB.clean();
}

bool JsonWriter::write_bool(bool b) noexcept {
  if (!register_value()) {
    return false;
  }
  b ? vk::singleton<KphpRuntimeContext>::get().static_SB.append("true", 4) : vk::singleton<KphpRuntimeContext>::get().static_SB.append("false", 5);
  return true;
}

bool JsonWriter::write_int(int64_t i) noexcept {
  if (!register_value()) {
    return false;
  }
  vk::singleton<KphpRuntimeContext>::get().static_SB << i;
  return true;
}

bool JsonWriter::write_double(double d) noexcept {
  if (!register_value()) {
    return false;
  }
  if (std::isnan(d) || std::isinf(d)) {
    d = 0.0;
  }
  if (double_precision_) {
    vk::singleton<KphpRuntimeContext>::get().static_SB << f$round(d, double_precision_);
  } else {
    vk::singleton<KphpRuntimeContext>::get().static_SB << d;
  }
  if (preserve_zero_fraction_) {
    if (double dummy = 0.0; std::modf(d, &dummy) == 0.0) {
      vk::singleton<KphpRuntimeContext>::get().static_SB << ".0";
    }
  }
  return true;
}

bool JsonWriter::write_string(const string &s) noexcept {
  if (!register_value()) {
    return false;
  }
  vk::singleton<KphpRuntimeContext>::get().static_SB.reserve(2 * s.size() + 2);
  vk::singleton<KphpRuntimeContext>::get().static_SB.append_char('"');
  escape_json_string(vk::singleton<KphpRuntimeContext>::get().static_SB, {s.c_str(), s.size()});
  vk::singleton<KphpRuntimeContext>::get().static_SB.append_char('"');
  return true;
}

bool JsonWriter::write_raw_string(const string &s) noexcept {
  if (!register_value()) {
    return false;
  }
  vk::singleton<KphpRuntimeContext>::get().static_SB << s;
  return true;
}

bool JsonWriter::write_null() noexcept {
  if (!register_value()) {
    return false;
  }
  vk::singleton<KphpRuntimeContext>::get().static_SB.append("null", 4);
  return true;
}

bool JsonWriter::write_key(std::string_view key, bool escape) noexcept {
  if (stack_.empty() || stack_.back().in_array) {
    error_.append("json key is allowed only inside object");
    return false;
  }
  if (stack_.back().values_count) {
    vk::singleton<KphpRuntimeContext>::get().static_SB << ',';
  }
  if (pretty_print_) {
    vk::singleton<KphpRuntimeContext>::get().static_SB << '\n';
    write_indent();
  }
  vk::singleton<KphpRuntimeContext>::get().static_SB << '"';
  if (escape) {
    escape_json_string(vk::singleton<KphpRuntimeContext>::get().static_SB, key);
  } else {
    vk::singleton<KphpRuntimeContext>::get().static_SB.append(key.data(), key.size());
  }
  vk::singleton<KphpRuntimeContext>::get().static_SB << '"';
  vk::singleton<KphpRuntimeContext>::get().static_SB << ':';
  if (pretty_print_) {
    vk::singleton<KphpRuntimeContext>::get().static_SB << ' ';
  }
  return true;
}

bool JsonWriter::start_object() noexcept {
  return new_level(false);
}

bool JsonWriter::end_object() noexcept {
  return exit_level(false);
}

bool JsonWriter::start_array() noexcept {
  return new_level(true);
}

bool JsonWriter::end_array() noexcept {
  return exit_level(true);
}

bool JsonWriter::is_complete() const noexcept {
  return error_.empty() && stack_.empty() && has_root_;
}

string JsonWriter::get_error() const noexcept {
  return error_;
}

string JsonWriter::get_final_json() const noexcept {
  return vk::singleton<KphpRuntimeContext>::get().static_SB.str();
}

bool JsonWriter::new_level(bool is_array) noexcept {
  if (!register_value()) {
    return false;
  }
  stack_.emplace_back(NestedLevel{.in_array = is_array});

  vk::singleton<KphpRuntimeContext>::get().static_SB << (is_array ? '[' : '{');
  indent_ += 4;
  return true;
}

bool JsonWriter::exit_level(bool is_array) noexcept {
  if (stack_.empty()) {
    error_.append("brace disbalance");
    return false;
  }

  auto cur_level = stack_.pop();
  if (cur_level.in_array != is_array) {
    error_.append("attempt to enclosure ");
    error_.push_back(cur_level.in_array ? '[' : '{');
    error_.append(" with ");
    error_.push_back(is_array ? ']' : '}');
    return false;
  }

  indent_ -= 4;
  if (pretty_print_ && cur_level.values_count) {
    vk::singleton<KphpRuntimeContext>::get().static_SB << '\n';
    write_indent();
  }

  vk::singleton<KphpRuntimeContext>::get().static_SB << (is_array ? ']' : '}');
  return true;
}

bool JsonWriter::register_value() noexcept {
  if (has_root_ && stack_.empty()) {
    error_.append("attempt to set value twice in a root of json");
    return false;
  }
  if (stack_.empty()) {
    has_root_ = true;
    return true;
  }
  auto &top = stack_.back();
  if (top.in_array) {
    if (top.values_count) {
      vk::singleton<KphpRuntimeContext>::get().static_SB << ',';
    }
    if (pretty_print_) {
      vk::singleton<KphpRuntimeContext>::get().static_SB << '\n';
      write_indent();
    }
  }
  ++top.values_count;

  return true;
}

void JsonWriter::write_indent() const noexcept {
  if (indent_) {
    vk::singleton<KphpRuntimeContext>::get().static_SB.reserve(indent_);
    for (std::size_t i = 0; i < indent_; ++i) {
      vk::singleton<KphpRuntimeContext>::get().static_SB.append_char(' ');
    }
  }
}

} // namespace impl_
