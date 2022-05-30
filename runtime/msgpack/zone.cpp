//
// MessagePack for C++ memory pool
//
// Copyright (C) 2008-2013 FURUHASHI Sadayuki and KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//

#include "runtime/msgpack/zone.h"

#include <cstdlib>
#include <memory>

namespace msgpack {

zone::chunk_list::chunk_list(size_t chunk_size) {
  chunk *c = static_cast<chunk *>(::malloc(sizeof(chunk) + chunk_size));
  if (!c) {
    throw std::bad_alloc();
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
    ::free(c);
    c = n;
  }
}

zone::zone(size_t chunk_size) noexcept
  : m_chunk_size(chunk_size)
  , m_chunk_list(m_chunk_size) {}

char *zone::get_aligned(char *ptr, size_t align) {
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

  chunk *c = static_cast<chunk *>(::malloc(sizeof(chunk) + sz));
  if (!c)
    throw std::bad_alloc();

  char *ptr = reinterpret_cast<char *>(c) + sizeof(chunk);

  c->m_next = cl->m_head;
  cl->m_head = c;
  cl->m_free = sz;
  cl->m_ptr = ptr;

  return ptr;
}

void *zone::operator new(std::size_t size) {
  void *p = ::malloc(size);
  if (!p) {
    throw std::bad_alloc();
  }
  return p;
}

void zone::operator delete(void *p) noexcept {
  ::free(p);
}

} // namespace msgpack
