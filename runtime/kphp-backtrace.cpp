// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/kphp-backtrace.h"

#include <cxxabi.h>
#include <execinfo.h>

#include "common/fast-backtrace.h"
#include "common/wrappers/string_view.h"

#include "runtime/critical_section.h"

std::forward_list<char **> KphpBacktrace::last_used_symbols_;

KphpBacktrace::KphpBacktrace(void *const *raw_backtrace, int32_t size) noexcept {
  dl::CriticalSectionGuard signal_critical_section;
  if ((symbols_begin_ = backtrace_symbols(raw_backtrace, size))) {
    symbols_end_ = symbols_begin_ + size;
    last_used_symbols_.emplace_front(symbols_begin_);
  }
}

void KphpBacktrace::clear() noexcept {
  dl::CriticalSectionGuard signal_critical_section;
  last_used_symbols_.remove(symbols_begin_);
  free(symbols_begin_);
}

const char *KphpBacktrace::make_line(const char *symbol, bool full_trace) noexcept {
  const char *mangled_name = nullptr;
  const char *offset_begin = nullptr;
  const char *offset_end = nullptr;
#ifdef __APPLE__
  const char *begin = strchr(symbol, '_');
  if (begin == nullptr) {
    return nullptr;
  }
  const char *end = strchr(begin, '+');
  if (end == nullptr) {
    return nullptr;
  }

  const size_t copy_name_len = end - begin;
  php_assert(name_buffer_.size() >= copy_name_len);
  memcpy(name_buffer_.data(), begin, end - begin);
  name_buffer_[copy_name_len - 1] = 0;

  full_trace = false;
#else
  for (const char *p = symbol; *p; ++p) {
    if (*p == '(') {
      mangled_name = p;
    } else if (*p == '+' && mangled_name != nullptr) {
      offset_begin = p;
    } else if (*p == ')' && offset_begin != nullptr) {
      offset_end = p;
      break;
    }
  }
  if (offset_end == nullptr) {
    return nullptr;
  }

  const size_t copy_name_len = offset_begin - mangled_name;
  php_assert(name_buffer_.size() >= copy_name_len);
  memcpy(name_buffer_.data(), mangled_name + 1, copy_name_len - 1);
  name_buffer_[copy_name_len - 1] = 0;
#endif

  dl::CriticalSectionGuard critical_section;
  int status = 0;
  char *real_name = abi::__cxa_demangle(name_buffer_.data(), nullptr, nullptr, &status);
  if (status == 0) {
    const size_t name_len = std::min(strlen(real_name), name_buffer_.size() - 1);
    memcpy(name_buffer_.data(), real_name, name_len);
    name_buffer_[name_len] = '\0';
    free(real_name);
  }
  if (full_trace) {
    snprintf(trace_buffer_.data(), trace_buffer_.size(), "%.*s : %s+%.*s%s\n", static_cast<int32_t>(mangled_name - symbol), symbol, name_buffer_.data(),
             static_cast<int32_t>(offset_end - offset_begin - 1), offset_begin + 1, offset_end + 1);
    return trace_buffer_.data();
  }
  return name_buffer_.data();
}

void parse_kphp_backtrace(char *buffer, size_t buffer_len, void *const *raw_backtrace, int backtrace_len) {
  if (buffer_len == 0) {
    return;
  }
  buffer[0] = '\0';
  const char *sep = ";\n";
  KphpBacktrace demangler{raw_backtrace, backtrace_len};
  size_t cur_len = 0;
  for (const char *name : demangler.make_demangled_backtrace_range()) {
    const size_t len = name ? strlen(name) : 0;
    if (len == 0) {
      continue;
    }
    if (cur_len + len + std::strlen(sep) + 1 > buffer_len) {
      break;
    }
    std::strcat(buffer, name);
    std::strcat(buffer, sep);
    cur_len += len + std::strlen(sep);
  }
}

void free_kphp_backtrace() noexcept {
  if (KphpBacktrace::last_used_symbols_.empty()) {
    return;
  }
  dl::CriticalSectionGuard critical_section;
  while (!KphpBacktrace::last_used_symbols_.empty()) {
    free(KphpBacktrace::last_used_symbols_.front());
    KphpBacktrace::last_used_symbols_.pop_front();
  }
}

array<string> f$kphp_backtrace(bool pretty) noexcept {
  std::array<void *, 128> buffer{};
  const int32_t nptrs = fast_backtrace(buffer.data(), buffer.size());
  array<string> backtrace{array_size{nptrs, true}};
  KphpBacktrace demangler{buffer.data(), nptrs};
  for (const char *name : demangler.make_demangled_backtrace_range()) {
    const size_t len = name ? strlen(name) : 0;
    if (!len) {
      continue;
    }
    vk::string_view func_name{name, len};
    // depending on the compilation options the fast_backtrace function and f$kphp_backtrace
    // may be present or absent inside a backtrace
    if (func_name.starts_with(__FUNCTION__) && vk::string_view{__PRETTY_FUNCTION__}.ends_with(func_name)) {
      backtrace.clear();
      continue;
    }
    if (!pretty) {
      backtrace.emplace_back(string{func_name.data(), static_cast<string::size_type>(func_name.size())});
      continue;
    }
    if (!func_name.starts_with("f$")) {
      continue;
    }
    func_name.remove_prefix(2);
    // skip the run() function which calls the main file
    if (func_name.ends_with("$run()")) {
      continue;
    }
    string pretty_name{static_cast<string::size_type>(func_name.size()), true};
    for (const auto *it = func_name.begin(); it != func_name.end();) {
      const auto *next = std::next(it);
      if (*it == '$') {
        if (next != func_name.end() && *next == '$') {
          pretty_name.append_unsafe("::", 2);
          ++next;
        } else {
          pretty_name.append_unsafe("\\", 1);
        }
      } else if (*it == 'C' && next != func_name.end() && *next == '$') {
        ++next;
      } else {
        pretty_name.append_unsafe(it, 1);
      }
      it = next;
    }
    pretty_name.finish_append();
    backtrace.emplace_back(std::move(pretty_name));
  }
  return backtrace;
}
