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
 */
class descriptor_guard {
  k2::descriptor m_descriptor{k2::INVALID_PLATFORM_DESCRIPTOR};
  bool m_owned{false};

  auto free() noexcept -> void {
    if (m_owned && m_descriptor != k2::INVALID_PLATFORM_DESCRIPTOR) {
      k2::free_descriptor(m_descriptor);
    }

    m_descriptor = k2::INVALID_PLATFORM_DESCRIPTOR;
    m_owned = false;
  }

protected:
  explicit descriptor_guard(k2::descriptor descriptor, bool owned = true) noexcept
      : m_descriptor(descriptor),
        m_owned(owned) {}

  descriptor_guard(const descriptor_guard&) = delete;

  descriptor_guard(descriptor_guard&& other) noexcept
      : m_descriptor(std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR)),
        m_owned(std::exchange(other.m_owned, false)) {}

  auto operator=(const descriptor_guard&) -> descriptor_guard& = delete;

  auto operator=(descriptor_guard&& other) noexcept -> descriptor_guard& {
    if (this != std::addressof(other)) {
      free();
      m_descriptor = std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR);
      m_owned = std::exchange(other.m_owned, false);
    }

    return *this;
  }

  ~descriptor_guard() {
    free();
  }

public:
  auto descriptor() const noexcept -> k2::descriptor {
    return m_descriptor;
  }

  auto release() noexcept -> k2::descriptor {
    m_owned = false;
    return std::exchange(m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR);
  }
};

class rsa final : public descriptor_guard {
  explicit rsa(k2::descriptor descriptor) noexcept
      : descriptor_guard(descriptor) {}

public:
  static auto adopt(k2::descriptor descriptor) noexcept -> rsa {
    return rsa{descriptor};
  }

  auto public_encrypt(std::span<const std::byte> from, std::span<std::byte> to, k2::RsaPadding padding,
                      int32_t* openssl_err = nullptr) const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_rsa_public_encrypt(from, to, descriptor(), padding, openssl_err);
  }

  auto private_decrypt(std::span<const std::byte> from, std::span<std::byte> to, k2::RsaPadding padding,
                       int32_t* openssl_err = nullptr) const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_rsa_private_decrypt(from, to, descriptor(), padding, openssl_err);
  }
};

class pkey final : public descriptor_guard {
  pkey(k2::descriptor descriptor, bool owned) noexcept
      : descriptor_guard(descriptor, owned) {}

public:
  static auto public_from_pem(std::span<const std::byte> pem) noexcept -> std::expected<pkey, int32_t> {
    return k2::openssl_pem_read_bio_pubkey(pem).transform(adopt);
  }

  static auto private_from_pem(std::span<const std::byte> pem, std::span<const std::byte> pass) noexcept -> std::expected<pkey, int32_t> {
    return k2::openssl_pem_read_bio_private_key(pem, pass).transform(adopt);
  }

  static auto adopt(k2::descriptor descriptor) noexcept -> pkey {
    return pkey{descriptor, true};
  }

  static auto borrow(k2::descriptor descriptor) noexcept -> pkey {
    return pkey{descriptor, false};
  }

  auto id() const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_evp_pkey_id(descriptor());
  }

  auto is_rsa() const noexcept -> std::expected<bool, int32_t> {
    return k2::openssl_evp_pkey_is_rsa(descriptor());
  }

  auto size() const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_evp_pkey_size(descriptor());
  }

  auto rsa() const noexcept -> std::expected<rsa, int32_t> {
    return k2::openssl_evp_pkey_get1_rsa(descriptor()).transform(rsa::adopt);
  }
};

class md_ctx final : public descriptor_guard {
  explicit md_ctx(k2::descriptor descriptor) noexcept
      : descriptor_guard(descriptor) {}

public:
  static auto create() noexcept -> std::expected<md_ctx, int32_t> {
    return k2::openssl_evp_md_ctx_new().transform([](k2::descriptor d) noexcept { return md_ctx{d}; });
  }

  auto sign_init(k2::DigestAlgorithm algorithm, int32_t* openssl_err = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_sign_init(descriptor(), algorithm, openssl_err);
  }

  auto sign_update(std::span<const std::byte> data, int32_t* openssl_err = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_sign_update(descriptor(), data, openssl_err);
  }

  auto sign_final(std::span<std::byte> sig, const pkey& key, int32_t* openssl_err = nullptr) const noexcept -> std::expected<uint32_t, int32_t> {
    return k2::openssl_evp_sign_final(descriptor(), sig, key.descriptor(), openssl_err);
  }

  auto verify_init(k2::DigestAlgorithm algorithm, int32_t* openssl_err = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_verify_init(descriptor(), algorithm, openssl_err);
  }

  auto verify_update(std::span<const std::byte> data, int32_t* openssl_err = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_verify_update(descriptor(), data, openssl_err);
  }

  auto verify_final(std::span<const std::byte> sig, const pkey& key, int32_t* openssl_err = nullptr) const noexcept -> std::expected<bool, int32_t> {
    return k2::openssl_evp_verify_final(descriptor(), sig, key.descriptor(), openssl_err);
  }
};

class hmac_ctx final : public descriptor_guard {
  explicit hmac_ctx(k2::descriptor descriptor) noexcept
      : descriptor_guard(descriptor) {}

public:
  static auto create() noexcept -> std::expected<hmac_ctx, int32_t> {
    return k2::openssl_hmac_ctx_new().transform([](k2::descriptor d) noexcept { return hmac_ctx{d}; });
  }

  auto init_ex(std::span<const std::byte> key, k2::HashAlgorithm algorithm, int32_t* openssl_err = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_hmac_init_ex(descriptor(), key, algorithm, openssl_err);
  }

  auto update(std::span<const std::byte> data, int32_t* openssl_err = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_hmac_update(descriptor(), data, openssl_err);
  }

  auto final(std::span<std::byte> md, int32_t* openssl_err = nullptr) const noexcept -> std::expected<uint32_t, int32_t> {
    return k2::openssl_hmac_final(descriptor(), md, openssl_err);
  }
};

class cipher final : public descriptor_guard {
  explicit cipher(k2::descriptor descriptor) noexcept
      : descriptor_guard(descriptor) {}

public:
  static auto from_name(std::string_view name) noexcept -> std::expected<cipher, int32_t> {
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

  auto mode() const noexcept -> std::expected<uint32_t, int32_t> {
    return k2::openssl_evp_cipher_mode(descriptor());
  }
};

class cipher_ctx final : public descriptor_guard {
  explicit cipher_ctx(k2::descriptor descriptor) noexcept
      : descriptor_guard(descriptor) {}

public:
  static auto create() noexcept -> std::expected<cipher_ctx, int32_t> {
    return k2::openssl_evp_cipher_ctx_new().transform([](k2::descriptor d) noexcept { return cipher_ctx{d}; });
  }

  auto init_ex(const cipher& type, std::span<const std::byte> key, std::span<const std::byte> iv, int32_t enc,
               int32_t* openssl_err = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_cipher_init_ex(descriptor(), type.descriptor(), key, iv, enc, openssl_err);
  }

  auto init_ex(std::span<const std::byte> key, std::span<const std::byte> iv, int32_t enc,
               int32_t* openssl_err = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_cipher_init_ex(descriptor(), k2::INVALID_PLATFORM_DESCRIPTOR, key, iv, enc, openssl_err);
  }

  auto set_iv_length(int32_t iv_len, int32_t* openssl_err = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_cipher_ctx_set_iv_len(descriptor(), iv_len, openssl_err);
  }

  auto set_tag_length(int32_t tag_len, int32_t* openssl_err = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_cipher_ctx_set_tag_len(descriptor(), tag_len, openssl_err);
  }

  auto set_tag(std::span<const std::byte> tag, int32_t* openssl_err = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_cipher_ctx_set_tag(descriptor(), tag, openssl_err);
  }

  auto get_tag(std::span<std::byte> tag, int32_t* openssl_err = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_cipher_ctx_get_tag(descriptor(), tag, openssl_err);
  }

  auto set_key_length(int32_t key_len, int32_t* openssl_err = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_cipher_ctx_set_key_length(descriptor(), key_len, openssl_err);
  }

  auto set_padding(int32_t pad) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_cipher_ctx_set_padding(descriptor(), pad);
  }

  auto update(std::span<std::byte> out, std::span<const std::byte> in, int32_t* openssl_err = nullptr) const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_evp_cipher_update(descriptor(), out, in, openssl_err);
  }

  auto set_data_length(int32_t data_len, int32_t* openssl_err = nullptr) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_evp_cipher_ctx_set_ccm_data_len(descriptor(), data_len, openssl_err);
  }

  auto final_ex(std::span<std::byte> out, int32_t* openssl_err = nullptr) const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_evp_cipher_final_ex(descriptor(), out, openssl_err);
  }
};

class cstring final : public descriptor_guard {
  const char* m_str{nullptr};

  cstring(k2::descriptor descriptor, const char* str) noexcept
      : descriptor_guard(descriptor),
        m_str(str) {}

public:
  static auto adopt(k2::cstring oneline) noexcept -> openssl::cstring {
    return openssl::cstring{oneline.descriptor, oneline.str};
  }

  auto c_str() const noexcept -> const char* {
    return m_str;
  }
};

class asn1_object final : public descriptor_guard {
  explicit asn1_object(k2::descriptor descriptor) noexcept
      : descriptor_guard(descriptor) {}

public:
  static auto adopt(k2::descriptor descriptor) noexcept -> asn1_object {
    return asn1_object{descriptor};
  }

  auto obj2nid() const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_obj_obj2nid(descriptor());
  }
};

class asn1_string final : public descriptor_guard {
  explicit asn1_string(k2::descriptor descriptor) noexcept
      : descriptor_guard(descriptor) {}

public:
  static auto adopt(k2::descriptor descriptor) noexcept -> asn1_string {
    return asn1_string{descriptor};
  }

  auto data() const noexcept -> std::expected<const void*, int32_t> {
    return k2::openssl_asn1_string_get0_data(descriptor());
  }

  auto length() const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_asn1_string_length(descriptor());
  }
};

class asn1_time final : public descriptor_guard {
  explicit asn1_time(k2::descriptor descriptor) noexcept
      : descriptor_guard(descriptor) {}

public:
  static auto adopt(k2::descriptor descriptor) noexcept -> asn1_time {
    return asn1_time{descriptor};
  }

  static auto create() noexcept -> std::expected<asn1_time, int32_t> {
    return k2::openssl_asn1_time_new().transform(adopt);
  }

  auto set(int64_t t) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_asn1_time_set(descriptor(), t);
  }

  auto diff(const asn1_time& to, int32_t* openssl_err = nullptr) const noexcept -> std::expected<k2::asn1_time_diff, int32_t> {
    return k2::openssl_asn1_time_diff(descriptor(), to.descriptor(), openssl_err);
  }

  auto diff_from_now(int32_t* openssl_err = nullptr) const noexcept -> std::expected<k2::asn1_time_diff, int32_t> {
    return k2::openssl_asn1_time_diff_from_now(descriptor(), openssl_err);
  }

  auto diff_to_now(int32_t* openssl_err = nullptr) const noexcept -> std::expected<k2::asn1_time_diff, int32_t> {
    return k2::openssl_asn1_time_diff_to_now(descriptor(), openssl_err);
  }
};

class x509_name_entry final : public descriptor_guard {
  explicit x509_name_entry(k2::descriptor descriptor) noexcept
      : descriptor_guard(descriptor) {}

public:
  static auto adopt(k2::descriptor descriptor) noexcept -> x509_name_entry {
    return x509_name_entry{descriptor};
  }

  auto object() const noexcept -> std::expected<asn1_object, int32_t> {
    return k2::openssl_x509_name_entry_get_object(descriptor()).transform(asn1_object::adopt);
  }

  auto data() const noexcept -> std::expected<asn1_string, int32_t> {
    return k2::openssl_x509_name_entry_get_data(descriptor()).transform(asn1_string::adopt);
  }
};

class x509_name final : public descriptor_guard {
  explicit x509_name(k2::descriptor descriptor) noexcept
      : descriptor_guard(descriptor) {}

public:
  static auto adopt(k2::descriptor descriptor) noexcept -> x509_name {
    return x509_name{descriptor};
  }

  auto oneline() const noexcept -> std::expected<openssl::cstring, int32_t> {
    return k2::openssl_x509_name_oneline(descriptor()).transform(openssl::cstring::adopt);
  }

  auto entry_count() const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_x509_name_entry_count(descriptor());
  }

  auto entry(int32_t loc) const noexcept -> std::expected<x509_name_entry, int32_t> {
    return k2::openssl_x509_name_get_entry(descriptor(), loc).transform(x509_name_entry::adopt);
  }
};

class x509_purpose final : public descriptor_guard {
  explicit x509_purpose(k2::descriptor descriptor) noexcept
      : descriptor_guard(descriptor) {}

public:
  static auto adopt(k2::descriptor descriptor) noexcept -> x509_purpose {
    return x509_purpose{descriptor};
  }

  static auto count() noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_x509_purpose_get_count();
  }

  static auto get(int32_t idx) noexcept -> std::expected<x509_purpose, int32_t> {
    return k2::openssl_x509_purpose_get0(idx).transform(adopt);
  }

  auto id() const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_x509_purpose_get_id(descriptor());
  }

  auto name() const noexcept -> std::expected<const char*, int32_t> {
    return k2::openssl_x509_purpose_get0_name(descriptor());
  }

  auto sname() const noexcept -> std::expected<const char*, int32_t> {
    return k2::openssl_x509_purpose_get0_sname(descriptor());
  }
};

class x509 final : public descriptor_guard {
  explicit x509(k2::descriptor descriptor) noexcept
      : descriptor_guard(descriptor) {}

public:
  static auto from_pem(std::span<const std::byte> pem) noexcept -> std::expected<x509, int32_t> {
    return k2::openssl_pem_read_bio_x509(pem).transform([](k2::descriptor d) noexcept { return x509{d}; });
  }

  auto pubkey() const noexcept -> std::expected<pkey, int32_t> {
    return k2::openssl_x509_get_pubkey(descriptor()).transform(pkey::adopt);
  }

  auto subject_name() const noexcept -> std::expected<x509_name, int32_t> {
    return k2::openssl_x509_get_subject_name(descriptor()).transform(x509_name::adopt);
  }

  auto issuer_name() const noexcept -> std::expected<x509_name, int32_t> {
    return k2::openssl_x509_get_issuer_name(descriptor()).transform(x509_name::adopt);
  }

  auto subject_name_hash() const noexcept -> std::expected<uint64_t, int32_t> {
    return k2::openssl_x509_subject_name_hash(descriptor());
  }

  auto version() const noexcept -> std::expected<int64_t, int32_t> {
    return k2::openssl_x509_get_version(descriptor());
  }

  auto not_before() const noexcept -> std::expected<asn1_time, int32_t> {
    return k2::openssl_x509_getm_not_before(descriptor()).transform(asn1_time::adopt);
  }

  auto not_after() const noexcept -> std::expected<asn1_time, int32_t> {
    return k2::openssl_x509_getm_not_after(descriptor()).transform(asn1_time::adopt);
  }

  auto check_purpose(int32_t id, int32_t ca) const noexcept -> std::expected<int32_t, int32_t> {
    return k2::openssl_x509_check_purpose(descriptor(), id, ca);
  }
};

} // namespace kphp::openssl
