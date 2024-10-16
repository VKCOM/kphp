//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/crypto/crypto-context.h"

#include "runtime-light/component/component.h"

CryptoComponentContext &CryptoComponentContext::get() noexcept {
  return get_component_context()->crypto_component_context;
}
