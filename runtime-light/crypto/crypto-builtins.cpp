//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/crypto/crypto-builtins.h"

#include "common/tl/constants/common.h"
#include "runtime-light/stdlib/component/component-api.h"
#include "runtime-light/tl/tl-core.h"

namespace {

// Crypto-Specific TL magics

constexpr uint32_t TL_CERT_INFO_ITEM_LONG = 0x533f'f89f;
constexpr uint32_t TL_CERT_INFO_ITEM_STR = 0xc427'feef;
constexpr uint32_t TL_CERT_INFO_ITEM_DICT = 0x1ea8'a774;

constexpr uint32_t TL_GET_PEM_CERT_INFO = 0xa50c'fd6c;
constexpr uint32_t TL_GET_CRYPTOSECURE_PSEUDORANDOM_BYTES = 0x2491'b81d;
} // namespace

task_t<Optional<string>> f$openssl_random_pseudo_bytes(int64_t length) noexcept {
  if (length <= 0 || length > string::max_size()) {
    co_return false;
  }

  tl::TLBuffer buffer;
  buffer.store_trivial<uint32_t>(TL_GET_CRYPTOSECURE_PSEUDORANDOM_BYTES);
  buffer.store_trivial<int32_t>(length);

  // TODO think about performance when transferring data

  string request_buf;
  request_buf.append(buffer.data(), buffer.size());

  auto query = f$component_client_send_request(string("crypto"), request_buf);
  string resp = co_await f$component_client_fetch_response(co_await query);

  buffer.clean();
  buffer.store_bytes(resp.c_str(), resp.size());

  std::optional<uint32_t> magic = buffer.fetch_trivial<uint32_t>();
  if (!magic.has_value() || *magic != TL_MAYBE_TRUE) {
    co_return false;
  }
  std::string_view str_view = buffer.fetch_string();
  co_return string(str_view.data(), str_view.size());
}

task_t<Optional<array<mixed>>> f$openssl_x509_parse(const string &data, bool shortnames) noexcept {
  tl::TLBuffer buffer;
  buffer.store_trivial<uint32_t>(TL_GET_PEM_CERT_INFO);
  buffer.store_trivial<uint32_t>(shortnames ? TL_BOOL_TRUE : TL_BOOL_FALSE);
  buffer.store_string(std::string_view(data.c_str(), data.size()));

  string request_buf;
  request_buf.append(buffer.data(), buffer.size());

  auto query = f$component_client_send_request(string("crypto"), request_buf);
  string resp_from_platform = co_await f$component_client_fetch_response(co_await query);

  buffer.clean();
  buffer.store_bytes(resp_from_platform.c_str(), resp_from_platform.size());

  if (const auto magic = buffer.fetch_trivial<uint32_t>(); magic.value_or(TL_ZERO) != TL_MAYBE_TRUE) {
    co_return false;
  }

  if (const auto magic = buffer.fetch_trivial<uint32_t>(); magic.value_or(TL_ZERO) != TL_DICTIONARY) {
    co_return false;
  }

  const std::optional<uint32_t> size = buffer.fetch_trivial<uint32_t>();
  if (!size.has_value()) {
    co_return false;
  }

  auto response = array<mixed>::create();

  for (uint32_t i = 0; i < size; ++i) {
    const auto key_view = buffer.fetch_string();
    if (key_view.empty()) {
      co_return false;
    }

    const auto key = string(key_view.data(), key_view.length());

    const std::optional<uint32_t> magic = buffer.fetch_trivial<uint32_t>();
    if (!magic.has_value()) {
      co_return false;
    }

    switch (*magic) {
      case TL_CERT_INFO_ITEM_LONG: {
        const std::optional<int64_t> val = buffer.fetch_trivial<int64_t>();
        if (!val.has_value()) {
          co_return false;
        }
        response[key] = *val;
        break;
      }
      case TL_CERT_INFO_ITEM_STR: {
        const auto value_view = buffer.fetch_string();
        if (value_view.empty()) {
          co_return false;
        }
        const auto value = string(value_view.data(), value_view.size());

        response[key] = string(value_view.data(), value_view.size());
        break;
      }
      case TL_CERT_INFO_ITEM_DICT: {
        auto sub_array = array<string>::create();
        const std::optional<uint32_t> sub_size = buffer.fetch_trivial<uint32_t>();

        if (!sub_size.has_value()) {
          co_return false;
        }

        for (size_t j = 0; j < sub_size; ++j) {
          const auto sub_key_view = buffer.fetch_string();
          if (sub_key_view.empty()) {
            co_return false;
          }
          const auto sub_key = string(sub_key_view.data(), sub_key_view.size());

          const auto sub_value_view = buffer.fetch_string();
          if (sub_value_view.empty()) {
            co_return false;
          }
          const auto sub_value = string(sub_value_view.data(), sub_value_view.size());

          sub_array[sub_key] = sub_value;
        }
        response[key] = sub_array;

        break;
      }
      default:
        co_return false;
    }
  }

  co_return response;
}
