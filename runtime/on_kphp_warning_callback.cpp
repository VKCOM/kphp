#include "runtime/on_kphp_warning_callback.h"

#include "common/fast-backtrace.h"

#include "runtime/kphp_backtrace.h"


OnKphpWarningCallback &OnKphpWarningCallback::get() {
  static OnKphpWarningCallback state;
  return state;
}

bool OnKphpWarningCallback::set_callback(on_kphp_warning_callback_type new_callback) {
  if (in_registered_callback) {
    return false;
  }
  callback = std::move(new_callback);
  return true;
}

void OnKphpWarningCallback::invoke_callback(const string &warning_message) {
  if (callback && !in_registered_callback) {
    // поддомены и тестовые домены собираются без debug-символов, поэтому там addr2line не действует
    // чтобы хоть какой-то трейс отобразить разработчикам при ошибке — собираем и деманглим вручную
    // получаются просто названия функций, но обычно это уже неплохо
    void *buffer[64];
    int nptrs = fast_backtrace(buffer, sizeof(buffer) / sizeof(buffer[0]));
    // start_i=2: 0 это invoke_callback(), 1 это php_warning(), а нужная инфа с 2
    array<string> arg_stacktrace;
    get_demangled_backtrace(buffer, nptrs, 0, [&arg_stacktrace](const char *function_name, const char *) {
      arg_stacktrace.push_back(string(function_name));
    }, 2);

    in_registered_callback = true;
    callback(warning_message, arg_stacktrace);
    in_registered_callback = false;
  }
}

void OnKphpWarningCallback::reset() {
  callback = {};
  in_registered_callback = false;
}

void f$register_kphp_on_warning_callback(const on_kphp_warning_callback_type &callback) {
  bool set_successfully = OnKphpWarningCallback::get().set_callback(callback);
  if (!set_successfully) {
    php_warning("It's forbidden to register new on warning callback inside registered one.");
  }
}
