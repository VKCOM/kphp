// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/string/string-functions.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/file/file-system-functions.h"
#include "runtime-light/stdlib/file/resource.h"
#include "runtime-light/stdlib/output/output-state.h"

// === print ======================================================================================

inline void print(const char* s, size_t len) noexcept {
  auto& output_instance_st{OutputInstanceState::get()};
  output_instance_st.output_buffers.current_buffer().get().append(s, len);
}

inline void print(const char* s) noexcept {
  print(s, strlen(s));
}

inline void print(const string_buffer& sb) noexcept {
  print(sb.buffer(), sb.size());
}

inline void print(const string& s) noexcept {
  print(s.c_str(), s.size());
}

inline int64_t f$print(const string& s) noexcept {
  print(s);
  return 1;
}

// === print_r ====================================================================================

string f$print_r(const mixed& v, bool buffered = false) noexcept;

template<class T>
string f$print_r(const class_instance<T>& v, bool buffered = false) noexcept {
  kphp::log::warning("print_r used on object");
  return f$print_r(string{v.get_class(), static_cast<string::size_type>(strlen(v.get_class()))}, buffered);
}

// === var_export =================================================================================

string f$var_export(const mixed& v, bool buffered = false) noexcept;

template<class T>
string f$var_export(const class_instance<T>& v, bool buffered = false) noexcept {
  kphp::log::warning("print_r used on object");
  return f$var_export(string{v.get_class(), static_cast<string::size_type>(strlen(v.get_class()))}, buffered);
}

// === var_dump ===================================================================================

void f$var_dump(const mixed& v) noexcept;

template<class T>
void f$var_dump(const class_instance<T>& v) noexcept {
  kphp::log::warning("print_r used on object");
  return f$var_dump(string{v.get_class(), static_cast<string::size_type>(strlen(v.get_class()))});
}

// ================================================================================================

inline void f$echo(const string& s) noexcept {
  print(s);
}

inline int64_t f$printf(const string& format, const array<mixed>& a) noexcept {
  const string to_print{f$sprintf(format, a)};
  print(to_print);
  return to_print.size();
}

inline kphp::coro::task<Optional<int64_t>> f$vfprintf(const resource& stream, const string& format, const array<mixed>& args) noexcept {
  co_return co_await f$fwrite(stream, f$vsprintf(format, args));
}

inline kphp::coro::task<Optional<int64_t>> f$fprintf(const resource& stream, const string& format, const array<mixed>& args) noexcept {
  co_return co_await f$vfprintf(stream, format, args);
}

inline Optional<int64_t> f$fputcsv(const mixed& /*unused*/, const array<mixed>& /*unused*/, const string& /*unused*/ = string(",", 1),
                                   const string& /*unused*/ = string("\"", 1), const string& /*unused*/ = string("\\", 1)) {
  kphp::log::error("call to unsupported function");
}
