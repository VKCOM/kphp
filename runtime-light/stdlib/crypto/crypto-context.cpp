//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/crypto/crypto-context.h"

#include "runtime-light/state/instance-state.h"

CryptoInstanceState &CryptoInstanceState::get() noexcept {
  return InstanceState::get().crypto_instance_state;
}
