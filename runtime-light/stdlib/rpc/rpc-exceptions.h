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

namespace details {

inline constexpr int64_t CODE = -1;
inline constexpr std::string_view DESCRIPTION = "not enough data to fetch";

} // namespace details

inline auto make(std::source_location loc = std::source_location::current()) noexcept {
  return kphp::exception::make_throwable<C$Exception>(string{loc.file_name()}, loc.line(), details::CODE,
                                                      string{details::DESCRIPTION.data(), details::DESCRIPTION.size()});
}

} // namespace not_enough_data_to_fetch

namespace cant_fetch_string { // TODO: add specific exceptions depending on the actual reason of fetch fail

namespace details {

inline constexpr int64_t CODE = -2;
inline constexpr std::string_view DESCRIPTION = "can't fetch string";

} // namespace details

inline auto make(std::source_location loc = std::source_location::current()) noexcept {
  return kphp::exception::make_throwable<C$Exception>(string{loc.file_name()}, loc.line(), details::CODE,
                                                      string{details::DESCRIPTION.data(), details::DESCRIPTION.size()});
}

} // namespace cant_fetch_string

namespace cant_fetch_function {

namespace details {

inline constexpr int64_t CODE = -3;
inline constexpr std::string_view DESCRIPTION = "can't fetch function: ";

} // namespace details

inline auto make(std::string_view func_name, std::source_location loc = std::source_location::current()) noexcept {
  string description{};
  description.reserve_at_least(details::DESCRIPTION.size() + func_name.size())
      .append(details::DESCRIPTION.data(), details::DESCRIPTION.size())
      .append(func_name.data(), func_name.size());
  return kphp::exception::make_throwable<C$Exception>(string{loc.file_name()}, loc.line(), details::CODE, description);
}

} // namespace cant_fetch_function

namespace cant_store_function {

namespace details {

inline constexpr int64_t CODE = -4;
inline constexpr std::string_view DESCRIPTION = "can't store function: ";

} // namespace details

inline auto make(std::string_view func_name, std::source_location loc = std::source_location::current()) noexcept {
  string description{};
  description.reserve_at_least(details::DESCRIPTION.size() + func_name.size())
      .append(details::DESCRIPTION.data(), details::DESCRIPTION.size())
      .append(func_name.data(), func_name.size());
  return kphp::exception::make_throwable<C$Exception>(string{loc.file_name()}, loc.line(), details::CODE, description);
}

} // namespace cant_store_function

} // namespace kphp::rpc::exception
