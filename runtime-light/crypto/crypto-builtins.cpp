//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/crypto/crypto-builtins.h"

#include "runtime-light/stdlib/rpc/rpc-buffer.h"
#include "runtime-light/streams/interface.h"

// Crypto-Specific TL magics

constexpr uint32_t TL_GET_CRYPTOSECURE_PSEUDORANDOM_BYTES = 0x2491'b81d;

task_t<Optional<string>> f$openssl_random_pseudo_bytes(int64_t length) {
  if (length <= 0 || length > string::max_size()) {
    co_return false;
  }

  RpcBuffer buffer;
  buffer.store_trivial(static_cast<uint32_t>(TL_GET_CRYPTOSECURE_PSEUDORANDOM_BYTES));
  buffer.store_trivial(static_cast<int32_t>(length));


  string request_buf{};
  request_buf.append(buffer.data(), buffer.size());

  auto query = f$component_client_send_query(string("crypto"), request_buf);
  co_return co_await f$component_client_get_result(co_await query);
}
