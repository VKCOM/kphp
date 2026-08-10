// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace kphp::memory::detail {

struct control_block {
private:
  static constexpr auto SIZE_FIELD_BITSIZE{48};
  static constexpr auto BASE_OFFSET_FIELD_BITSIZE{16};
  static constexpr uint64_t BLOCK_SIZE_MASK{(1UL << SIZE_FIELD_BITSIZE) - 1};
  static constexpr uint64_t BASE_OFFSET_MASK{(1UL << BASE_OFFSET_FIELD_BITSIZE) - 1};

  static_assert(SIZE_FIELD_BITSIZE + BASE_OFFSET_FIELD_BITSIZE == std::numeric_limits<uint64_t>::digits);

public:
  static constexpr uint64_t max_size() noexcept {
    return 1UL << SIZE_FIELD_BITSIZE;
  }

  static constexpr uint64_t max_alignment() noexcept {
    return 1UL << BASE_OFFSET_FIELD_BITSIZE;
  }

  uint64_t raw() const noexcept {
    return (static_cast<uint64_t>(base_offset) << SIZE_FIELD_BITSIZE) | (static_cast<uint64_t>(size) & BLOCK_SIZE_MASK);
  }

  static control_block from_raw(uint64_t raw) noexcept {
    return control_block{.size = raw & BLOCK_SIZE_MASK, .base_offset = static_cast<uint16_t>((raw >> SIZE_FIELD_BITSIZE) & BASE_OFFSET_MASK)};
  }

  uint64_t size : SIZE_FIELD_BITSIZE;
  uint16_t base_offset : BASE_OFFSET_FIELD_BITSIZE;
};

inline bool is_power_of_2(uint64_t v) noexcept {
  return v && !(v & (v - 1));
}

static_assert(sizeof(control_block) == sizeof(uint64_t), "Control block's size must be equal to uint64");

} // namespace kphp::memory::detail
