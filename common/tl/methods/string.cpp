// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/tl/methods/string.h"

#include <cstring>
#include <memory>

#include "common/tl/methods/base.h"
#include "common/tl/parse.h"

struct tl_in_methods_str final : tl_in_methods {
  const char *in = nullptr;
  const char *mark = nullptr;

  explicit tl_in_methods_str(const char *s)
    : in(s) {}

  void fetch_raw_data(void *buf, int len) noexcept override {
    memcpy(buf, in, len);
    in += len;
  }

  void fetch_chunked(int len, const std::function<void(const void *, int)> &callback) noexcept override {
    callback(in, len);
    in += len;
  }
  void fetch_lookup_chunked(int len, const std::function<void(const void *, int)> &callback) noexcept override {
    callback(in, len);
  }
  void fetch_move(int len) noexcept override {
    in += len;
  }
  void fetch_lookup(void *buf, int len) noexcept override {
    memcpy(buf, in, len);
  }
  void fetch_mark() noexcept override {
    assert(!mark);
    mark = in;
  }
  void fetch_mark_restore() noexcept override {
    assert(mark);
    in = mark;
    mark = nullptr;
  }
  void fetch_mark_delete() noexcept override {
    mark = nullptr;
  }
  int decompress(int) noexcept override {
    assert(0 && "decompress not implemented for str");
  }
};

struct tl_out_methods_str final : tl_out_methods {
  char *str;
  explicit tl_out_methods_str(char *str)
    : str(str) {}
  void *store_get_ptr(int len) noexcept override {
    void *r = str;
    str += len;
    return r;
  }
  void store_raw_data(const void *buf, int len) noexcept override {
    memcpy(str, buf, len);
    str += len;
  }
};

void tl_store_init_str(char *s, int size) {
  tl_store_init(std::make_unique<tl_out_methods_str>(s), size);
}

void tl_fetch_init_str(const char *s, int size) {
  return tl_fetch_init(std::make_unique<tl_in_methods_str>(s), size);
}
