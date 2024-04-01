// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "common/pid.h"
#include "common/tl/methods/base.h"

constexpr int NETWORK_MAX_STORED_SIZE = 1 << 27;

struct tl_out_methods_network : tl_out_methods {
  virtual void store_reset() noexcept = 0;
  virtual void store_flush(const char *, int) noexcept = 0;
  virtual int compress(int) noexcept = 0;
  virtual const process_id_t *get_pid() noexcept = 0;
  virtual void* get_connection() noexcept { return nullptr; }
};


