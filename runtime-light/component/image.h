// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-light/stdlib/rpc/rpc-context.h"

struct ImageState {
  char *c_linear_mem;
  RpcImageState rpc_image_state;
};
