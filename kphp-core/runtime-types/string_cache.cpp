// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "kphp-core/kphp_core.h"

constexpr auto string_cache::constexpr_make_large_ints() noexcept {
  return constexpr_make_ints(std::make_index_sequence<cached_int_max() / 10>{});
}

const string::string_inner &string_cache::cached_large_int(int64_t i) noexcept {
  static_assert(sizeof(string_8bytes) == sizeof(string::string_inner) + string_8bytes::TAIL_SIZE, "unexpected padding");
  // to avoid a big compilation time impact, we implement numbers generation from 100 (small_int_max) to 9999 (cached_int_max - 1) here
  static constexpr auto large_int_cache = constexpr_make_large_ints();
  php_assert(i < large_int_cache.size());
  return large_int_cache[i].inner;
}
