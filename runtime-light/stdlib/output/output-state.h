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

  string_buffer m_system_buffer;
  std::array<std::optional<string_buffer>, MAX_BUFFERS> m_user_buffers{};
  decltype(m_user_buffers)::iterator m_user_buffer_iterator{m_user_buffers.end()};

public:
  output_buffers() noexcept = default;

  auto user_level() const noexcept {
    return m_user_buffer_iterator != m_user_buffers.end()
               ? std::distance(m_user_buffers.cbegin(), decltype(m_user_buffers)::const_iterator{m_user_buffer_iterator}) + 1
               : 0;
  }

  auto user_buffers() const noexcept {
    const auto* const end_iterator{user_level() != 0 ? std::next(decltype(m_user_buffers)::const_iterator{m_user_buffer_iterator}) : m_user_buffers.cbegin()};
    return std::views::transform(std::span{m_user_buffers.cbegin(), end_iterator},
                                 [](const auto& opt_buffer) noexcept -> decltype(auto) { return *opt_buffer; });
  }

  std::reference_wrapper<string_buffer> system_buffer() noexcept {
    return m_system_buffer;
  }

  std::optional<std::reference_wrapper<string_buffer>> user_buffer() noexcept {
    if (user_level() == 0) [[unlikely]] {
      return {};
    }
    return **m_user_buffer_iterator;
  }

  std::reference_wrapper<string_buffer> current_buffer() noexcept {
    const auto opt_user_buffer{user_buffer()};
    return opt_user_buffer.has_value() ? *opt_user_buffer : system_buffer();
  }

  std::optional<std::reference_wrapper<string_buffer>> next_user_buffer() noexcept {
    if (user_level() == MAX_BUFFERS) [[unlikely]] {
      return {};
    }
    m_user_buffer_iterator = user_level() != 0 ? std::next(m_user_buffer_iterator) : m_user_buffers.begin();
    return m_user_buffer_iterator->has_value() ? (**m_user_buffer_iterator).clean() : m_user_buffer_iterator->emplace();
  }

  std::optional<std::reference_wrapper<string_buffer>> prev_user_buffer() noexcept {
    if (user_level() == 0 || m_user_buffer_iterator == m_user_buffers.begin()) [[unlikely]] {
      m_user_buffer_iterator = m_user_buffers.end();
      return {};
    }
    m_user_buffer_iterator = std::prev(m_user_buffer_iterator);
    return **m_user_buffer_iterator;
  }
};

} // namespace kphp::output

struct OutputInstanceState : private vk::not_copyable {
  kphp::output::output_buffers output_buffers;

  OutputInstanceState() noexcept = default;

  static OutputInstanceState& get() noexcept;
};
