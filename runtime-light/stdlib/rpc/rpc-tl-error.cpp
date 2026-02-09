//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-tl-error.h"

#include <tuple>

#include "common/tl/constants/common.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/rpc/rpc-api.h"
#include "runtime-light/stdlib/rpc/rpc-tl-builtins.h"

bool TlRpcError::try_fetch() noexcept {
  const auto backup_pos{tl_parse_save_pos()};
  auto op{TRY_CALL(decltype(f$fetch_int()), bool, f$fetch_int())};
  if (op == TL_REQ_RESULT_HEADER) {
    if (!fetch_and_skip_header()) [[unlikely]] {
      THROW_EXCEPTION(kphp::rpc::exception::cant_fetch_header::make());
      return false;
    }
    op = TRY_CALL(decltype(f$fetch_int()), bool, f$fetch_int());
  }
  if (op != TL_RPC_REQ_ERROR) {
    tl_parse_restore_pos(backup_pos);
    return false;
  }

  std::ignore = TRY_CALL(decltype(f$fetch_long()), bool, f$fetch_long());
  error_code = static_cast<int32_t>(TRY_CALL(decltype(f$fetch_int()), bool, f$fetch_int()));
  error_msg = TRY_CALL(decltype(f$fetch_string()), bool, f$fetch_string());
  return true;
}

bool TlRpcError::fetch_and_skip_header() const noexcept {
  auto& fetcher{RpcServerInstanceState::get().tl_fetcher};
  tl::mask extra_flags{};
  return extra_flags.fetch(fetcher) && tl::rpcReqResultExtra{}.fetch(fetcher, extra_flags);
}
