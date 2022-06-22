// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/json-writer.h"

#include "runtime/math_functions.h"

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
  static_SB.clean();
}

JsonWriter::~JsonWriter() noexcept {
  static_SB.clean();
}

bool JsonWriter::Bool(bool b) noexcept {
  if (!RegisterValue()) {
    return false;
  }
  b ? static_SB.append("true", 4) : static_SB.append("false", 5);
  return true;
}

bool JsonWriter::Int64(int64_t i) noexcept {
  if (!RegisterValue()) {
    return false;
  }
  static_SB << i;
  return true;
}

bool JsonWriter::Double(double d) noexcept {
  if (!RegisterValue()) {
    return false;
  }
  if (std::isnan(d) || std::isinf(d)) {
    d = 0.0;
  }
  if (double_precision_) {
    static_SB << f$round(d, double_precision_);
  } else {
    static_SB << d;
  }
  if (preserve_zero_fraction_) {
    if (double dummy = 0.0; std::modf(d, &dummy) == 0.0) {
      static_SB << ".0";
    }
  }
  return true;
}

bool JsonWriter::String(const string &s) noexcept {
  if (!RegisterValue()) {
    return false;
  }
  static_SB.reserve(2 * s.size() + 2);
  static_SB.append_char('"');
  escape_json_string(static_SB, {s.c_str(), s.size()});
  static_SB.append_char('"');
  return true;
}

bool JsonWriter::RawString(const string &s) noexcept {
  if (!RegisterValue()) {
    return false;
  }
  static_SB << s;
  return true;
}

bool JsonWriter::Null() noexcept {
  if (!RegisterValue()) {
    return false;
  }
  static_SB.append("null", 4);
  return true;
}

bool JsonWriter::Key(std::string_view key, bool escape) noexcept {
  if (stack_top_ == -1 || stack_[stack_top_].in_array) {
    error_.append("json key is allowed only inside object");
    return false;
  }
  if (stack_[stack_top_].values_count) {
    static_SB << ',';
  }
  if (pretty_print_) {
    static_SB << '\n';
    WriteIndent();
  }
  static_SB << '"';
  if (escape) {
    escape_json_string(static_SB, key);
  } else {
    static_SB.append(key.data(), key.size());
  }
  static_SB << '"';
  static_SB << ':';
  if (pretty_print_) {
    static_SB << ' ';
  }
  return true;
}

bool JsonWriter::StartObject() noexcept {
  return NewLevel(false);
}

bool JsonWriter::EndObject() noexcept {
  return ExitLevel(false);
}

bool JsonWriter::StartArray() noexcept {
  return NewLevel(true);
}

bool JsonWriter::EndArray() noexcept {
  return ExitLevel(true);
}

bool JsonWriter::IsComplete() const noexcept {
  return error_.empty() && stack_top_ == -1 && has_root_;
}

string JsonWriter::GetError() const noexcept {
  return error_;
}

string JsonWriter::GetJson() const noexcept {
  return static_SB.str();
}

bool JsonWriter::NewLevel(bool is_array) noexcept {
  if (!RegisterValue()) {
    return false;
  }
  ++stack_top_;
  if (stack_top_ == MAX_DEPTH) {
    error_.append("stack overflow, max depth level is ");
    error_.append(static_cast<std::int64_t>(MAX_DEPTH));
    return false;
  }
  stack_[stack_top_] = NestedLevel{.in_array = is_array};

  static_SB << (is_array ? '[' : '{');
  indent_ += 4;
  return true;
}

bool JsonWriter::ExitLevel(bool is_array) noexcept {
  if (stack_top_ == -1) {
    error_.append("brace disbalance");
    return false;
  }
  auto cur_level = stack_[stack_top_];
  --stack_top_;
  if (cur_level.in_array != is_array) {
    error_.append("attempt to enclosure ");
    error_.push_back(cur_level.in_array ? '[' : '{');
    error_.append(" with ");
    error_.push_back(is_array ? ']' : '}');
    return false;
  }

  indent_ -= 4;
  if (pretty_print_ && cur_level.values_count) {
    static_SB << '\n';
    WriteIndent();
  }

  static_SB << (is_array ? ']' : '}');
  return true;
}

bool JsonWriter::RegisterValue() noexcept {
  if (has_root_ && stack_top_ == -1) {
    error_.append("attempt to set value twice in a root of json");
    return false;
  }
  if (stack_top_ == -1) {
    has_root_ = true;
    return true;
  }
  if (stack_[stack_top_].in_array) {
    if (stack_[stack_top_].values_count) {
      static_SB << ',';
    }
    if (pretty_print_) {
      static_SB << '\n';
      WriteIndent();
    }
  }
  ++stack_[stack_top_].values_count;

  return true;
}

void JsonWriter::WriteIndent() const noexcept {
  if (indent_) {
    static_SB.reserve(indent_);
    for (std::size_t i = 0; i < indent_; ++i) {
      static_SB.append_char(' ');
    }
  }
}

} // namespace impl_
