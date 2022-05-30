//
// MessagePack for C++ memory pool
//
// Copyright (C) 2008-2013 FURUHASHI Sadayuki and KONDO Takatoshi
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <cstdlib>

#include "common/mixin/not_copyable.h"

namespace msgpack {

class zone : private vk::not_copyable {
private:
  struct chunk {
    chunk *m_next{nullptr};
  };
  struct chunk_list : private vk::not_copyable {
    explicit chunk_list(size_t chunk_size);
    ~chunk_list();

    size_t m_free{0};
    char *m_ptr{nullptr};
    chunk *m_head{nullptr};
  };

  size_t m_chunk_size{0};
  chunk_list m_chunk_list;

public:
  explicit zone(size_t chunk_size = 8192) noexcept;

  void *allocate_align(size_t size, size_t align);

  static void *operator new(std::size_t size);
  static void operator delete(void *p) noexcept;
  static void *operator new(std::size_t /*size*/, void *mem) noexcept { return mem; }
  static void operator delete(void * /*p*/, void * /*mem*/) noexcept {}

private:
  static char *get_aligned(char *ptr, size_t align);
  char *allocate_expand(size_t size);
};

} // namespace msgpack
