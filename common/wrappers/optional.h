// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"

#include <optional.hpp>

#pragma GCC diagnostic pop

namespace vk {
  using ::tl::optional;
  using ::tl::nullopt_t;
  using ::tl::nullopt;
} // namespace vk
