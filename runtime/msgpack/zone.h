// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdlib>

#include "common/mixin/not_copyable.h"

namespace msgpack {

class zone : private vk::not_copyable {
public:
  explicit zone(size_t chunk_size = 8192);
  void *allocate_align(size_t size, size_t align);

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

  char *allocate_expand(size_t size);

  size_t m_chunk_size{0};
  chunk_list m_chunk_list;
};

} // namespace msgpack
