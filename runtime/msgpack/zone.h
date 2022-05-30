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
#include <memory>

namespace msgpack {

class zone {
private:
  struct finalizer {
    finalizer(void (*func)(void *), void *data)
      : m_func(func)
      , m_data(data) {}
    void operator()() {
      m_func(m_data);
    }
    void (*m_func)(void *){nullptr};
    void *m_data{nullptr};
  };
  struct finalizer_array {
    void call();
    finalizer_array() = default;
    ~finalizer_array();
    finalizer_array(finalizer_array &&other) noexcept;
    finalizer_array &operator=(finalizer_array &&other) noexcept;
    finalizer_array(const finalizer_array &) = delete;
    finalizer_array &operator=(const finalizer_array &) = delete;

    void clear();
    void push(void (*func)(void *data), void *data);
    void push_expand(void (*func)(void *), void *data);

    finalizer *m_tail{nullptr};
    finalizer *m_end{nullptr};
    finalizer *m_array{nullptr};
  };
  struct chunk {
    chunk *m_next{nullptr};
  };
  struct chunk_list {
    chunk_list(size_t chunk_size);
    ~chunk_list();
    chunk_list(chunk_list &&other) noexcept;
    chunk_list &operator=(chunk_list &&other) noexcept;
    chunk_list(const chunk_list &) = delete;
    chunk_list &operator=(const chunk_list &) = delete;

    void clear(size_t chunk_size);

    size_t m_free{0};
    char *m_ptr{nullptr};
    chunk *m_head{nullptr};
  };

  size_t m_chunk_size{0};
  chunk_list m_chunk_list;
  finalizer_array m_finalizer_array;

public:
  zone(size_t chunk_size = 8192) noexcept;

  void *allocate_align(size_t size, size_t align = sizeof(void *));
  void *allocate_no_align(size_t size);

  void push_finalizer(void (*func)(void *), void *data);

  template<typename T>
  void push_finalizer(std::unique_ptr<T> obj);

  void clear();

  void swap(zone &o);

  static void *operator new(std::size_t size) {
    void *p = ::malloc(size);
    if (!p)
      throw std::bad_alloc();
    return p;
  }
  static void operator delete(void *p) noexcept {
    ::free(p);
  }
  static void *operator new(std::size_t /*size*/, void *mem) noexcept {
    return mem;
  }
  static void operator delete(void * /*p*/, void * /*mem*/) noexcept {}

  template<typename T, typename... Args>
  T *allocate(Args... args);

  zone(zone &&) = default;
  zone &operator=(zone &&) = default;
  zone(const zone &) = delete;
  zone &operator=(const zone &) = delete;

private:
  void undo_allocate(size_t size);

  template<typename T>
  static void object_destruct(void *obj);

  template<typename T>
  static void object_delete(void *obj);

  static char *get_aligned(char *ptr, size_t align);

  char *allocate_expand(size_t size);
};

} // namespace msgpack
