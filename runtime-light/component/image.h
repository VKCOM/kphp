// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime-light/stdlib/rpc/rpc-context.h"
#include "runtime-light/stdlib/string/regexp-context.h"
#include "runtime-light/stdlib/string/string-context.h"

struct ImageState final : private vk::not_copyable {
  char *c_linear_mem;
  RpcImageState rpc_image_state{};
  StringImageState string_image_state{};
  RegexpImageState regexp_image_state{};
};
