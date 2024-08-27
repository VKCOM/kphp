//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/crypto/crypto-builtins.h"

#include "runtime-light/streams/interface.h"
#include "runtime-light/tl/tl-core.h"

// Crypto-Specific TL magics
[[maybe_unused]] constexpr uint32_t TL_ZERO = 0x0000'0000;
[[maybe_unused]] constexpr uint32_t TL_BOOL_FALSE = 0xbc79'9737;
[[maybe_unused]] constexpr uint32_t TL_BOOL_TRUE = 0x9972'75b5;

[[maybe_unused]] constexpr uint32_t TL_RESULT_FALSE = 0x2793'0a7b;
[[maybe_unused]] constexpr uint32_t TL_RESULT_TRUE = 0x3f9c'8ef8;

[[maybe_unused]] constexpr uint32_t TL_VECTOR = 0x1cb5'c415;
[[maybe_unused]] constexpr uint32_t TL_TUPLE = 0x9770'768a;
[[maybe_unused]] constexpr uint32_t TL_DICTIONARY = 0x1f4c'618f;

[[maybe_unused]] constexpr uint32_t TL_CERT_INFO_ITEM_LONG = 0x533f'f89f;
[[maybe_unused]] constexpr uint32_t TL_CERT_INFO_ITEM_STR = 0xc427'feef;
[[maybe_unused]] constexpr uint32_t TL_CERT_INFO_ITEM_DICT = 0x1ea8'a774;

[[maybe_unused]] constexpr uint32_t TL_GET_CRYPTOSECURE_PSEUDORANDOM_BYTES = 0x2491'b81d;
[[maybe_unused]] constexpr uint32_t TL_GET_PEM_CERT_INFO = 0xa50c'fd6c;

task_t<Optional<string>> f$openssl_random_pseudo_bytes(int64_t length) {
  if (length <= 0 || length > string::max_size()) {
    co_return false;
  }

  tl::TLBuffer buffer;
  buffer.store_trivial<uint32_t>(TL_GET_CRYPTOSECURE_PSEUDORANDOM_BYTES);
  buffer.store_trivial<int32_t>(length);

  // TODO think about performance when transferring data

  string request_buf{};
  request_buf.append(buffer.data(), buffer.size());

  auto query = f$component_client_send_query(string("crypto"), request_buf);
  string resp = co_await f$component_client_get_result(co_await query);

  buffer.clean();
  buffer.store_bytes(resp.c_str(), resp.size());

  std::optional<uint32_t> magic = buffer.fetch_trivial<uint32_t>();
  if (!magic || *magic != TL_RESULT_TRUE) {
    co_return false;
  }
  std::string_view str_view = buffer.fetch_string();
  co_return string(str_view.data(), str_view.size());
}

task_t<Optional<array<mixed>>> f$openssl_x509_parse(const string &data, bool shortnames) {
  tl::TLBuffer buffer;
  buffer.store_trivial<uint32_t>(TL_GET_PEM_CERT_INFO);
  buffer.store_trivial<uint32_t>(shortnames ? TL_BOOL_TRUE : TL_BOOL_FALSE);
  buffer.store_string(std::string_view{data.c_str(), data.size()});

  string request_buf{};
  request_buf.append(buffer.data(), buffer.size());

  auto query = f$component_client_send_query(string("crypto"), request_buf);
  string resp_from_platform = co_await f$component_client_get_result(co_await query);

  buffer.clean();
  buffer.store_bytes(resp_from_platform.c_str(), resp_from_platform.size());

  if (auto magic = buffer.fetch_trivial<uint32_t>(); magic.value_or(TL_ZERO) != TL_RESULT_TRUE) {
    co_return false;
  }

  if (auto magic = buffer.fetch_trivial<uint32_t>(); magic.value_or(TL_ZERO) != TL_DICTIONARY) {
    co_return false;
  }

  uint32_t size;
  if (auto size_op = buffer.fetch_trivial<uint32_t>(); size_op) {
    size = *size_op;
  } else {
    co_return false;
  }

  auto response = array<mixed>::create();

  for (uint32_t i = 0; i < size; ++i) {
    auto key_view = buffer.fetch_string();
    if (key_view.empty()) {
      co_return false;
    }

    string key(key_view.data(), key_view.length());

    const auto magic = buffer.fetch_trivial<uint32_t>();
    if (!magic.has_value()) {
      co_return false;
    }

    switch (*magic) {
      case TL_CERT_INFO_ITEM_LONG: {
        auto value = buffer.fetch_trivial<int64_t>();
        if (!value.has_value()) {
          co_return false;
        }
        response[key] = *value;
        break;
      }
      case TL_CERT_INFO_ITEM_STR: {
        auto value = buffer.fetch_string();
        if (value.empty()) {
          co_return false;
        }
        response[key] = string(value.data(), value.size());
        break;
      }
      case TL_CERT_INFO_ITEM_DICT: {
        auto sub_array = array<string>::create();
        const auto sub_size = buffer.fetch_trivial<uint32_t>();

        if (!sub_size.has_value()) {
          co_return false;
        }

        for (size_t j = 0; j < sub_size; ++j) {
          auto sub_key = buffer.fetch_string();
          if (sub_key.empty()) {
            co_return false;
          }
          auto sub_value = buffer.fetch_string();
          if (sub_value.empty()) {
            co_return false;
          }

          sub_array[string(sub_key.data(), sub_key.size())] = string(sub_value.data(), sub_value.size());
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