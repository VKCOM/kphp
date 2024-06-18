// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/resumable.h"

Storage::Storage() noexcept :
  tag(0) {
  memset(storage_.storage_.data(), 0, sizeof(mixed));
}

void Storage::save_void() noexcept {
  if (!CurException.is_null()) {
    save_exception();
  } else {
    tag = tagger<void>::get_tag();
  }
}

void Storage::save_exception() noexcept {
  php_assert (!CurException.is_null());
  Throwable exception = std::move(CurException);
  save<thrown_exception>(thrown_exception{exception});
}
