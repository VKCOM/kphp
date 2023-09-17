// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/allocator.h"
#include "runtime/kphp_core.h"

class string_list : vk::not_copyable {
public:
  bool push_string(const char *s, size_t len) noexcept {
    auto *mem = dl::script_allocator_malloc(sizeof(list_node) + len);
    if (unlikely(!mem)) {
      return false;
    }
    const size_t new_len = len_ + len;
    if (unlikely(new_len > string::max_size())) {
      return false;
    }

    len_ = new_len;
    auto *node = new(mem) list_node{nullptr, len};
    std::memcpy(node + 1, s, len);
    if (tail_) {
      tail_->next = node;
      tail_ = node;
    } else {
      php_assert(!head_);
      head_ = node;
      tail_ = head_;
    }
    return true;
  }

  const string &concat_and_get_string() noexcept {
    if (str_.empty() && len_) {
      str_.reserve_at_least(static_cast<string::size_type>(len_));
      flush_list([this](const char *s, size_t size) { str_.append_unsafe(s, size); });
      str_.finish_append();
    }
    return str_;
  }

  void reset() noexcept {
    flush_list([](const char *, size_t) {});
    str_ = string{};
  }

  size_t size() const noexcept {
    return len_;
  }

  ~string_list() {
    reset();
  }

private:
  template<class F>
  void flush_list(F cb) noexcept {
    for (list_node *node = head_; node;) {
      cb(reinterpret_cast<const char *>(node + 1), node->size);
      list_node *next = node->next;
      node->~list_node();
      dl::script_allocator_free(node);
      node = next;
    }
    head_ = nullptr;
    tail_ = nullptr;
    len_ = 0;
  }


  struct alignas(8) list_node {
    list_node *next;
    size_t size;
  };

  list_node *head_{nullptr};
  list_node *tail_{nullptr};
  size_t len_{0};

  string str_;
};
