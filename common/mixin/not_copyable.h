// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

namespace vk {

class not_copyable {
protected:
  not_copyable(const not_copyable&) = delete;
  not_copyable& operator=(const not_copyable&) = delete;
  not_copyable(not_copyable&&) = delete;
  not_copyable& operator=(not_copyable&&) = delete;

  not_copyable() = default;
  ~not_copyable() = default;
};

} // namespace vk
