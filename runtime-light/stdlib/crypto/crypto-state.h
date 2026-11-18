// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string_view>

#include "common/mixin/not_copyable.h"
#include "common/php-functions.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/core/reference-counter/reference-counter-functions.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

struct CryptoImageState final : private vk::not_copyable {
  string NAME_STR{NAME_.data(), static_cast<string::size_type>(NAME_.size())};
  string SUBJECT_STR{SUBJECT_.data(), static_cast<string::size_type>(SUBJECT_.size())};
  string HASH_STR{HASH_.data(), static_cast<string::size_type>(HASH_.size())};
  string ISSUER_STR{ISSUER_.data(), static_cast<string::size_type>(ISSUER_.size())};
  string VERSION_STR{VERSION_.data(), static_cast<string::size_type>(VERSION_.size())};
  string VALID_FROM_TIME_T_STR{VALID_FROM_TIME_T_.data(), static_cast<string::size_type>(VALID_FROM_TIME_T_.size())};
  string VALID_TO_TIME_T_STR{VALID_TO_TIME_T_.data(), static_cast<string::size_type>(VALID_TO_TIME_T_.size())};
  string PURPOSES_STR{PURPOSES_.data(), static_cast<string::size_type>(PURPOSES_.size())};

  CryptoImageState() noexcept {
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

  static const CryptoImageState& get() noexcept;

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
