// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

#include "runtime-core/runtime-core.h"
#include "runtime-core/utils/small-object-storage.h"

class fork_result {
  small_object_storage<sizeof(mixed)> storage{};

public:
  template<typename T>
  requires(!std::same_as<T, fork_result>) explicit fork_result(T &&t) noexcept {
    storage.emplace<std::remove_cvref_t<T>>(std::forward<T>(t));
  }

  template<typename T>
  T get() const noexcept {
    return *storage.get<T>();
  }
};
