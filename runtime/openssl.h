#pragma once

#include <openssl/pkcs7.h>

#include "runtime/kphp_core.h"

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

array<string> f$hash_algos();

bool f$hash_equals(const string &known_string, const string &user_string) noexcept;

string f$hash(const string &algo, const string &s, bool raw_output = false);

string f$hash_hmac(const string &algo, const string &data, const string &key, bool raw_output = false);

string f$sha1(const string &s, bool raw_output = false);

string f$md5(const string &s, bool raw_output = false);

Optional<string> f$md5_file(const string &file_name, bool raw_output = false);

int64_t f$crc32(const string &s);

int64_t f$crc32_file(const string &file_name);


bool f$openssl_public_encrypt(const string &data, string &result, const string &key);

bool f$openssl_public_encrypt(const string &data, var &result, const string &key);

bool f$openssl_private_decrypt(const string &data, string &result, const string &key);

bool f$openssl_private_decrypt(const string &data, var &result, const string &key);

Optional<string> f$openssl_pkey_get_private(const string &key, const string &passphrase = string());

Optional<string> f$openssl_pkey_get_public(const string &key);

bool f$openssl_sign(const string &data, string &signature, const string &priv_key_id, int64_t algo = OPENSSL_ALGO_SHA1);

int64_t f$openssl_verify(const string &data, const string &signature, const string &pub_key_id, int64_t algo = OPENSSL_ALGO_SHA1);

Optional<string> f$openssl_random_pseudo_bytes(int64_t length);

Optional<array<var>> f$openssl_x509_parse(const string &data, bool shortnames = true);

var f$openssl_x509_checkpurpose(const string &data, int64_t purpose);

bool f$openssl_pkcs7_sign(const string &infile, const string &outfile,
                          const string &sign_cert, const string &priv_key,
                          const array<string> &headers, int64_t flags = PKCS7_DETACHED, const string &extra_certs = {}) noexcept;

array<string> f$openssl_get_cipher_methods(bool aliases = false);

Optional<int64_t> f$openssl_cipher_iv_length(const string &method);

Optional<string> f$openssl_encrypt(const string &data, const string &method,
                                  const string &key, int64_t options = 0, const string &iv = string());
Optional<string> f$openssl_decrypt(const string &data, const string &method,
                                  const string &key, int64_t options = 0, const string &iv = string());

void global_init_openssl_lib();
void reinit_openssl_lib_hack();

void free_openssl_lib();
