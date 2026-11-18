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
 * A private or public key.
 *
 * An owning key frees its platform descriptor on destruction. A borrowing key (see `pkey::borrow`) refers to a descriptor owned by someone else
 * and never frees it.
 */
class pkey final {
  k2::descriptor m_descriptor{k2::INVALID_PLATFORM_DESCRIPTOR};
  bool m_owned{false};

  pkey(k2::descriptor descriptor, bool owned) noexcept
      : m_descriptor(descriptor),
        m_owned(owned) {}

  auto reset() noexcept -> void {
    if (m_owned && m_descriptor != k2::INVALID_PLATFORM_DESCRIPTOR) {
      k2::free_descriptor(m_descriptor);
    }

    m_descriptor = k2::INVALID_PLATFORM_DESCRIPTOR;
    m_owned = false;
  }

public:
  pkey(const pkey&) = delete;

  pkey(pkey&& other) noexcept
      : m_descriptor(std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR)),
        m_owned(std::exchange(other.m_owned, false)) {}

  auto operator=(const pkey&) -> pkey& = delete;

  auto operator=(pkey&& other) noexcept -> pkey& {
    if (this != std::addressof(other)) {
      reset();
      m_descriptor = std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR);
      m_owned = std::exchange(other.m_owned, false);
    }

    return *this;
  }

  ~pkey() {
    reset();
  }

  static auto get_public(std::span<const std::byte> pem) noexcept -> std::expected<pkey, int32_t> {
    return k2::openssl_pkey_get_public(pem).transform([](k2::descriptor d) noexcept { return pkey{d, true}; });
  }

  static auto get_private(std::span<const std::byte> pem, std::span<const std::byte> passphrase) noexcept -> std::expected<pkey, int32_t> {
    return k2::openssl_pkey_get_private(pem, passphrase).transform([](k2::descriptor d) noexcept { return pkey{d, true}; });
  }

  // takes ownership of the descriptor
  static auto adopt(k2::descriptor descriptor) noexcept -> pkey {
    return pkey{descriptor, true};
  }

  // does not take ownership of the descriptor, the caller guarantees that it outlives the key
  static auto borrow(k2::descriptor descriptor) noexcept -> pkey {
    return pkey{descriptor, false};
  }

  auto descriptor() const noexcept -> k2::descriptor {
    return m_descriptor;
  }

  // gives up the ownership and returns the descriptor, it is not freed by this key anymore
  auto release() noexcept -> k2::descriptor {
    m_owned = false;
    return std::exchange(m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR);
  }

  auto size() const noexcept -> std::expected<size_t, int32_t> {
    return k2::openssl_pkey_size(m_descriptor);
  }

  auto is_public() const noexcept -> std::expected<bool, int32_t> {
    return k2::openssl_pkey_is_public(m_descriptor);
  }

  auto is_private() const noexcept -> std::expected<bool, int32_t> {
    return k2::openssl_pkey_is_private(m_descriptor);
  }

  auto sign(k2::DigestAlgorithm algorithm, std::span<const std::byte> data, std::span<std::byte> result) const noexcept -> std::expected<size_t, int32_t> {
    return k2::openssl_sign(algorithm, data, m_descriptor, result);
  }

  auto verify(k2::DigestAlgorithm algorithm, std::span<const std::byte> data,
              std::span<const std::byte> signature) const noexcept -> std::expected<bool, int32_t> {
    return k2::openssl_verify(algorithm, data, m_descriptor, signature);
  }

  auto public_encrypt(std::span<const std::byte> data, k2::RsaPadding padding, std::span<std::byte> result) const noexcept -> std::expected<size_t, int32_t> {
    return k2::openssl_public_encrypt(data, m_descriptor, padding, result);
  }

  auto private_decrypt(std::span<const std::byte> data, k2::RsaPadding padding, std::span<std::byte> result) const noexcept -> std::expected<size_t, int32_t> {
    return k2::openssl_private_decrypt(data, m_descriptor, padding, result);
  }
};

/**
 * A parsed X509 certificate that frees its platform descriptor on destruction.
 *
 * All `std::string_view`s returned by the accessors point to memory owned by the certificate and are valid only while the certificate is alive.
 * Keys obtained from the certificate do not depend on it and may outlive it.
 */
class x509 final {
  k2::descriptor m_descriptor{k2::INVALID_PLATFORM_DESCRIPTOR};

  explicit x509(k2::descriptor descriptor) noexcept
      : m_descriptor(descriptor) {}

  auto reset() noexcept -> void {
    if (m_descriptor != k2::INVALID_PLATFORM_DESCRIPTOR) {
      k2::free_descriptor(std::exchange(m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR));
    }
  }

public:
  x509(x509&& other) noexcept
      : m_descriptor(std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR)) {}

  auto operator=(x509&& other) noexcept -> x509& {
    if (this != std::addressof(other)) {
      reset();
      m_descriptor = std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR);
    }

    return *this;
  }

  ~x509() {
    reset();
  }

  x509(const x509&) = delete;
  auto operator=(const x509&) -> x509& = delete;

  static auto parse(std::span<const std::byte> pem) noexcept -> std::expected<x509, int32_t> {
    return k2::openssl_x509_parse(pem).transform([](k2::descriptor d) noexcept { return x509{d}; });
  }

  auto pubkey() const noexcept -> std::expected<pkey, int32_t> {
    return k2::openssl_x509_get_pubkey(m_descriptor).transform(pkey::adopt);
  }

  auto subject_name() const noexcept -> std::expected<std::string_view, int32_t> {
    return k2::openssl_x509_get_subject_name(m_descriptor);
  }

  auto name_entry_count(bool is_issuer) const noexcept -> std::expected<size_t, int32_t> {
    return k2::openssl_x509_name_entry_count(m_descriptor, is_issuer);
  }

  auto name_entry_key(bool is_issuer, bool shortnames, size_t index) const noexcept -> std::expected<std::string_view, int32_t> {
    return k2::openssl_x509_name_entry_key(m_descriptor, is_issuer, shortnames, index);
  }

  auto name_entry_value(bool is_issuer, size_t index) const noexcept -> std::expected<std::string_view, int32_t> {
    return k2::openssl_x509_name_entry_value(m_descriptor, is_issuer, index);
  }

  auto hash(std::span<std::byte> result) const noexcept -> std::expected<void, int32_t> {
    return k2::openssl_x509_get_hash(m_descriptor, result);
  }

  auto version() const noexcept -> std::expected<int64_t, int32_t> {
    return k2::openssl_x509_get_version(m_descriptor);
  }

  auto valid_from() const noexcept -> std::expected<int64_t, int32_t> {
    return k2::openssl_x509_get_valid_from(m_descriptor);
  }

  auto valid_to() const noexcept -> std::expected<int64_t, int32_t> {
    return k2::openssl_x509_get_valid_to(m_descriptor);
  }

  auto purpose_check(size_t index, bool ca) const noexcept -> std::expected<bool, int32_t> {
    return k2::openssl_x509_purpose_check(m_descriptor, index, ca);
  }
};

} // namespace kphp::openssl
