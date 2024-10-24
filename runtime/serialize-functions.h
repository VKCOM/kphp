// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime/context/runtime-context.h"

namespace impl_ {

class PhpSerializer : vk::not_copyable {
public:
  static void serialize(bool b) noexcept;
  static void serialize(int64_t i) noexcept;
  static void serialize(double d) noexcept;
  static void serialize(const string &s) noexcept;
  static void serialize(const mixed &v) noexcept;

  template<class T>
  static void serialize(const array<T> &arr) noexcept;

  template<class T>
  static void serialize(const Optional<T> &opt) noexcept;

private:
  static void serialize_null() noexcept;
};

template<class T>
void PhpSerializer::serialize(const array<T> &arr) noexcept {
  kphp_runtime_context.static_SB.append("a:", 2);
  kphp_runtime_context.static_SB << arr.count();
  kphp_runtime_context.static_SB.append(":{", 2);
  for (auto p : arr) {
    auto key = p.get_key();
    if (array<T>::is_int_key(key)) {
      serialize(key.to_int());
    } else {
      serialize(key.to_string());
    }
    serialize(p.get_value());
  }
  kphp_runtime_context.static_SB << '}';
}

template<class T>
void PhpSerializer::serialize(const Optional<T> &opt) noexcept {
  switch (opt.value_state()) {
    case OptionalState::has_value:
      return serialize(opt.val());
    case OptionalState::false_value:
      return serialize(false);
    case OptionalState::null_value:
      return serialize_null();
  }
}

} // namespace impl_

template<class T>
string f$serialize(const T &v) noexcept {
  kphp_runtime_context.static_SB.clean();
  impl_::PhpSerializer::serialize(v);
  return kphp_runtime_context.static_SB.str();
}

mixed f$unserialize(const string &v) noexcept;

mixed unserialize_raw(const char *v, int32_t v_len) noexcept;
