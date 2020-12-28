// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"

namespace impl_ {

class PhpSerializer : vk::not_copyable {
public:
  static void serialize(bool b) noexcept;
  static void serialize(int64_t i) noexcept;
  static void serialize(double d) noexcept;
  static void serialize(const string &s) noexcept;
  static void serialize(const mixed &v) noexcept;
};

} // namespace impl_

template<class T>
string f$serialize(const T &v) noexcept {
  static_SB.clean();
  impl_::PhpSerializer::serialize(v);
  return static_SB.str();
}

mixed f$unserialize(const string &v) noexcept;

mixed unserialize_raw(const char *v, int32_t v_len) noexcept;
