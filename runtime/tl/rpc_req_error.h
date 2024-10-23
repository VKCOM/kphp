// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/runtime-core/runtime-core.h"

struct RpcError {
  int error_code = 0;
  string error_msg = {};

  bool try_fetch() noexcept;

private:
  void fetch_and_skip_header(int flags) const noexcept;
};
