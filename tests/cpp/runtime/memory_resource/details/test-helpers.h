#pragma once

#include "common/wrappers/to_array.h"

#include "runtime-common/runtime-core/memory-resource/details/memory_chunk_list.h"
#include "runtime-common/runtime-core/memory-resource/memory_resource.h"

template<size_t N>
inline auto make_offsets(const std::array<size_t, N> &sizes) {
  std::array<size_t, N + 1> offsets;
  offsets[0] = 0;
  for (size_t i = 0; i < N; ++i) {
    offsets[i + 1] = offsets[i] + sizes[i];
  }
  return offsets;
}

inline auto prepare_test_sizes() {
  auto sizes = vk::to_array<size_t>(
    {
      100UL, 120UL, 50UL, 78UL, 100UL, 101UL, 112UL, 222UL, 78UL, 111UL, 112UL, 114UL, 80UL, 79UL, 70UL, 79UL, 55UL, 70UL,
      130UL, 129UL, 130UL, 129UL, 155UL, 150UL, 140UL, 100UL, 88UL, 99UL, 77UL, 45UL, 46UL, 64UL, 87UL, 82UL, 147UL, 99UL,
      48UL, 88UL, 64UL, 100UL, 56UL, 198UL, 41UL, 42UL, 42UL, 42UL, 112UL, 164UL, 163UL, 162UL, 162UL, 163UL, 164UL, 200UL
    });
  for (auto &size: sizes) {
    size = memory_resource::details::align_for_chunk(size);
  }
  return sizes;
}
