// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <expected>
#include <functional>
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
struct image_state final : private vk::not_copyable {
  string NAME_STR{NAME.data(), static_cast<string::size_type>(NAME.size())};
  string SUBJECT_STR{SUBJECT.data(), static_cast<string::size_type>(SUBJECT.size())};
  string HASH_STR{HASH.data(), static_cast<string::size_type>(HASH.size())};
  string ISSUER_STR{ISSUER.data(), static_cast<string::size_type>(ISSUER.size())};
  string VERSION_STR{VERSION.data(), static_cast<string::size_type>(VERSION.size())};
  string VALID_FROM_TIME_T_STR{VALID_FROM_TIME_T.data(), static_cast<string::size_type>(VALID_FROM_TIME_T.size())};
  string VALID_TO_TIME_T_STR{VALID_TO_TIME_T.data(), static_cast<string::size_type>(VALID_TO_TIME_T.size())};
  string PURPOSES_STR{PURPOSES.data(), static_cast<string::size_type>(PURPOSES.size())};

  image_state() noexcept {
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

  static const image_state& get() noexcept;

private:
  static constexpr std::string_view NAME{"name"};
  static constexpr std::string_view SUBJECT{"subject"};
  static constexpr std::string_view HASH{"hash"};
  static constexpr std::string_view ISSUER{"issuer"};
  static constexpr std::string_view VERSION{"version"};
  static constexpr std::string_view VALID_FROM_TIME_T{"validFrom_time_t"};
  static constexpr std::string_view VALID_TO_TIME_T{"validTo_time_t"};
  static constexpr std::string_view PURPOSES{"purposes"};
};

class instance_state final : private vk::not_copyable {
  // The contexts that are reused by all signatures, verifications, HMACs and ciphers of the instance, so that a context is not created for every call.
  // OpenSSL allows to start a new operation on a context after the previous one. A context is created on the first use and freed together with the instance.
  std::optional<kphp::openssl::md_ctx> m_md_ctx;
  std::optional<kphp::openssl::hmac_ctx> m_hmac_ctx;
  std::optional<kphp::openssl::cipher_ctx> m_cipher_ctx;

public:
  instance_state() noexcept = default;

  static instance_state& get() noexcept;

  auto md_ctx() noexcept -> std::expected<std::reference_wrapper<const kphp::openssl::md_ctx>, int32_t> {
    if (!m_md_ctx) {
      auto created{kphp::openssl::md_ctx::create()};
      if (!created) [[unlikely]] {
        return std::unexpected{created.error()};
      }

      m_md_ctx.emplace(std::move(*created));
    }

    return std::cref(*m_md_ctx);
  }

  auto hmac_ctx() noexcept -> std::expected<std::reference_wrapper<const kphp::openssl::hmac_ctx>, int32_t> {
    if (!m_hmac_ctx) {
      auto created{kphp::openssl::hmac_ctx::create()};
      if (!created) [[unlikely]] {
        return std::unexpected{created.error()};
      }

      m_hmac_ctx.emplace(std::move(*created));
    }

    return std::cref(*m_hmac_ctx);
  }

  auto cipher_ctx() noexcept -> std::expected<std::reference_wrapper<const kphp::openssl::cipher_ctx>, int32_t> {
    if (!m_cipher_ctx) {
      auto created{kphp::openssl::cipher_ctx::create()};
      if (!created) [[unlikely]] {
        return std::unexpected{created.error()};
      }

      m_cipher_ctx.emplace(std::move(*created));
    }

    return std::cref(*m_cipher_ctx);
  }
};

} // namespace kphp::openssl
