// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"

#include <array>
#include <string_view>

namespace impl_ {

// note: this class in runtime is used for classes, e.g. `JsonEncoder::encode(new A)` (also see from/to visitors)
// for untyped json_encode() and json_decode(), see json-functions.h
// todo somewhen, unify this JsonWriter and untyped JsonEncoder
class JsonWriter : vk::not_copyable {
public:
  explicit JsonWriter(bool pretty_print = false, bool preserve_zero_fraction = false) noexcept;
  ~JsonWriter() noexcept;

  bool Bool(bool b) noexcept;
  bool Int64(int64_t i) noexcept;
  bool Double(double d) noexcept;
  bool String(const string &s) noexcept;
  bool RawString(const string &s) noexcept;
  bool Null() noexcept;

  bool Key(std::string_view key, bool escape = false) noexcept;
  bool StartObject() noexcept;
  bool EndObject() noexcept;
  bool StartArray() noexcept;
  bool EndArray() noexcept;

  void SetDoublePrecision(std::size_t precision) noexcept {
    double_precision_ = precision;
  }

  bool IsComplete() const noexcept;
  string GetError() const noexcept;
  string GetJson() const noexcept;

private:
  bool NewLevel(bool is_array) noexcept;
  bool ExitLevel(bool is_array) noexcept;
  bool RegisterValue() noexcept;
  void WriteIndent() const noexcept;

  struct NestedLevel {
    bool in_array{false};
    std::uint32_t values_count{0};
  };

  constexpr static std::size_t MAX_DEPTH{64};
  std::array<NestedLevel, MAX_DEPTH> stack_;
  std::int64_t stack_top_{-1};

  string error_;
  std::size_t double_precision_{0};
  const bool pretty_print_{false};
  const bool preserve_zero_fraction_{false};
  bool has_root_{false};
  std::size_t indent_{0};
};

} // namespace impl_
