// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cassert>
#include <functional>

#include "common/mixin/not_copyable.h"

struct tl_in_methods : vk::not_copyable {
  virtual void fetch_raw_data(void *buf, int len) noexcept = 0;
  virtual void fetch_chunked(int len, const std::function<void(const void *, int)> &callback) noexcept = 0;
  virtual void fetch_lookup_chunked(int len, const std::function<void(const void *, int)> &callback) noexcept = 0;
  virtual void fetch_move(int len) noexcept = 0;
  virtual void fetch_lookup(void *buf, int len) noexcept = 0;
  virtual void fetch_mark() noexcept = 0;
  virtual void fetch_mark_restore() noexcept = 0;
  virtual void fetch_mark_delete() noexcept = 0;
  virtual int decompress(int version) noexcept = 0;
  virtual ~tl_in_methods() = default;
};

struct tl_out_methods : vk::not_copyable {
  virtual void *store_get_ptr(int len) noexcept = 0;
  virtual void store_raw_data(const void *buf, int len) noexcept = 0;
  virtual void copy_through(tl_in_methods *in, int len, int advance) noexcept {
    auto store = [this](const void *data, int len) { store_raw_data(data, len); };
    if (advance) {
      in->fetch_chunked(len, store);
    } else {
      in->fetch_lookup_chunked(len, store);
    }
  }
  virtual ~tl_out_methods() = default;
};
