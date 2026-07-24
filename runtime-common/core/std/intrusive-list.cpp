#include "intrusive-list.h"

#include <memory>
#include <utility>

namespace kphp::stl::intrusive::details {

constexpr auto list_node_base::insert_instead(list_node_base&& other) noexcept -> void {
  if (other.is_linked()) {
    m_prev = std::exchange(other.m_prev, std::addressof(other));
    m_next = std::exchange(other.m_next, std::addressof(other));

    m_prev->m_next = this;
    m_next->m_prev = this;
  }
}

constexpr list_node_base::list_node_base(list_node_base&& other) noexcept {
  insert_instead(std::move(other));
}

constexpr auto list_node_base::operator=(list_node_base&& other) noexcept -> list_node_base& {
  if (this != std::addressof(other)) {
    unlink();
    insert_instead(std::move(other));
  }

  return *this;
}

list_node_base::~list_node_base() {
  unlink();
}

constexpr auto list_node_base::is_linked() const noexcept -> bool {
  return m_prev != this;
}

constexpr auto list_node_base::unlink() noexcept -> void {
  m_prev->m_next = m_next;
  m_next->m_prev = m_prev;

  m_prev = this;
  m_next = this;
}

} // namespace kphp::stl::intrusive::details
