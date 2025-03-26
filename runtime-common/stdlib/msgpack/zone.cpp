// Compiler for PHP (aka KPHP)
// msgpack (c) https://github.com/msgpack/msgpack-c/tree/cpp_master (copied as third-party and slightly modified)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/msgpack/zone.h"
#include "runtime-common/core/allocator/script-malloc-like-allocator.h"

#include <memory>

namespace vk::msgpack {

zone::chunk_list::chunk_list(size_t chunk_size) {
  auto *c = static_cast<chunk *>(kphp::memory::script::alloc(sizeof(chunk) + chunk_size));
  if (!c) {
    throw std::bad_alloc{};
  }

  m_head = c;
  m_free = chunk_size;
  m_ptr = reinterpret_cast<char *>(c) + sizeof(chunk);
  c->m_next = nullptr;
}

zone::chunk_list::~chunk_list() {
  chunk *c = m_head;
  while (c) {
    chunk *n = c->m_next;
    kphp::memory::script::free(c);
    c = n;
  }
}

zone::zone(size_t chunk_size)
  : m_chunk_size(chunk_size)
  , m_chunk_list(m_chunk_size) {}

static char *get_aligned(const char *ptr, size_t align) noexcept {
  return reinterpret_cast<char *>(reinterpret_cast<size_t>((ptr + (align - 1))) / align * align);
}

void *zone::allocate_align(size_t size, size_t align) {
  char *aligned = get_aligned(m_chunk_list.m_ptr, align);
  size_t adjusted_size = size + (aligned - m_chunk_list.m_ptr);
  if (m_chunk_list.m_free < adjusted_size) {
    size_t enough_size = size + align - 1;
    char *ptr = allocate_expand(enough_size);
    aligned = get_aligned(ptr, align);
    adjusted_size = size + (aligned - m_chunk_list.m_ptr);
  }
  m_chunk_list.m_free -= adjusted_size;
  m_chunk_list.m_ptr += adjusted_size;
  return aligned;
}

char *zone::allocate_expand(size_t size) {
  chunk_list *const cl = &m_chunk_list;

  size_t sz = m_chunk_size;

  while (sz < size) {
    size_t tmp_sz = sz * 2;
    if (tmp_sz <= sz) {
      sz = size;
      break;
    }
    sz = tmp_sz;
  }
  auto *c = static_cast<chunk *>(kphp::memory::script::alloc(sizeof(chunk) + sz));
  if (!c) {
    throw std::bad_alloc{};
  }

  char *ptr = reinterpret_cast<char *>(c) + sizeof(chunk);

  c->m_next = cl->m_head;
  cl->m_head = c;
  cl->m_free = sz;
  cl->m_ptr = ptr;

  return ptr;
}

} // namespace vk::msgpack
