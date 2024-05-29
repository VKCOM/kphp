#pragma once

#include "runtime-light/header.h"
#include "runtime-light/stdlib/rpc/rpc_context.h"

struct ImageState {
  char *c_linear_mem;
  RpcImageState rpc_image_state;
};
