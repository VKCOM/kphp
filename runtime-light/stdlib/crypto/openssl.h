// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

#include "runtime-light/k2-platform/k2-api.h"

namespace kphp::openssl {

/**
 * Base class of all OpenSSL objects: a move-only holder of a platform descriptor.
 *
 * An owning object frees its descriptor on destruction. A borrowing object (see `borrow` of the derived classes that allow it)
 * refers to a descriptor owned by someone else and never frees it.
 * The member functions of the derived classes are thin wrappers of the `k2::openssl_*` functions with the same OpenSSL name.
 */
class object {
  k2::descriptor m_descriptor{k2::INVALID_PLATFORM_DESCRIPTOR};
  bool m_owned{false};

  auto reset() noexcept -> void {
    if (m_owned && m_descriptor != k2::INVALID_PLATFORM_DESCRIPTOR) {
      k2::free_descriptor(m_descriptor);
    }

    m_descriptor = k2::INVALID_PLATFORM_DESCRIPTOR;
    m_owned = false;
  }

protected:
  explicit object(k2::descriptor descriptor, bool owned = true) noexcept
      : m_descriptor(descriptor),
        m_owned(owned) {}

  object(const object&) = delete;

  object(object&& other) noexcept
      : m_descriptor(std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR)),
        m_owned(std::exchange(other.m_owned, false)) {}

  auto operator=(const object&) -> object& = delete;

  auto operator=(object&& other) noexcept -> object& {
    if (this != std::addressof(other)) {
      reset();
      m_descriptor = std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR);
      m_owned = std::exchange(other.m_owned, false);
    }

    return *this;
  }

  ~object() {
    reset();
  }

public:
  auto descriptor() const noexcept -> k2::descriptor {
    return m_descriptor;
  }

  // gives up the ownership: the caller becomes responsible for freeing the returned descriptor
  auto release() noexcept -> k2::descriptor {
    m_owned = false;
    return std::exchange(m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR);
  }
};

class rsa final : public object {
  explicit rsa(k2::descriptor descriptor) noexcept
      : object(descriptor) {}

public:
  static auto adopt(k2::descriptor descriptor) noexcept -> rsa {
    return rsa{descriptor};
  }

  auto public_encrypt(std::span<const std::byte> from, std::span<std::byte> to, k2::RsaPadding padding,
                      int32_t* openssl_ret = nullptr) const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_rsa_public_encrypt(from, to, descriptor(), padding, openssl_ret);
  }

  auto private_decrypt(std::span<const std::byte> from, std::span<std::byte> to, k2::RsaPadding padding,
                       int32_t* openssl_ret = nullptr) const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_rsa_private_decrypt(from, to, descriptor(), padding, openssl_ret);
  }
};

/**
 * A private or public key.
 */
class pkey final : public object {
  pkey(k2::descriptor descriptor, bool owned) noexcept
      : object(descriptor, owned) {}

public:
  static auto read_pem_pubkey(std::span<const std::byte> pem) noexcept -> std::expected<pkey, int32_t> {
    return k2::openssl_pem_read_bio_pubkey(pem).transform(adopt);
  }

  static auto read_pem_private_key(std::span<const std::byte> pem, std::span<const std::byte> pass) noexcept -> std::expected<pkey, int32_t> {
    return k2::openssl_pem_read_bio_private_key(pem, pass).transform(adopt);
  }

  // takes ownership of the descriptor
  static auto adopt(k2::descriptor descriptor) noexcept -> pkey {
    return pkey{descriptor, true};
  }

  // does not take ownership of the descriptor, the caller guarantees that it outlives the key
  static auto borrow(k2::descriptor descriptor) noexcept -> pkey {
    return pkey{descriptor, false};
  }

  auto id() const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_evp_pkey_id(descriptor());
  }

  auto size() const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_evp_pkey_size(descriptor());
  }

  auto get1_rsa() const noexcept -> std::expected<rsa, int32_t> {
    return k2::openssl_evp_pkey_get1_rsa(descriptor()).transform(rsa::adopt);
  }
};

/**
 * A message digest context. It can be reused for any number of operations (hashing, signing, verifying) one after another:
 * every operation starts with `digest_init_ex`, `sign_init` or `verify_init`, which resets the state left by the previous one.
 * It must not be used by two operations at the same time, in particular an operation must not be interrupted by a suspension of a coroutine.
 */
class md_ctx final : public object {
  explicit md_ctx(k2::descriptor descriptor) noexcept
      : object(descriptor) {}

public:
  static auto create() noexcept -> std::expected<md_ctx, int32_t> {
    return k2::openssl_evp_md_ctx_new().transform([](k2::descriptor d) noexcept { return md_ctx{d}; });
  }

  auto digest_init_ex(k2::HashAlgorithm algorithm, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_digest_init_ex(descriptor(), algorithm, openssl_ret);
  }

  auto digest_update(std::span<const std::byte> data, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_digest_update(descriptor(), data, openssl_ret);
  }

  // the size of `md` must be at least the size of the digest, the result is the length of the digest
  auto digest_final_ex(std::span<std::byte> md, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<uint32_t, int32_t> {
    return k2::openssl_evp_digest_final_ex(descriptor(), md, openssl_ret);
  }

  auto sign_init(k2::DigestAlgorithm algorithm, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_sign_init(descriptor(), algorithm, openssl_ret);
  }

  auto sign_update(std::span<const std::byte> data, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_sign_update(descriptor(), data, openssl_ret);
  }

  auto sign_final(std::span<std::byte> sig, const pkey& key, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<uint32_t, int32_t> {
    return k2::openssl_evp_sign_final(descriptor(), sig, key.descriptor(), openssl_ret);
  }

  auto verify_init(k2::DigestAlgorithm algorithm, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_verify_init(descriptor(), algorithm, openssl_ret);
  }

  auto verify_update(std::span<const std::byte> data, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_verify_update(descriptor(), data, openssl_ret);
  }

  auto verify_final(std::span<const std::byte> sig, const pkey& key, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<bool, int32_t> {
    return k2::openssl_evp_verify_final(descriptor(), sig, key.descriptor(), openssl_ret);
  }
};

/**
 * An HMAC context. It can be reused for any number of HMACs one after another: every one starts with `init_ex`.
 * The same restrictions as for `md_ctx` apply.
 */
class hmac_ctx final : public object {
  explicit hmac_ctx(k2::descriptor descriptor) noexcept
      : object(descriptor) {}

public:
  static auto create() noexcept -> std::expected<hmac_ctx, int32_t> {
    return k2::openssl_hmac_ctx_new().transform([](k2::descriptor d) noexcept { return hmac_ctx{d}; });
  }

  // an empty `key` is allowed
  auto init_ex(std::span<const std::byte> key, k2::HashAlgorithm algorithm, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_hmac_init_ex(descriptor(), key, algorithm, openssl_ret);
  }

  auto update(std::span<const std::byte> data, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_hmac_update(descriptor(), data, openssl_ret);
  }

  // the size of `md` must be at least the size of the digest, the result is the length of the HMAC
  auto final(std::span<std::byte> md, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<uint32_t, int32_t> {
    return k2::openssl_hmac_final(descriptor(), md, openssl_ret);
  }
};

class cipher final : public object {
  explicit cipher(k2::descriptor descriptor) noexcept
      : object(descriptor) {}

public:
  // `name` must be NUL-terminated
  static auto by_name(const char* name) noexcept -> std::expected<cipher, int32_t> {
    return k2::openssl_evp_get_cipherbyname(name).transform([](k2::descriptor d) noexcept { return cipher{d}; });
  }

  auto key_length() const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_evp_cipher_key_length(descriptor());
  }

  auto iv_length() const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_evp_cipher_iv_length(descriptor());
  }

  auto block_size() const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_evp_cipher_block_size(descriptor());
  }

  auto mode() const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_evp_cipher_mode(descriptor());
  }
};

class cipher_ctx final : public object {
  explicit cipher_ctx(k2::descriptor descriptor) noexcept
      : object(descriptor) {}

public:
  static auto create() noexcept -> std::expected<cipher_ctx, int32_t> {
    return k2::openssl_evp_cipher_ctx_new().transform([](k2::descriptor d) noexcept { return cipher_ctx{d}; });
  }

  // sets the cipher (and the key and the IV if their `data()` is not null), `enc` is `1` to encrypt and `0` to decrypt
  auto init_ex(const cipher& type, std::span<const std::byte> key, std::span<const std::byte> iv, int32_t enc,
               int32_t* openssl_ret = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_cipher_init_ex(descriptor(), type.descriptor(), key, iv, enc, openssl_ret);
  }

  // sets the key and the IV (if their `data()` is not null) for the cipher that was set earlier, `enc` is `1` to encrypt and `0` to decrypt
  auto init_ex(std::span<const std::byte> key, std::span<const std::byte> iv, int32_t enc,
               int32_t* openssl_ret = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_cipher_init_ex(descriptor(), k2::INVALID_PLATFORM_DESCRIPTOR, key, iv, enc, openssl_ret);
  }

  auto ctrl(k2::CipherCtrl type, int32_t arg, std::span<std::byte> ptr = {}, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_cipher_ctx_ctrl(descriptor(), type, arg, ptr, openssl_ret);
  }

  auto set_iv_length(int32_t iv_len, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<void, int32_t> {
    return ctrl(k2::CipherCtrl::CIPHER_CTRL_AEAD_SET_IVLEN, iv_len, {}, openssl_ret);
  }

  // for CCM when encrypting: sets the length of the tag, which is then retrieved with `get_tag`
  auto set_tag_length(int32_t tag_len, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<void, int32_t> {
    return ctrl(k2::CipherCtrl::CIPHER_CTRL_AEAD_SET_TAG, tag_len, {}, openssl_ret);
  }

  // when decrypting: sets the tag that is checked by `final_ex`
  auto set_tag(std::span<const std::byte> tag, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<void, int32_t> {
    // OpenSSL only reads the tag for `CIPHER_CTRL_AEAD_SET_TAG`
    return ctrl(k2::CipherCtrl::CIPHER_CTRL_AEAD_SET_TAG, static_cast<int32_t>(tag.size()), {const_cast<std::byte*>(tag.data()), tag.size()}, openssl_ret);
  }

  // when encrypting: retrieves the tag after `final_ex`, the size of `tag` is the length of the tag
  auto get_tag(std::span<std::byte> tag, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<void, int32_t> {
    return ctrl(k2::CipherCtrl::CIPHER_CTRL_AEAD_GET_TAG, static_cast<int32_t>(tag.size()), tag, openssl_ret);
  }

  auto set_key_length(int32_t key_len, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_cipher_ctx_set_key_length(descriptor(), key_len, openssl_ret);
  }

  auto set_padding(int32_t pad, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_cipher_ctx_set_padding(descriptor(), pad, openssl_ret);
  }

  auto update(std::span<std::byte> out, std::span<const std::byte> in, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_evp_cipher_update(descriptor(), out, in, openssl_ret);
  }

  // for CCM: sets the length of the data that is then passed to `update`
  auto set_data_length(size_t data_len, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_cipher_set_data_length(descriptor(), data_len, openssl_ret);
  }

  auto final_ex(std::span<std::byte> out, int32_t* openssl_ret = nullptr) const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_evp_cipher_final_ex(descriptor(), out, openssl_ret);
  }
};

/**
 * A name (subject or issuer) of a certificate. It does not depend on the certificate that it was received from.
 * All `std::string_view`s returned by the accessors point to memory owned by the name and are valid only while the name is alive.
 */
class x509_name final : public object {
  explicit x509_name(k2::descriptor descriptor) noexcept
      : object(descriptor) {}

public:
  static auto adopt(k2::descriptor descriptor) noexcept -> x509_name {
    return x509_name{descriptor};
  }

  auto oneline() const noexcept -> std::expected<std::string_view, int32_t> {
    return k2::openssl_x509_name_oneline(descriptor());
  }

  auto entry_count() const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_x509_name_entry_count(descriptor());
  }

  // the NID of the key of the entry
  auto entry_nid(int32_t loc) const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_x509_name_entry_nid(descriptor(), loc);
  }

  // the value of the entry
  auto entry_data(int32_t loc) const noexcept -> std::expected<std::string_view, int32_t> {
    return k2::openssl_x509_name_entry_data(descriptor(), loc);
  }
};

/**
 * A parsed X509 certificate. Keys and names obtained from the certificate do not depend on it and may outlive it.
 */
class x509 final : public object {
  explicit x509(k2::descriptor descriptor) noexcept
      : object(descriptor) {}

public:
  static auto read_pem(std::span<const std::byte> pem) noexcept -> std::expected<x509, int32_t> {
    return k2::openssl_pem_read_bio_x509(pem).transform([](k2::descriptor d) noexcept { return x509{d}; });
  }

  auto get_pubkey() const noexcept -> std::expected<pkey, int32_t> {
    return k2::openssl_x509_get_pubkey(descriptor()).transform(pkey::adopt);
  }

  auto get_subject_name() const noexcept -> std::expected<x509_name, int32_t> {
    return k2::openssl_x509_get_subject_name(descriptor()).transform(x509_name::adopt);
  }

  auto get_issuer_name() const noexcept -> std::expected<x509_name, int32_t> {
    return k2::openssl_x509_get_issuer_name(descriptor()).transform(x509_name::adopt);
  }

  auto subject_name_hash() const noexcept -> std::expected<uint64_t, int32_t> {
    return k2::openssl_x509_subject_name_hash(descriptor());
  }

  auto get_version() const noexcept -> std::expected<int64_t, int32_t> {
    return k2::openssl_x509_get_version(descriptor());
  }

  // `notBefore` as a difference from the Unix epoch
  auto not_before_diff() const noexcept -> std::expected<k2::asn1_time_diff, int32_t> {
    return k2::openssl_x509_not_before_diff(descriptor());
  }

  // `notAfter` as a difference from the Unix epoch
  auto not_after_diff() const noexcept -> std::expected<k2::asn1_time_diff, int32_t> {
    return k2::openssl_x509_not_after_diff(descriptor());
  }

  auto check_purpose(int32_t id, int32_t ca) const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_x509_check_purpose(descriptor(), id, ca);
  }
};

} // namespace kphp::openssl
