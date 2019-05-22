#include "runtime/resumable.h"

Storage::Storage() :
  getter_(nullptr) {
  memset(storage_, 0, sizeof(var));
}

var Storage::load_exception(char *storage) {
  php_assert (CurException.is_null());
  CurException = load_implementation_helper<Exception, Exception>::load(storage);
  return var();
}

var Storage::load_as_var() {
  if (getter_ == nullptr) {
    last_wait_error = "Result already was gotten";
    return false;
  }

  Getter getter = getter_;
  getter_ = nullptr;
  return getter(storage_);
}

void Storage::save_void() {
  if (!CurException.is_null()) {
    save_exception();
  } else {
    getter_ = load_implementation_helper<void, var>::load;
  }
}

void Storage::save_exception() {
  php_assert (!CurException.is_null());
  Exception exception = std::move(CurException);
  save<Exception>(exception, load_exception);
}
