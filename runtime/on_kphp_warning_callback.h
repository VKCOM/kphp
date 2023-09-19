// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <functional>

#include "runtime/critical_section.h"
#include "runtime/kphp_core.h"

using on_kphp_warning_callback_type = std::function<void(const string &, const array<string> &)>;

class OnKphpWarningCallback {
public:
  static OnKphpWarningCallback &get();
  bool set_callback(on_kphp_warning_callback_type &&new_callback);
  void invoke_callback(const string &warning_message);
  void reset();
private:
  bool in_registered_callback{false};
  on_kphp_warning_callback_type callback;
  OnKphpWarningCallback() = default;
};

void register_kphp_on_warning_callback_impl(on_kphp_warning_callback_type &&callback);

template <typename F>
void f$register_kphp_on_warning_callback(F &&callback) {
  // std::function sometimes uses heap, when constructed from captured lambda. So it must be constructed under critical section only.
  dl::CriticalSectionGuard heap_guard;
  register_kphp_on_warning_callback_impl(on_kphp_warning_callback_type{std::forward<F>(callback)});
}
