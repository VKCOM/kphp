// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "server/server-context.h"

class RpcServerContext : public ServerContext<>, vk::not_copyable {
private:
  RpcServerContext() = default;

  friend class vk::singleton<RpcServerContext>;
};
