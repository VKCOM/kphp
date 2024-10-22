//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-core/runtime-core.h"

struct CryptoComponentContext final : vk::not_copyable {
  string default_tag_dummy;

  static CryptoComponentContext &get() noexcept;
};
