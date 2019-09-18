#pragma once
#include <functional>

#include "runtime/kphp_core.h"

using on_kphp_warning_callback_type = std::function<void(const string &, const array<string> &)>;

class OnKphpWarningCallback {
public:
  static OnKphpWarningCallback &get();
  bool set_callback(on_kphp_warning_callback_type new_callback);
  void invoke_callback(const string &warning_message);
  void reset();
private:
  bool in_registered_callback{false};
  on_kphp_warning_callback_type callback;
  OnKphpWarningCallback() = default;
};

void f$register_kphp_on_warning_callback(const on_kphp_warning_callback_type &callback);
