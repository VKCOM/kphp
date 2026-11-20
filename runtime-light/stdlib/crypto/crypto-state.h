// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "common/php-functions.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/core/reference-counter/reference-counter-functions.h"
#include "runtime-light/stdlib/crypto/openssl.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::openssl {
struct crypto_image_state final : private vk::not_copyable {
  string NAME_STR{NAME_.data(), static_cast<string::size_type>(NAME_.size())};
  string SUBJECT_STR{SUBJECT_.data(), static_cast<string::size_type>(SUBJECT_.size())};
  string HASH_STR{HASH_.data(), static_cast<string::size_type>(HASH_.size())};
  string ISSUER_STR{ISSUER_.data(), static_cast<string::size_type>(ISSUER_.size())};
  string VERSION_STR{VERSION_.data(), static_cast<string::size_type>(VERSION_.size())};
  string VALID_FROM_TIME_T_STR{VALID_FROM_TIME_T_.data(), static_cast<string::size_type>(VALID_FROM_TIME_T_.size())};
  string VALID_TO_TIME_T_STR{VALID_TO_TIME_T_.data(), static_cast<string::size_type>(VALID_TO_TIME_T_.size())};
  string PURPOSES_STR{PURPOSES_.data(), static_cast<string::size_type>(PURPOSES_.size())};

  crypto_image_state() noexcept {
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(NAME_STR, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(NAME_STR, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(SUBJECT_STR, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(SUBJECT_STR, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(HASH_STR, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(HASH_STR, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(ISSUER_STR, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(ISSUER_STR, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(VERSION_STR, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(VERSION_STR, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(VALID_FROM_TIME_T_STR, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(VALID_FROM_TIME_T_STR, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(VALID_TO_TIME_T_STR, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(VALID_TO_TIME_T_STR, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(PURPOSES_STR, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(PURPOSES_STR, ExtraRefCnt::for_global_const)));
  }

  static const crypto_image_state& get() noexcept;

private:
  static constexpr std::string_view NAME_{"name"};
  static constexpr std::string_view SUBJECT_{"subject"};
  static constexpr std::string_view HASH_{"hash"};
  static constexpr std::string_view ISSUER_{"issuer"};
  static constexpr std::string_view VERSION_{"version"};
  static constexpr std::string_view VALID_FROM_TIME_T_{"validFrom_time_t"};
  static constexpr std::string_view VALID_TO_TIME_T_{"validTo_time_t"};
  static constexpr std::string_view PURPOSES_{"purposes"};
};

class crypto_instance_state final : private vk::not_copyable {
  // The contexts that are reused by all signatures, verifications, HMACs and ciphers of the instance, so that a context is not created for every call.
  // OpenSSL allows to start a new operation on a context after the previous one. A context is created on the first use and freed together with the instance.
  std::optional<kphp::openssl::md_ctx> m_md_ctx;
  std::optional<kphp::openssl::hmac_ctx> m_hmac_ctx;
  std::optional<kphp::openssl::cipher_ctx> m_cipher_ctx;

public:
  crypto_instance_state() noexcept = default;

  static crypto_instance_state& get() noexcept;

  auto md_ctx() noexcept -> std::expected<const kphp::openssl::md_ctx*, int32_t> {
    if (!m_md_ctx) {
      auto created{kphp::openssl::md_ctx::create()};
      if (!created) [[unlikely]] {
        return std::unexpected{created.error()};
      }
      m_md_ctx.emplace(std::move(*created));
    }

    return std::addressof(*m_md_ctx);
  }

  auto hmac_ctx() noexcept -> std::expected<const kphp::openssl::hmac_ctx*, int32_t> {
    if (!m_hmac_ctx) {
      auto created{kphp::openssl::hmac_ctx::create()};
      if (!created) [[unlikely]] {
        return std::unexpected{created.error()};
      }
      m_hmac_ctx.emplace(std::move(*created));
    }

    return std::addressof(*m_hmac_ctx);
  }

  auto cipher_ctx() noexcept -> std::expected<const kphp::openssl::cipher_ctx*, int32_t> {
    if (!m_cipher_ctx) {
      auto created{kphp::openssl::cipher_ctx::create()};
      if (!created) [[unlikely]] {
        return std::unexpected{created.error()};
      }
      m_cipher_ctx.emplace(std::move(*created));
    }

    return std::addressof(*m_cipher_ctx);
  }
};

} // namespace kphp::openssl
