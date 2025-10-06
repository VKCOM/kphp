//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <functional>
#include <optional>

#include "common/md5.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/string/string-state.h"
#include "runtime-light/tl/tl-types.h"

kphp::coro::task<Optional<string>> f$openssl_random_pseudo_bytes(int64_t length) noexcept;

kphp::coro::task<Optional<array<mixed>>> f$openssl_x509_parse(string data, bool shortnames = true) noexcept;

kphp::coro::task<bool> f$openssl_sign(string data, string& signature, string private_key, int64_t algo = tl::HashAlgorithm::SHA1) noexcept;

kphp::coro::task<int64_t> f$openssl_verify(string data, string signature, string pub_key_id, int64_t algo = tl::HashAlgorithm::SHA1) noexcept;

array<string> f$openssl_get_cipher_methods(bool aliases = false) noexcept;

Optional<int64_t> f$openssl_cipher_iv_length(const string& method) noexcept;

kphp::coro::task<Optional<string>> f$openssl_encrypt(string data, string method, string key, int64_t options = 0, string iv = string{},
                                                     std::optional<std::reference_wrapper<string>> tag = {}, string aad = string{},
                                                     int64_t tag_length = 16) noexcept;
kphp::coro::task<Optional<string>> f$openssl_decrypt(string data, string method, string key, int64_t options = 0, string iv = string{}, string tag = string{},
                                                     string aad = string{}) noexcept;

kphp::coro::task<Optional<string>> f$openssl_pkey_get_public(const string& key) noexcept;

kphp::coro::task<Optional<string>> f$openssl_pkey_get_private(const string& key, const string& passphrase = string()) noexcept;

// TODO const string& for `data` and `key`
kphp::coro::task<bool> f$openssl_public_encrypt(string data, mixed& result, string key) noexcept;

kphp::coro::task<bool> f$openssl_private_decrypt(string data, mixed& result, string key) noexcept;

array<string> f$hash_algos() noexcept;

array<string> f$hash_hmac_algos() noexcept;

kphp::coro::task<string> f$hash(string algo_str, string s, bool raw_output = false) noexcept;

kphp::coro::task<string> f$hash_hmac(string algo_str, string s, string key, bool raw_output = false) noexcept;

kphp::coro::task<string> f$sha1(string s, bool raw_output = false) noexcept;

inline string f$md5(const string& str, bool binary = false) noexcept {
  constexpr auto MD5_HASH_LEN = 16;
  string output{static_cast<string::size_type>(MD5_HASH_LEN * (binary ? 1 : 2)), false};
  md5(reinterpret_cast<const unsigned char*>(str.c_str()), static_cast<int32_t>(str.size()), reinterpret_cast<unsigned char*>(output.buffer()));
  if (!binary) {
    const auto& lhex_digits{StringImageState::get().lhex_digits};
    for (int64_t i = MD5_HASH_LEN - 1; i >= 0; --i) {
      output[2 * i + 1] = lhex_digits[output[i] & 0x0F];
      output[2 * i] = lhex_digits[(output[i] >> 4) & 0x0F];
    }
  }
  return output;
}

int64_t f$crc32(const string& s) noexcept;
