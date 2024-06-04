// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime/runtime-types.h"

#include <string_view>

namespace impl_ {

// note: this class in runtime is used for classes, e.g. `JsonEncoder::encode(new A)` (also see from/to visitors)
// for untyped json_encode() and json_decode(), see json-functions.h
// todo somewhen, unify this JsonWriter and untyped JsonEncoder
class JsonWriter : vk::not_copyable {
public:
  explicit JsonWriter(bool pretty_print = false, bool preserve_zero_fraction = false) noexcept;
  ~JsonWriter() noexcept;

  bool write_bool(bool b) noexcept;
  bool write_int(int64_t i) noexcept;
  bool write_double(double d) noexcept;
  bool write_string(const string &s) noexcept;
  bool write_raw_string(const string &s) noexcept;
  bool write_null() noexcept;

  bool write_key(std::string_view key, bool escape = false) noexcept;
  bool start_object() noexcept;
  bool end_object() noexcept;
  bool start_array() noexcept;
  bool end_array() noexcept;

  void set_double_precision(std::size_t precision) noexcept {
    double_precision_ = precision;
  }

  bool is_complete() const noexcept;
  string get_error() const noexcept;
  string get_final_json() const noexcept;

private:
  bool new_level(bool is_array) noexcept;
  bool exit_level(bool is_array) noexcept;
  bool register_value() noexcept;
  void write_indent() const noexcept;

  struct NestedLevel {
    bool in_array{false};
    std::uint32_t values_count{0};
  };

  array<NestedLevel> stack_;
  string error_;
  std::size_t double_precision_{0};
  const bool pretty_print_{false};
  const bool preserve_zero_fraction_{false};
  bool has_root_{false};
  std::size_t indent_{0};
};

} // namespace impl_
