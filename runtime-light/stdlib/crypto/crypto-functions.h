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

Optional<string> f$openssl_random_pseudo_bytes(int64_t length) noexcept;

kphp::coro::task<Optional<array<mixed>>> f$openssl_x509_parse(string data, bool shortnames = true) noexcept;

kphp::coro::task<bool> f$openssl_sign(string data, string& signature, string private_key, int64_t algo = tl::HashAlgorithm::SHA1) noexcept;

kphp::coro::task<int64_t> f$openssl_verify(string data, string signature, string pub_key_id, int64_t algo = tl::HashAlgorithm::SHA1) noexcept;

array<string> f$openssl_get_cipher_methods(bool aliases = false) noexcept;

Optional<int64_t> f$openssl_cipher_iv_length(const string& algorithm) noexcept;

Optional<string> f$openssl_encrypt(const string& data, const string& algorithm, const string& key, int64_t options = 0, const string& iv = string{},
                                   std::optional<std::reference_wrapper<string>> tag = {}, const string& aad = string{}, int64_t tag_length = 16) noexcept;

Optional<string> f$openssl_decrypt(const string& data, const string& algorithm, const string& key, int64_t options = 0, const string& iv = string{},
                                   const string& tag = string{}, const string& aad = string{}) noexcept;

Optional<string> f$openssl_pkey_get_public(const string& public_key) noexcept;

Optional<string> f$openssl_pkey_get_private(const string& private_key, const string& passphrase = string{}) noexcept;

bool f$openssl_public_encrypt(const string& data, mixed& encrypted_data, const string& public_key) noexcept;

bool f$openssl_private_decrypt(const string& data, mixed& decrypted_data, const string& private_key) noexcept;

array<string> f$hash_algos() noexcept;

array<string> f$hash_hmac_algos() noexcept;

string f$hash(const string& algorithm, const string& data, bool binary = false) noexcept;

string f$hash_hmac(const string& algorithm, const string& data, const string& key, bool binary = false) noexcept;

string f$sha1(const string& data, bool binary = false) noexcept;

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
