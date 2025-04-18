// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <source_location>
#include <string_view>

#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/diagnostics/exception-types.h"

namespace kphp::rpc::exception {

namespace not_enough_data_to_fetch {

inline constexpr int64_t CODE = -1;
inline constexpr std::string_view DESCRIPTION = "not enough data to fetch";

inline auto make(std::source_location loc = std::source_location::current()) noexcept {
  return kphp::exception::make_throwable<C$Exception>(
      string{loc.file_name()}, loc.line(), kphp::rpc::exception::not_enough_data_to_fetch::CODE,
      string{kphp::rpc::exception::not_enough_data_to_fetch::DESCRIPTION.data(), kphp::rpc::exception::not_enough_data_to_fetch::DESCRIPTION.size()});
}

} // namespace not_enough_data_to_fetch

namespace cant_fetch_string { // TODO: add specific exceptions depending on the actual reason of fetch fail

inline constexpr int64_t CODE = -2;
inline constexpr std::string_view DESCRIPTION = "can't fetch string";

inline auto make(std::source_location loc = std::source_location::current()) noexcept {
  return kphp::exception::make_throwable<C$Exception>(
      string{loc.file_name()}, loc.line(), kphp::rpc::exception::cant_fetch_string::CODE,
      string{kphp::rpc::exception::cant_fetch_string::DESCRIPTION.data(), kphp::rpc::exception::cant_fetch_string::DESCRIPTION.size()});
}

} // namespace cant_fetch_string

namespace cant_fetch_function {

inline constexpr int64_t CODE = -3;
inline constexpr std::string_view DESCRIPTION = "can't fetch function";

inline auto make(std::source_location loc = std::source_location::current()) noexcept {
  return kphp::exception::make_throwable<C$Exception>(
      string{loc.file_name()}, loc.line(), kphp::rpc::exception::cant_fetch_function::CODE,
      string{kphp::rpc::exception::cant_fetch_function::DESCRIPTION.data(), kphp::rpc::exception::cant_fetch_function::DESCRIPTION.size()});
}

} // namespace cant_fetch_function

namespace cant_store_function {

inline constexpr int64_t CODE = -4;
inline constexpr std::string_view DESCRIPTION = "can't store function";

inline auto make(std::source_location loc = std::source_location::current()) noexcept {
  return kphp::exception::make_throwable<C$Exception>(
      string{loc.file_name()}, loc.line(), kphp::rpc::exception::cant_store_function::CODE,
      string{kphp::rpc::exception::cant_store_function::DESCRIPTION.data(), kphp::rpc::exception::cant_store_function::DESCRIPTION.size()});
}

} // namespace cant_store_function

} // namespace kphp::rpc::exception
