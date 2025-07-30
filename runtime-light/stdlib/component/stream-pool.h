// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <optional>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/streams/stream.h"
#include "runtime-light/utils/logs.h"

namespace kphp::component {

/**
 * @brief A pool of weighted streams for efficient resource management.
 *
 * The stream_pool maintains a collection of reusable streams sorted by their capacity,
 * allowing efficient allocation and deallocation of streams with specific requirements.
 *
 * Important: the stream_pool doesn't take care of stream status.
 */
class stream_pool {
  kphp::stl::map<size_t, kphp::component::stream, kphp::memory::script_allocator> m_streams;

public:
  stream_pool() noexcept = default;

  /**
   * @brief Acquires a stream from the pool with at least the specified capacity.
   *
   * @param capacity Minimum required capacity for the stream (default: 0 - any stream).
   * @return std::optional<kphp::component::stream>
   *         - Contains the stream if one with sufficient capacity was found
   *         - std::nullopt if no suitable stream is available.
   */
  auto take(size_t capacity = 0) noexcept -> std::optional<kphp::component::stream>;

  /**
   * @brief Returns a stream to the pool for future reuse.
   *
   * @param stream The stream to return to the pool.
   */
  auto yield(kphp::component::stream&& stream) noexcept -> void;
};

inline auto stream_pool::take(size_t capacity) noexcept -> std::optional<kphp::component::stream> {
  auto pos{m_streams.lower_bound(capacity)};
  if (pos == m_streams.end()) {
    return std::nullopt;
  }

  auto node_handle{m_streams.extract(pos)};
  kphp::log::assertion(!node_handle.empty());
  return std::move(node_handle.mapped());
}

inline auto stream_pool::yield(kphp::component::stream&& stream) noexcept -> void {
  // Source: https://en.cppreference.com/w/cpp/language/eval_order.html
  // Order of evaluation of any part of any expression, including order of evaluation of function arguments is unspecified (with some exceptions listed below).
  // The compiler can evaluate operands and other subexpressions in any order, and may choose another order when the same expression is evaluated again.
  const auto capacity{stream.capacity()};
  m_streams.emplace(capacity, std::move(stream));
}

} // namespace kphp::component
