#include "runtime/resumable.h"

Storage::Storage() :
  tag(0) {
  memset(storage_.storage_, 0, sizeof(var));
}

void Storage::save_void() {
  if (!CurException.is_null()) {
    save_exception();
  } else {
    tag = tagger<void>::get_tag();
  }
}

void Storage::save_exception() {
  php_assert (!CurException.is_null());
  Exception exception = std::move(CurException);
  save<thrown_exception>(thrown_exception{exception});
}
