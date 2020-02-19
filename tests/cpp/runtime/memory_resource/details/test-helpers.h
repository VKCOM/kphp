#pragma once

#include "common/wrappers/to_array.h"

#include "runtime/memory_resource/memory_resource.h"

template<size_t N>
inline auto make_offsets(const std::array<memory_resource::size_type, N> &sizes) {
  std::array<memory_resource::size_type, N + 1> offsets;
  offsets[0] = 0;
  for (size_t i = 0; i < N; ++i) {
    offsets[i + 1] = offsets[i] + sizes[i];
  }
  return offsets;
}

inline auto prepare_test_sizes() {
  return vk::to_array<memory_resource::size_type>(
    {
      100u, 120u, 50u, 78u, 100u, 101u, 112u, 222u, 78u, 111u, 112u, 114u, 80u, 79u, 70u, 79u, 55u, 70u,
      130u, 129u, 130u, 129u, 155u, 150u, 140u, 100u, 88u, 99u, 77u, 45u, 46u, 64u, 87u, 82u, 147u, 99u,
      48u, 88u, 64u, 100u, 56u, 198u, 41u, 40u, 40u, 40u, 112u, 164u, 163u, 162u, 162u, 163u, 164u, 200u
    });
}
