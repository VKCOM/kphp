// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <openssl/pkcs7.h>

#include "runtime/stream_functions.h"

struct ssl_stream_functions : stream_functions {
  ssl_stream_functions() {
    name = string("ssl");
    supported_functions.set_value(string("fopen"), false);
    supported_functions.set_value(string("fwrite"), true);
    supported_functions.set_value(string("fseek"), false);
    supported_functions.set_value(string("ftell"), false);
    supported_functions.set_value(string("fread"), true);
    supported_functions.set_value(string("fgetc"), false);
    supported_functions.set_value(string("fgets"), false);
    supported_functions.set_value(string("fpassthru"), false);
    supported_functions.set_value(string("fflush"), false);
    supported_functions.set_value(string("feof"), true);
    supported_functions.set_value(string("fclose"), true);
    supported_functions.set_value(string("file_get_contents"), false);
    supported_functions.set_value(string("file_put_contents"), false);
    supported_functions.set_value(string("stream_socket_client"), true);
    supported_functions.set_value(string("context_set_option"), true);
    supported_functions.set_value(string("stream_set_option"), true);
    supported_functions.set_value(string("get_fd"), true);
  }

  ~ssl_stream_functions() override = default;

  Optional<int64_t> fwrite(const Stream &stream, const string &data) const override;

  Optional<string> fread(const Stream &stream, int64_t length) const override;

  bool feof(const Stream &stream) const override;

  bool fclose(const Stream &stream) const override;

  Stream stream_socket_client(const string &url, int64_t &error_number, string &error_description, double timeout, int64_t flags, const mixed &context) const override;

  bool context_set_option(mixed &context, const string &option, const mixed &value) const override;

  bool stream_set_option(const Stream &stream, int64_t option, int64_t value) const override;

  int32_t get_fd(const Stream &stream) const override;
};

enum openssl_algo {
  OPENSSL_ALGO_SHA1 = 1,
  OPENSSL_ALGO_MD5 = 2,
  OPENSSL_ALGO_MD4 = 3,
  OPENSSL_ALGO_MD2 = 4,
  OPENSSL_ALGO_DSS1 = 5,
  OPENSSL_ALGO_SHA224 = 6,
  OPENSSL_ALGO_SHA256 = 7,
  OPENSSL_ALGO_SHA384 = 8,
  OPENSSL_ALGO_SHA512 = 9,
  OPENSSL_ALGO_RMD160 = 10,
};

array<string> f$hash_algos() noexcept;

array<string> f$hash_hmac_algos() noexcept;

string f$hash(const string &algo, const string &s, bool raw_output = false) noexcept;

string f$hash_hmac(const string &algo, const string &data, const string &key, bool raw_output = false) noexcept;

string f$sha1(const string &s, bool raw_output = false) noexcept;

string f$md5(const string &s, bool raw_output = false) noexcept;

Optional<string> f$md5_file(const string &file_name, bool raw_output = false);

int64_t f$crc32(const string &s);

int64_t f$crc32_file(const string &file_name);

bool f$hash_equals(const string &known_string, const string &user_string) noexcept;

bool f$openssl_public_encrypt(const string &data, string &result, const string &key);

bool f$openssl_public_encrypt(const string &data, mixed &result, const string &key);

bool f$openssl_private_decrypt(const string &data, string &result, const string &key);

bool f$openssl_private_decrypt(const string &data, mixed &result, const string &key);

Optional<string> f$openssl_pkey_get_private(const string &key, const string &passphrase = string());

Optional<string> f$openssl_pkey_get_public(const string &key);

bool f$openssl_sign(const string &data, string &signature, const string &priv_key_id, int64_t algo = OPENSSL_ALGO_SHA1);

int64_t f$openssl_verify(const string &data, const string &signature, const string &pub_key_id, int64_t algo = OPENSSL_ALGO_SHA1);

Optional<string> f$openssl_random_pseudo_bytes(int64_t length);

Optional<array<mixed>> f$openssl_x509_parse(const string &data, bool shortnames = true);

int64_t f$openssl_x509_verify(const string &certificate, const string &public_key);

mixed f$openssl_x509_checkpurpose(const string &data, int64_t purpose);

bool f$openssl_pkcs7_sign(const string &infile, const string &outfile,
                          const string &sign_cert, const string &priv_key,
                          const array<string> &headers, int64_t flags = PKCS7_DETACHED, const string &extra_certs = {}) noexcept;

array<string> f$openssl_get_cipher_methods(bool aliases = false);

Optional<int64_t> f$openssl_cipher_iv_length(const string &method);

namespace impl_ {
extern string default_tag_stub;
} // namespace impl_
Optional<string> f$openssl_encrypt(const string &data, const string &method,
                                   const string &key, int64_t options = 0, const string &iv = string{},
                                   string &tag = impl_::default_tag_stub, const string &aad = string{}, int64_t tag_length = 16);
Optional<string> f$openssl_decrypt(string data, const string &method,
                                   const string &key, int64_t options = 0, const string &iv = string{},
                                   string tag = string{}, const string &aad = string{});

void global_init_openssl_lib();

void init_openssl_lib();
void free_openssl_lib();
