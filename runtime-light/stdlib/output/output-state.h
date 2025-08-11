// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <span>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/core-types/decl/declarations.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::output {

class output_buffers {
  static constexpr size_t MAX_USER_BUFFERS = 50;
  static constexpr size_t NO_USER_BUFFERS_IDX = MAX_USER_BUFFERS + 1;

  std::optional<string_buffer> m_system_buffer;

  size_t m_inited_user_buffers{};
  size_t m_user_buffer_idx{NO_USER_BUFFERS_IDX};
  kphp::stl::vector<string_buffer, kphp::memory::script_allocator> m_user_buffers;

public:
  output_buffers() noexcept {
    m_user_buffers.reserve(MAX_USER_BUFFERS);
  }

  size_t user_level() const noexcept {
    return m_user_buffer_idx == NO_USER_BUFFERS_IDX ? 0 : m_user_buffer_idx + 1;
  }

  auto user_buffers() const noexcept {
    return std::span<const string_buffer>{m_user_buffers.cbegin(), user_level()};
  }

  std::reference_wrapper<string_buffer> system_buffer() noexcept {
    return m_system_buffer.has_value() ? *m_system_buffer : m_system_buffer.emplace();
  }

  std::optional<std::reference_wrapper<string_buffer>> user_buffer() noexcept {
    if (user_level() == 0) [[unlikely]] {
      return {};
    }
    kphp::log::assertion(m_user_buffer_idx < m_user_buffers.size());
    return m_user_buffers[m_user_buffer_idx];
  }

  std::reference_wrapper<string_buffer> current_buffer() noexcept {
    const auto opt_user_buffer{user_buffer()};
    return opt_user_buffer.has_value() ? *opt_user_buffer : system_buffer();
  }

  std::optional<std::reference_wrapper<string_buffer>> next_user_buffer() noexcept {
    if (user_level() == MAX_USER_BUFFERS) [[unlikely]] {
      return {};
    }

    m_user_buffer_idx = user_level() == 0 ? 0 : m_user_buffer_idx + 1;
    if (user_level() > m_inited_user_buffers) {
      m_user_buffers.emplace_back();
      ++m_inited_user_buffers;
    }
    kphp::log::assertion(m_user_buffer_idx < m_user_buffers.size());
    return m_user_buffers[m_user_buffer_idx].clean();
  }

  std::optional<std::reference_wrapper<string_buffer>> prev_user_buffer() noexcept {
    if (user_level() <= 1) [[unlikely]] {
      m_user_buffer_idx = NO_USER_BUFFERS_IDX;
      return {};
    }
    --m_user_buffer_idx;
    kphp::log::assertion(m_user_buffer_idx < m_user_buffers.size());
    return m_user_buffers[m_user_buffer_idx];
  }
};

} // namespace kphp::output

struct OutputInstanceState : private vk::not_copyable {
  kphp::output::output_buffers output_buffers;

  OutputInstanceState() noexcept = default;

  static OutputInstanceState& get() noexcept;
};
