// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "runtime-common/stdlib/diagnostics/builtin-time-stats.h"

enum class CryptoBuiltin : uint8_t {
  hash_algos,
  hash_hmac_algos,
  hash,
  hash_hmac,
  sha1,
  md5,
  hash_equals,
  openssl_public_encrypt,
  openssl_private_decrypt,
  openssl_pkey_get_private,
  openssl_pkey_get_public,
  openssl_sign,
  openssl_verify,
  openssl_random_pseudo_bytes,
  openssl_x509_parse,
  openssl_get_cipher_methods,
  openssl_cipher_iv_length,
  openssl_encrypt,
  openssl_decrypt,
  count,
};

inline constexpr std::array<std::string_view, static_cast<size_t>(CryptoBuiltin::count)> CRYPTO_BUILTIN_NAMES{
    "hash_algos",
    "hash_hmac_algos",
    "hash",
    "hash_hmac",
    "sha1",
    "md5",
    "hash_equals",
    "openssl_public_encrypt",
    "openssl_private_decrypt",
    "openssl_pkey_get_private",
    "openssl_pkey_get_public",
    "openssl_sign",
    "openssl_verify",
    "openssl_random_pseudo_bytes",
    "openssl_x509_parse",
    "openssl_get_cipher_methods",
    "openssl_cipher_iv_length",
    "openssl_encrypt",
    "openssl_decrypt",
};

struct CryptoTimeStats final : BuiltinTimeStats<CryptoBuiltin, static_cast<size_t>(CryptoBuiltin::count)> {
  static CryptoTimeStats& get() noexcept;
};
