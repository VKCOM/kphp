//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/md5.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/crypto/crypto-state.h"
#include "runtime-light/stdlib/string/string-state.h"
#include "runtime-light/tl/tl-types.h"

task_t<Optional<string>> f$openssl_random_pseudo_bytes(int64_t length) noexcept;

task_t<Optional<array<mixed>>> f$openssl_x509_parse(const string &data, bool shortnames = true) noexcept;

task_t<bool> f$openssl_sign(const string &data, string &signature, const string &private_key, int64_t algo = tl::HashAlgorithm::SHA1) noexcept;

task_t<int64_t> f$openssl_verify(const string &data, const string &signature, const string &pub_key_id, int64_t algo = tl::HashAlgorithm::SHA1) noexcept;

array<string> f$openssl_get_cipher_methods(bool aliases = false) noexcept;

Optional<int64_t> f$openssl_cipher_iv_length(const string &method) noexcept;

task_t<Optional<string>> f$openssl_encrypt(const string &data, const string &method, const string &key, int64_t options = 0, const string &iv = string{},
                                           string &tag = CryptoInstanceState::get().default_tag_dummy, const string &aad = string{},
                                           int64_t tag_length = 16) noexcept;
task_t<Optional<string>> f$openssl_decrypt(string data, const string &method, const string &key, int64_t options = 0, const string &iv = string{},
                                           string tag = string{}, const string &aad = string{}) noexcept;


array<string> f$hash_algos() noexcept;
array<string> f$hash_hmac_algos() noexcept;

// function hash_algos () ::: string[];
// function hash_hmac_algos () ::: string[];
// function hash ($algo ::: string, $data ::: string, $raw_output ::: bool = false) ::: string;
// function hash_hmac ($algo ::: string, $data ::: string, $key ::: string, $raw_output ::: bool = false) ::: string;
// function hash_equals(string $known_string, string $user_string) ::: bool;
// function sha1 ($s ::: string, $raw_output ::: bool = false) ::: string;

inline string f$md5(const string &str, bool binary = false) noexcept {
  constexpr auto MD5_HASH_LEN = 16;
  string output{static_cast<string::size_type>(MD5_HASH_LEN * (binary ? 1 : 2)), false};
  md5(reinterpret_cast<const unsigned char *>(str.c_str()), static_cast<int32_t>(str.size()), reinterpret_cast<unsigned char *>(output.buffer()));
  if (!binary) {
    const auto &lhex_digits{StringImageState::get().lhex_digits};
    for (int64_t i = MD5_HASH_LEN - 1; i >= 0; --i) {
      output[2 * i + 1] = lhex_digits[output[i] & 0x0F];
      output[2 * i] = lhex_digits[(output[i] >> 4) & 0x0F];
    }
  }
  return output;
}

