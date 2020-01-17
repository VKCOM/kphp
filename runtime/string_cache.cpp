#include "common/algorithms/simd-int-to-string.h"

#include "runtime/kphp_core.h"

std::array<string_cache::string_8bytes, 256> string_cache::make_chars() noexcept {
  std::array<string_8bytes, 256> cached_chars;
  for (size_t c = 0; c < cached_chars.size(); ++c) {
    cached_chars[c].inner.size = 1;
    cached_chars[c].inner.capacity = 1;
    cached_chars[c].data[0] = static_cast<char>(c);
  }
  return cached_chars;
}

std::array<string_cache::string_8bytes, 10000> string_cache::make_ints() noexcept {
  std::array<string_8bytes, 10000> cached_ints;
  for (size_t c = 0; c < cached_ints.size(); ++c) {
    char *end = simd_int32_to_string(static_cast<int>(c), cached_ints[c].data);
    *end = '\0';
    cached_ints[c].inner.size = static_cast<dl::size_type>(end - cached_ints[c].data);
    cached_ints[c].inner.capacity = cached_ints[c].inner.size;
  }
  return cached_ints;
}

void global_init_string_cache() noexcept {
  string_cache::empty_string();
  string_cache::cached_char(0);
  string_cache::cached_ints();
}
