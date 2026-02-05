//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-tl-error.h"

#include <tuple>

#include "common/tl/constants/common.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/rpc/rpc-api.h"
#include "runtime-light/stdlib/rpc/rpc-tl-builtins.h"
#include "runtime-light/tl/tl-types.h"

namespace {

template<typename Type>
bool fetch_and_skip_n(int32_t n, tl::fetcher& fetcher) noexcept {
  for (auto i = 0; i < n; ++i) {
    if (!Type{}.fetch(fetcher)) [[unlikely]] {
      return false;
    }
  }
  return true;
}

} // namespace

bool TlRpcError::try_fetch() noexcept {
  const auto backup_pos{tl_parse_save_pos()};
  auto op{TRY_CALL(decltype(f$fetch_int()), bool, f$fetch_int())};
  if (op == TL_REQ_RESULT_HEADER) {
    fetch_and_skip_header();
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

void TlRpcError::fetch_and_skip_header() const noexcept {
  auto& fetcher{RpcServerInstanceState::get().tl_fetcher};
  tl::i32 flags{};
  if (!flags.fetch(fetcher)) [[unlikely]] {
    THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
    return;
  }

  if (flags.value & vk::tl::common::rpc_req_result_extra_flags::binlog_pos && !tl::i64{}.fetch(fetcher)) [[unlikely]] {
    THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
    return;
  }
  if (flags.value & vk::tl::common::rpc_req_result_extra_flags::binlog_time && !tl::i64{}.fetch(fetcher)) [[unlikely]] {
    THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
    return;
  }
  if (flags.value & vk::tl::common::rpc_req_result_extra_flags::engine_pid && !fetch_and_skip_n<tl::i32>(3, fetcher)) [[unlikely]] {
    THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
    return;
  }
  if (flags.value & vk::tl::common::rpc_req_result_extra_flags::request_size && !tl::i32{}.fetch(fetcher)) [[unlikely]] {
    THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
    return;
  }
  if (flags.value & vk::tl::common::rpc_req_result_extra_flags::response_size && !tl::i32{}.fetch(fetcher)) [[unlikely]] {
    THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
    return;
  }
  if (flags.value & vk::tl::common::rpc_req_result_extra_flags::failed_subqueries && !tl::i32{}.fetch(fetcher)) [[unlikely]] {
    THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
    return;
  }
  if (flags.value & vk::tl::common::rpc_req_result_extra_flags::compression_version && !tl::i32{}.fetch(fetcher)) [[unlikely]] {
    THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
    return;
  }
  if (flags.value & vk::tl::common::rpc_req_result_extra_flags::stats) {
    tl::i32 size{};
    if (!size.fetch(fetcher)) [[unlikely]] {
      THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
      return;
    }
    if (!fetch_and_skip_n<tl::string>(size.value * 2, fetcher)) [[unlikely]] {
      THROW_EXCEPTION(kphp::rpc::exception::cant_fetch_string::make());
      return;
    }
  }
  if (flags.value & vk::tl::common::rpc_req_result_extra_flags::epoch_number && !tl::i64{}.fetch(fetcher)) [[unlikely]] {
    THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
    return;
  }
  if (flags.value & vk::tl::common::rpc_req_result_extra_flags::view_number && !tl::i64{}.fetch(fetcher)) [[unlikely]] {
    THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
    return;
  }
}
