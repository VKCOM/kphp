// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <sys/cdefs.h>

#include "common/tl/methods/base.h"
#include "common/tl/store.h"

#include "net/net-msg.h"

void compress_rwm(raw_message_t &rwm, int version) noexcept;
void decompress_rwm(raw_message_t &rwm, int version) noexcept;

struct tl_in_methods_raw_msg final : tl_in_methods {
  raw_message in{};
  raw_message mark{};

  explicit tl_in_methods_raw_msg(raw_message_t *msg) {
    rwm_steal(&in, msg);
  }

  void fetch_raw_data(void *buf, int len) noexcept override {
    assert(rwm_fetch_data(&in, buf, len) == len);
  }
  void fetch_chunked(int len, const std::function<void(const void *, int)> &callback) noexcept override {
    assert(rwm_process_and_advance(&in, len, callback) == len);
  }
  void fetch_lookup_chunked(int len, const std::function<void(const void *, int)> &callback) noexcept override {
    assert(rwm_process(&in, len,
                       [&](const void *data, int len) {
                         callback(data, len);
                         return 0;
                       })
           == len);
  }

  void fetch_move(int len) noexcept override {
    assert(len >= 0);
    assert(rwm_fetch_data(&in, nullptr, len) == len);
  }

  void fetch_lookup(void *buf, int len) noexcept override {
    assert(rwm_fetch_lookup(&in, buf, len) == len);
  }
  void fetch_mark() noexcept override {
    assert(mark.magic == 0);
    rwm_clone(&mark, &in);
  }
  void fetch_mark_restore() noexcept override {
    assert(mark.magic == RM_INIT_MAGIC);
    rwm_free(&in);
    rwm_steal(&in, &mark);
    rwm_free(&mark);
  }
  void fetch_mark_delete() noexcept override {
    if (mark.magic == RM_INIT_MAGIC) {
      rwm_free(&mark);
    }
  }
  int decompress(int version) noexcept override {
    decompress_rwm(in, version);
    return in.total_bytes;
  }

  ~tl_in_methods_raw_msg() noexcept override {
    if (in.magic == RM_INIT_MAGIC) {
      rwm_free(&in);
    }
    if (mark.magic == RM_INIT_MAGIC) {
      rwm_free(&mark);
    }
  }
};

template<typename T, typename Base = tl_out_methods>
struct tl_out_methods_raw_msg_base : Base {

  raw_message *get_rwm() {
    return &static_cast<T *>(this)->rwm;
  }

  void *store_get_ptr(int len) noexcept final {
    return rwm_postpone_alloc(get_rwm(), len);
  }
  void store_raw_data(const void *buf, int len) noexcept final {
    assert(rwm_push_data(get_rwm(), buf, len) == len);
  }
  void copy_through(tl_in_methods *in, int len, int advance) noexcept final {
    if (auto *rwm_in = dynamic_cast<tl_in_methods_raw_msg *>(in)) {
      if (!advance) {
        raw_message_t r;
        rwm_clone(&r, &rwm_in->in);
        rwm_trunc(&r, len);
        rwm_union(get_rwm(), &r);
      } else {
        raw_message_t r;
        rwm_split_head(&r, &rwm_in->in, len);
        rwm_union(get_rwm(), &r);
      }
    } else {
      tl_out_methods::copy_through(in, len, advance);
    }
  }
};

void tl_fetch_init_raw_message(raw_message_t *msg);
