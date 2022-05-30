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

void zone::finalizer_array::call() {
  finalizer *fin = m_tail;
  for (; fin != m_array; --fin)
    (*(fin - 1))();
}

zone::finalizer_array::~finalizer_array() {
  call();
  ::free(m_array);
}

void zone::finalizer_array::clear() {
  call();
  m_tail = m_array;
}

void zone::finalizer_array::push(void (*func)(void *data), void *data) {
  finalizer *fin = m_tail;

  if (fin == m_end) {
    push_expand(func, data);
    return;
  }

  fin->m_func = func;
  fin->m_data = data;

  ++m_tail;
}

void zone::finalizer_array::push_expand(void (*func)(void *), void *data) {
  const size_t nused = m_end - m_array;
  size_t nnext;
  if (nused == 0) {
    nnext = (sizeof(finalizer) < 72 / 2) ? 72 / sizeof(finalizer) : 8;
  } else {
    nnext = nused * 2;
  }
  finalizer *tmp = static_cast<finalizer *>(::realloc(m_array, sizeof(finalizer) * nnext));
  if (!tmp) {
    throw std::bad_alloc();
  }
  m_array = tmp;
  m_end = tmp + nnext;
  m_tail = tmp + nused;
  new (m_tail) finalizer(func, data);

  ++m_tail;
}

zone::finalizer_array::finalizer_array(finalizer_array &&other) noexcept
  : m_tail(other.m_tail)
  , m_end(other.m_end)
  , m_array(other.m_array) {
  other.m_tail = nullptr;
  other.m_end = nullptr;
  other.m_array = nullptr;
}

zone::finalizer_array &zone::finalizer_array::operator=(finalizer_array &&other) noexcept {
  this->~finalizer_array();
  new (this) finalizer_array(std::move(other));
  return *this;
}

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

void zone::chunk_list::clear(size_t chunk_size) {
  chunk *c = m_head;
  while (true) {
    chunk *n = c->m_next;
    if (n) {
      ::free(c);
      c = n;
    } else {
      m_head = c;
      break;
    }
  }
  m_head->m_next = nullptr;
  m_free = chunk_size;
  m_ptr = reinterpret_cast<char *>(m_head) + sizeof(chunk);
}

zone::chunk_list::chunk_list(chunk_list &&other) noexcept
  : m_free(other.m_free)
  , m_ptr(other.m_ptr)
  , m_head(other.m_head) {
  other.m_head = nullptr;
}

zone::chunk_list &zone::chunk_list::operator=(chunk_list &&other) noexcept {
  this->~chunk_list();
  new (this) chunk_list(std::move(other));
  return *this;
}

zone::zone(size_t chunk_size) noexcept
  : m_chunk_size(chunk_size)
  , m_chunk_list(m_chunk_size) {}

inline char *zone::get_aligned(char *ptr, size_t align) {
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

inline void *zone::allocate_no_align(size_t size) {
  char *ptr = m_chunk_list.m_ptr;
  if (m_chunk_list.m_free < size) {
    ptr = allocate_expand(size);
  }
  m_chunk_list.m_free -= size;
  m_chunk_list.m_ptr += size;

  return ptr;
}

inline char *zone::allocate_expand(size_t size) {
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

inline void zone::push_finalizer(void (*func)(void *), void *data) {
  m_finalizer_array.push(func, data);
}

template<typename T>
inline void zone::push_finalizer(std::unique_ptr<T> obj) {
  m_finalizer_array.push(&zone::object_delete<T>, obj.release());
}

inline void zone::clear() {
  m_finalizer_array.clear();
  m_chunk_list.clear(m_chunk_size);
}

inline void zone::swap(zone &o) {
  std::swap(*this, o);
}

template<typename T>
void zone::object_delete(void *obj) {
  delete static_cast<T *>(obj);
}

template<typename T>
void zone::object_destruct(void *obj) {
  static_cast<T *>(obj)->~T();
}

inline void zone::undo_allocate(size_t size) {
  m_chunk_list.m_ptr -= size;
  m_chunk_list.m_free += size;
}

template<typename T, typename... Args>
T *zone::allocate(Args... args) {
  void *x = allocate_align(sizeof(T), alignof(T));
  try {
    m_finalizer_array.push(&zone::object_destruct<T>, x);
  } catch (...) {
    undo_allocate(sizeof(T));
    throw;
  }
  try {
    return new (x) T(args...);
  } catch (...) {
    --m_finalizer_array.m_tail;
    undo_allocate(sizeof(T));
    throw;
  }
}

} // namespace msgpack
