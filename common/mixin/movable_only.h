// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

namespace vk {

class movable_only {
protected:
  movable_only(const movable_only &) = delete;
  movable_only &operator=(const movable_only &) = delete;

  movable_only() = default;
  ~movable_only() = default;
  movable_only(movable_only &&) = default;
  movable_only &operator=(movable_only &&) = default;
};

} // namespace vk
