// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/on_kphp_warning_callback.h"

#include "common/fast-backtrace.h"

#include "runtime/kphp-backtrace.h"

OnKphpWarningCallback &OnKphpWarningCallback::get() {
  static OnKphpWarningCallback state;
  return state;
}

bool OnKphpWarningCallback::set_callback(on_kphp_warning_callback_type &&new_callback) {
  if (in_registered_callback) {
    return false;
  }
  callback = std::move(new_callback);
  return true;
}

void OnKphpWarningCallback::invoke_callback(const string &warning_message) {
  if (callback && !in_registered_callback) {
    // subdomains and test domains are compiled without debug symbols, so addr2line will not be available there;
    // in an effort to show at least some kind of trace during an error — collect and demangle manually
    // what we can get is a function names, but that's better than nothing
    std::array<void *, 64> buffer{};
    const int nptrs = fast_backtrace(buffer.data(), buffer.size());
    array<string> arg_stacktrace;
    if (nptrs > 2) {
      // start with the 2nd frame: 0 is invoke_callback(), 1 is php_warning(), 2 is what we want
      arg_stacktrace.reserve(nptrs - 2, true);
      KphpBacktrace demangler{buffer.data() + 2, nptrs - 2};
      for (const char *name : demangler.make_demangled_backtrace_range()) {
        if (name) {
          arg_stacktrace.emplace_back(string{name});
        }
      }
    }

    in_registered_callback = true;
    callback(warning_message, arg_stacktrace);
    in_registered_callback = false;
  }
}

void OnKphpWarningCallback::reset() {
  callback = {};
  in_registered_callback = false;
}

void register_kphp_on_warning_callback_impl(on_kphp_warning_callback_type &&callback) {
  bool set_successfully = OnKphpWarningCallback::get().set_callback(std::move(callback));
  if (!set_successfully) {
    php_warning("It's forbidden to register new on warning callback inside registered one.");
  }
}
