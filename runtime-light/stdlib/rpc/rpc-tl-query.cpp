//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-tl-query.h"

#include <array>
#include <string_view>

#include "runtime-light/server/rpc/rpc-server-state.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/rpc/rpc-client-state.h"
#include "runtime-light/stdlib/rpc/rpc-exceptions.h"
#include "runtime-light/utils/logs.h"

void CurrentTlQuery::raise_fetching_error(const char* format, ...) const noexcept {
  kphp::log::assertion(!current_tl_function_name.empty());
  CHECK_EXCEPTION(return);

  constexpr size_t BUFF_SZ = 1024;
  std::array<char, BUFF_SZ> buff{};

  va_list args;
  va_start(args, format);
  int32_t sz{vsnprintf(buff.data(), BUFF_SZ, format, args)};
  kphp::log::assertion(sz > 0);
  va_end(args);

  string msg{buff.data(), static_cast<string::size_type>(sz)};
  std::string_view func_name{current_tl_function_name.c_str(), current_tl_function_name.size()};

  kphp::log::warning("Fetching error:\n{}\nIn {} deserializing TL object", msg.c_str(), func_name);
  THROW_EXCEPTION(kphp::rpc::exception::cant_fetch_function::make(func_name));
}

void CurrentTlQuery::raise_storing_error(const char* format, ...) const noexcept {
  CHECK_EXCEPTION(return);

  constexpr size_t BUFF_SZ = 1024;
  std::array<char, BUFF_SZ> buff{};

  va_list args;
  va_start(args, format);
  int32_t sz{vsnprintf(buff.data(), BUFF_SZ, format, args)};
  kphp::log::assertion(sz > 0);
  va_end(args);

  string msg{buff.data(), static_cast<string::size_type>(sz)};
  std::string_view func_name{"_unknown_"};
  if (current_tl_function_name.size() != 0) [[likely]] {
    func_name = {current_tl_function_name.c_str(), current_tl_function_name.size()};
  }

  kphp::log::warning("Storing error:\n{}\nIn {} serializing TL object", msg.c_str(), func_name);
  THROW_EXCEPTION(kphp::rpc::exception::cant_store_function::make(func_name));
}

CurrentTlQuery& CurrentTlQuery::get() noexcept {
  return RpcClientInstanceState::get().current_client_query;
}

// ================================================================================================

CurrentRpcServerQuery& CurrentRpcServerQuery::get() noexcept {
  return RpcServerInstanceState::get().current_server_query;
}
