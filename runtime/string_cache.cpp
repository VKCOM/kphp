#include "runtime/kphp_core.h"

constexpr auto string_cache::constexpr_make_large_ints() noexcept {
  return constexpr_make_ints(std::make_index_sequence<cached_int_max() / 10>{});
}

const string::string_inner &string_cache::cached_large_int(int i) noexcept {
  static_assert(sizeof(string_8bytes) == sizeof(string::string_inner) + string_8bytes::TAIL_SIZE, "unexpected padding");
  // что бы не напрягать сильно компилятор, генерация чисел от 100 (small_int_max) до 9999 (cached_int_max - 1) находится тут
  static constexpr auto large_int_cache = constexpr_make_large_ints();
  php_assert(i < large_int_cache.size());
  return large_int_cache[i].inner;
}
