// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "server/server-context.h"

class HttpServerContext : public ServerContext<64>, vk::not_copyable {
private:
  HttpServerContext() = default;

  friend class vk::singleton<HttpServerContext>;
};
