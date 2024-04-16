// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/core/runtime_types/storage.h"

Storage::Storage() noexcept :
  tag(0) {
  memset(storage_.storage_, 0, sizeof(mixed));
}

void Storage::save_void() noexcept {
  //todo:k2 check CurException
  tag = tagger<void>::get_tag();
}

