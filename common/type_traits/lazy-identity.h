// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

namespace vk {

template<typename T>
struct lazy_identity {
  using type = T;
};

template<typename T>
using lazy_identity_t = typename vk::lazy_identity<T>::type;

} // namespace vk
