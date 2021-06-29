// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "flex/flex.h"

#include <cstring>

int node::get_child(unsigned char c, const lang *ctx) const noexcept {
  int l = children_start;
  int r = children_end;
  if (r - l <= 4) {
    for (int i = l; i < r; i++) {
      if (ctx->children[2 * i] == c) {
        return ctx->children[2 * i + 1];
      }
    }
  } else {
    while (r - l > 1) {
      int x = (r + l) >> 1;
      if (ctx->children[2 * x] <= c) {
        l = x;
      } else {
        r = x;
      }
    }
    if (ctx->children[2 * l] == c) {
      return ctx->children[2 * l + 1];
    }
  }
  return -1;
}

bool lang::has_symbol(char c) const noexcept {
  return strchr(flexible_symbols, c) != nullptr;
}
