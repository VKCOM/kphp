// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-core/runtime-core.h"

template<class Tag, class T, class U = Unknown>
void to_msgpack_impl(const class_instance<T> &klass, const array<U> &more) noexcept {

}

template<class Tag, class T, class U = Unknown>
string f$MsgPackEncoder$$to_msgpack_impl(Tag /*tag*/, const class_instance<T> &klass, int64_t flags = 0, const array<U> &more = {}) noexcept {
  return string();
}
