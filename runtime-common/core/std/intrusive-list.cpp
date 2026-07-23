#include "intrusive-list.h"

namespace kphp::stl::intrusive_list::details {

auto hook_base::move_hook(hook_base&& other) noexcept -> void {
  assert(!is_linked());
  if (other.is_linked()) {
    m_prev = std::exchange(other.m_prev, std::addressof(other));
    m_next = std::exchange(other.m_next, std::addressof(other));

    m_prev->m_next = this;
    m_next->m_prev = this;
  }
}

hook_base::hook_base(const hook_base& /*unused*/) noexcept
    : hook_base() {}

hook_base::hook_base(hook_base&& other) noexcept {
  move_hook(std::move(other));
}

auto hook_base::operator=(const hook_base& /*unused*/) noexcept -> hook_base& {
  return *this;
}

auto hook_base::operator=(hook_base&& other) noexcept -> hook_base& {
  if (this == std::addressof(other)) {
    return *this;
  }

  unlink();
  move_hook(std::move(other));

  return *this;
}

hook_base::~hook_base() {
  unlink();
}

auto hook_base::is_linked() const noexcept -> bool {
  return m_prev != this;
}

auto hook_base::unlink() noexcept -> void {
  m_prev->m_next = m_next;
  m_next->m_prev = m_prev;

  m_prev = this;
  m_next = this;
}

} // namespace kphp::stl::intrusive_list::details
