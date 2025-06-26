// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <iterator>
#include <optional>
#include <ranges>
#include <span>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"

namespace kphp::output {

class output_buffers {
  static constexpr size_t MAX_BUFFERS = 50;

  std::array<std::optional<string_buffer>, MAX_BUFFERS> m_buffers{}; // there is a "system" buffer under begin() iterator
  decltype(m_buffers)::iterator m_buffer_iterator{m_buffers.begin()};

public:
  output_buffers() noexcept {
    m_buffer_iterator->emplace(); // "system" buffer init
  }

  std::reference_wrapper<string_buffer> current_buffer() const noexcept {
    return **m_buffer_iterator;
  }

  std::optional<std::reference_wrapper<string_buffer>> next_buffer() noexcept {
    if (m_buffer_iterator == std::prev(m_buffers.end())) [[unlikely]] {
      return {};
    }
    m_buffer_iterator = std::next(m_buffer_iterator);
    return m_buffer_iterator->has_value() ? (**m_buffer_iterator).clean() : m_buffer_iterator->emplace();
  }

  std::optional<std::reference_wrapper<string_buffer>> prev_buffer() noexcept {
    if (m_buffer_iterator == m_buffers.begin()) [[unlikely]] {
      return {};
    }
    m_buffer_iterator = std::prev(m_buffer_iterator);
    return **m_buffer_iterator;
  }

  auto level() const noexcept {
    return std::distance(m_buffers.cbegin(), decltype(m_buffers)::const_iterator{m_buffer_iterator});
  }

  auto buffers() const noexcept {
    return std::views::transform(std::span{m_buffers.cbegin(), std::next(decltype(m_buffers)::const_iterator{m_buffer_iterator})},
                                 [](const auto& opt_buffer) noexcept -> decltype(auto) { return *opt_buffer; });
  }
};

} // namespace kphp::output

struct OutputInstanceState : private vk::not_copyable {
  kphp::output::output_buffers output_buffers;

  OutputInstanceState() noexcept = default;

  static OutputInstanceState& get() noexcept;
};
