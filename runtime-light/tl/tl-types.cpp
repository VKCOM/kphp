// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/tl/tl-types.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/tl/tl-core.h"

namespace tl {

bool string::fetch(tl::fetcher& tlf) noexcept {
  uint8_t first_byte{};
  if (const auto opt_first_byte{tlf.fetch_trivial<uint8_t>()}; opt_first_byte) [[likely]] {
    first_byte = *opt_first_byte;
  } else {
    return false;
  }

  uint8_t size_len{};
  uint64_t string_len{};
  switch (first_byte) {
  case HUGE_STRING_MAGIC: {
    if (tlf.remaining() < HUGE_STRING_SIZE_LEN) [[unlikely]] {
      return false;
    }
    size_len = HUGE_STRING_SIZE_LEN + 1;
    auto len_bytes{*tlf.fetch_bytes(HUGE_STRING_SIZE_LEN)};
    string_len = static_cast<uint64_t>(len_bytes[0]) | (static_cast<uint64_t>(len_bytes[1]) << 8) | (static_cast<uint64_t>(len_bytes[2]) << 16) |
                 (static_cast<uint64_t>(len_bytes[3]) << 24) | (static_cast<uint64_t>(len_bytes[4]) << 32) | (static_cast<uint64_t>(len_bytes[5]) << 40) |
                 (static_cast<uint64_t>(len_bytes[6]) << 48);

    if (string_len <= MEDIUM_STRING_MAX_LEN) [[unlikely]] {
      kphp::log::warning("large string's length is less than (1 << 24) - 1 (length = {})", string_len);
      return false;
    }
    break;
  }
  case MEDIUM_STRING_MAGIC: {
    if (tlf.remaining() < MEDIUM_STRING_SIZE_LEN) [[unlikely]] {
      return false;
    }
    size_len = MEDIUM_STRING_SIZE_LEN + 1;
    auto len_bytes{*tlf.fetch_bytes(MEDIUM_STRING_SIZE_LEN)};
    string_len = static_cast<uint64_t>(len_bytes[0]) | (static_cast<uint64_t>(len_bytes[1]) << 8) | (static_cast<uint64_t>(len_bytes[2]) << 16);

    if (string_len <= TINY_STRING_MAX_LEN) [[unlikely]] {
      kphp::log::warning("long string's length is less than 254 (length = {})", string_len);
      return false;
    }
    break;
  }
  default: {
    size_len = TINY_STRING_SIZE_LEN;
    string_len = static_cast<uint64_t>(first_byte);
    break;
  }
  }
  // Alignment on 4 is required
  const auto total_len_with_padding{(size_len + string_len + 3) & ~static_cast<uint64_t>(3)};

  if (auto required{total_len_with_padding - size_len}; required > tlf.remaining()) [[unlikely]] {
    kphp::log::warning("not enough space in buffer for string (length = {}) fetching, required {} bytes, remain {} bytes", string_len, required,
                       tlf.remaining());
    return false;
  }

  value = {reinterpret_cast<const char*>(std::next(tlf.view().data(), tlf.pos())), static_cast<size_t>(string_len)};
  tlf.adjust(total_len_with_padding - size_len);
  return true;
}

void string::store(tl::storer& tls) const noexcept {
  const char* str_buf{value.data()};
  size_t str_len{value.size()};
  uint8_t size_len{};
  if (str_len <= TINY_STRING_MAX_LEN) {
    size_len = TINY_STRING_SIZE_LEN;
    tls.store_trivial<uint8_t>(str_len);
  } else if (str_len <= MEDIUM_STRING_MAX_LEN) {
    size_len = MEDIUM_STRING_SIZE_LEN + 1;
    tls.store_trivial<uint8_t>(MEDIUM_STRING_MAGIC);
    std::array<std::byte, MEDIUM_STRING_SIZE_LEN> len_bytes{static_cast<std::byte>(str_len & 0xff), static_cast<std::byte>((str_len >> 8) & 0xff),
                                                            static_cast<std::byte>((str_len >> 16) & 0xff)};
    tls.store_bytes(len_bytes);
  } else if (str_len <= HUGE_STRING_MAX_LEN) {
    size_len = HUGE_STRING_SIZE_LEN + 1;
    tls.store_trivial<uint8_t>(HUGE_STRING_MAGIC);
    std::array<std::byte, HUGE_STRING_SIZE_LEN> len_bytes{static_cast<std::byte>(str_len & 0xff),         static_cast<std::byte>((str_len >> 8) & 0xff),
                                                          static_cast<std::byte>((str_len >> 16) & 0xff), static_cast<std::byte>((str_len >> 24) & 0xff),
                                                          static_cast<std::byte>((str_len >> 32) & 0xff), static_cast<std::byte>((str_len >> 40) & 0xff),
                                                          static_cast<std::byte>((str_len >> 48) & 0xff)};
    tls.store_bytes(len_bytes);
  } else {
    kphp::log::error("string length exceeds maximum allowed length: max allowed -> {}, actual -> {}", HUGE_STRING_MAX_LEN, str_len);
  }

  tls.store_bytes({reinterpret_cast<const std::byte*>(str_buf), str_len});

  const auto total_len{size_len + str_len};
  const auto total_len_with_padding{(total_len + 3) & ~3};
  const auto padding{total_len_with_padding - total_len};

  constexpr std::array padding_array{'\0', '\0', '\0', '\0'};
  tls.store_bytes({reinterpret_cast<const std::byte*>(padding_array.data()), padding});
}

bool CertInfoItem::fetch(tl::fetcher& tlf) noexcept {
  tl::magic magic{};
  if (!magic.fetch(tlf)) [[unlikely]] {
    return false;
  }

  switch (magic.value) {
  case Magic::LONG: {
    if (tl::i64 val{}; val.fetch(tlf)) [[likely]] {
      data = val;
      break;
    }
    return false;
  }
  case Magic::STR: {
    if (tl::string val{}; val.fetch(tlf)) [[likely]] {
      data = val;
      break;
    }
    return false;
  }
  case Magic::DICT: {
    if (tl::dictionary<tl::string> val{}; val.fetch(tlf)) [[likely]] {
      data = std::move(val);
      break;
    }
    return false;
  }
  }

  return true;
}

// ===== RPC =====

bool rpcInvokeReqExtra::fetch(tl::fetcher& tlf, const tl::mask& flags) noexcept {
  bool ok{true};
  if (ok && static_cast<bool>(flags.value & WAIT_BINLOG_POS_FLAG)) {
    ok &= opt_wait_binlog_pos.emplace().fetch(tlf);
  }
  if (ok && static_cast<bool>(flags.value & STRING_FORWARD_KEYS_FLAG)) {
    ok &= opt_string_forward_keys.emplace().fetch(tlf);
  }
  if (ok && static_cast<bool>(flags.value & INT_FORWARD_KEYS_FLAG)) {
    ok &= opt_int_forward_keys.emplace().fetch(tlf);
  }
  if (ok && static_cast<bool>(flags.value & STRING_FORWARD_FLAG)) {
    ok &= opt_string_forward.emplace().fetch(tlf);
  }
  if (ok && static_cast<bool>(flags.value & INT_FORWARD_FLAG)) {
    ok &= opt_int_forward.emplace().fetch(tlf);
  }
  if (ok && static_cast<bool>(flags.value & CUSTOM_TIMEOUT_MS_FLAG)) {
    ok &= opt_custom_timeout_ms.emplace().fetch(tlf);
  }
  if (ok && static_cast<bool>(flags.value & SUPPORTED_COMPRESSION_VERSION_FLAG)) {
    ok &= opt_supported_compression_version.emplace().fetch(tlf);
  }
  if (ok && static_cast<bool>(flags.value & RANDOM_DELAY_FLAG)) {
    ok &= opt_random_delay.emplace().fetch(tlf);
  }
  if (ok && static_cast<bool>(flags.value & PERSISTENT_QUERY_FLAG)) {
    ok &= opt_persistent_query.emplace().fetch(tlf);
  }
  if (ok && static_cast<bool>(flags.value & TRACE_CONTEXT_FLAG)) {
    ok &= opt_trace_context.emplace().fetch(tlf);
  }
  if (ok && static_cast<bool>(flags.value & EXECUTION_CONTEXT_FLAG)) {
    ok &= opt_execution_context.emplace().fetch(tlf);
  }

  return_binlog_pos = static_cast<bool>(flags.value & RETURN_BINLOG_POS_FLAG);
  return_binlog_time = static_cast<bool>(flags.value & RETURN_BINLOG_TIME_FLAG);
  return_pid = static_cast<bool>(flags.value & RETURN_PID_FLAG);
  return_request_sizes = static_cast<bool>(flags.value & RETURN_REQUEST_SIZES_FLAG);
  return_failed_subqueries = static_cast<bool>(flags.value & RETURN_FAILED_SUBQUERIES_FLAG);
  return_query_stats = static_cast<bool>(flags.value & RETURN_QUERY_STATS_FLAG);
  no_result = static_cast<bool>(flags.value & NORESULT_FLAG);
  return_view_number = static_cast<bool>(flags.value & RETURN_VIEW_NUMBER_FLAG);
  return ok;
}

tl::mask rpcInvokeReqExtra::get_flags() const noexcept {
  tl::mask flags{.value = static_cast<tl::mask::underlying_type>(return_binlog_pos)};

  flags.value |= static_cast<tl::mask::underlying_type>(return_binlog_time) << 1U;
  flags.value |= static_cast<tl::mask::underlying_type>(return_pid) << 2U;
  flags.value |= static_cast<tl::mask::underlying_type>(return_request_sizes) << 3U;
  flags.value |= static_cast<tl::mask::underlying_type>(return_failed_subqueries) << 4U;
  flags.value |= static_cast<tl::mask::underlying_type>(return_query_stats) << 6U;
  flags.value |= static_cast<tl::mask::underlying_type>(no_result) << 7U;
  flags.value |= static_cast<tl::mask::underlying_type>(return_view_number) << 27U;

  flags.value |= static_cast<tl::mask::underlying_type>(opt_wait_binlog_pos.has_value()) << 16U;
  flags.value |= static_cast<tl::mask::underlying_type>(opt_string_forward_keys.has_value()) << 18U;
  flags.value |= static_cast<tl::mask::underlying_type>(opt_int_forward_keys.has_value()) << 19U;
  flags.value |= static_cast<tl::mask::underlying_type>(opt_string_forward.has_value()) << 20U;
  flags.value |= static_cast<tl::mask::underlying_type>(opt_int_forward.has_value()) << 21U;
  flags.value |= static_cast<tl::mask::underlying_type>(opt_custom_timeout_ms.has_value()) << 23U;
  flags.value |= static_cast<tl::mask::underlying_type>(opt_supported_compression_version.has_value()) << 25U;
  flags.value |= static_cast<tl::mask::underlying_type>(opt_random_delay.has_value()) << 26U;
  flags.value |= static_cast<tl::mask::underlying_type>(opt_persistent_query.has_value()) << 28U;
  flags.value |= static_cast<tl::mask::underlying_type>(opt_trace_context.has_value()) << 29U;
  flags.value |= static_cast<tl::mask::underlying_type>(opt_execution_context.has_value()) << 30U;
  return flags;
}

bool rpcReqResultExtra::fetch(tl::fetcher& tlf, const tl::mask& flags) noexcept {
  bool ok{true};
  if (ok && static_cast<bool>(flags.value & BINLOG_POS_FLAG)) {
    ok = opt_binlog_pos.emplace().fetch(tlf);
  }
  if (ok && static_cast<bool>(flags.value & BINLOG_TIME_FLAG)) {
    ok = opt_binlog_time.emplace().fetch(tlf);
  }
  if (ok && static_cast<bool>(flags.value & ENGINE_PID_FLAG)) {
    ok = opt_engine_pid.emplace().fetch(tlf);
  }
  if (ok && static_cast<bool>(flags.value & REQUEST_SIZE_FLAG)) {
    kphp::log::assertion(static_cast<bool>(flags.value & RESPONSE_SIZE_FLAG));
    ok = opt_request_size.emplace().fetch(tlf) && opt_response_size.emplace().fetch(tlf);
  }
  if (ok && static_cast<bool>(flags.value & FAILED_SUBQUERIES_FLAG)) {
    ok = opt_failed_subqueries.emplace().fetch(tlf);
  }
  if (ok && static_cast<bool>(flags.value & COMPRESSION_VERSION_FLAG)) {
    ok = opt_compression_version.emplace().fetch(tlf);
  }
  if (ok && static_cast<bool>(flags.value & STATS_FLAG)) {
    ok = opt_stats.emplace().fetch(tlf);
  }
  if (ok && static_cast<bool>(flags.value & EPOCH_NUMBER_FLAG)) {
    kphp::log::assertion(static_cast<bool>(flags.value & VIEW_NUMBER_FLAG));
    ok = opt_epoch_number.emplace().fetch(tlf) && opt_view_number.emplace().fetch(tlf);
  }
  return ok;
}

void rpcReqResultExtra::store(tl::storer& tls, const tl::mask& flags) const noexcept {
  if (static_cast<bool>(flags.value & BINLOG_POS_FLAG)) {
    kphp::log::assertion(opt_binlog_pos.has_value());
    opt_binlog_pos->store(tls);
  }
  if (static_cast<bool>(flags.value & BINLOG_TIME_FLAG)) {
    kphp::log::assertion(opt_binlog_time.has_value());
    opt_binlog_time->store(tls);
  }
  if (static_cast<bool>(flags.value & ENGINE_PID_FLAG)) {
    kphp::log::assertion(opt_engine_pid.has_value());
    opt_engine_pid->store(tls);
  }
  if (static_cast<bool>(flags.value & REQUEST_SIZE_FLAG)) {
    kphp::log::assertion(opt_request_size.has_value() && static_cast<bool>(flags.value & RESPONSE_SIZE_FLAG) && opt_response_size.has_value());
    opt_request_size->store(tls), opt_response_size->store(tls);
  }
  if (static_cast<bool>(flags.value & FAILED_SUBQUERIES_FLAG)) {
    kphp::log::assertion(opt_failed_subqueries.has_value());
    opt_failed_subqueries->store(tls);
  }
  if (static_cast<bool>(flags.value & COMPRESSION_VERSION_FLAG)) {
    kphp::log::assertion(opt_compression_version.has_value());
    opt_compression_version->store(tls);
  }
  if (static_cast<bool>(flags.value & STATS_FLAG)) {
    kphp::log::assertion(opt_stats.has_value());
    opt_stats->store(tls);
  }
  if (static_cast<bool>(flags.value & EPOCH_NUMBER_FLAG)) {
    kphp::log::assertion(opt_epoch_number.has_value() && static_cast<bool>(flags.value & VIEW_NUMBER_FLAG) && opt_view_number.has_value());
    opt_epoch_number->store(tls), opt_view_number->store(tls);
  }
}

size_t rpcReqResultExtra::footprint(const tl::mask& flags) const noexcept {
  size_t footprint{};
  if (static_cast<bool>(flags.value & BINLOG_POS_FLAG)) {
    kphp::log::assertion(opt_binlog_pos.has_value());
    footprint += opt_binlog_pos->footprint();
  }
  if (static_cast<bool>(flags.value & BINLOG_TIME_FLAG)) {
    kphp::log::assertion(opt_binlog_time.has_value());
    footprint += opt_binlog_time->footprint();
  }
  if (static_cast<bool>(flags.value & ENGINE_PID_FLAG)) {
    kphp::log::assertion(opt_engine_pid.has_value());
    footprint += opt_engine_pid->footprint();
  }
  if (static_cast<bool>(flags.value & REQUEST_SIZE_FLAG)) {
    kphp::log::assertion(opt_request_size.has_value() && static_cast<bool>(flags.value & RESPONSE_SIZE_FLAG) && opt_response_size.has_value());
    footprint += opt_request_size->footprint() + opt_response_size->footprint();
  }
  if (static_cast<bool>(flags.value & FAILED_SUBQUERIES_FLAG)) {
    kphp::log::assertion(opt_failed_subqueries.has_value());
    footprint += opt_failed_subqueries->footprint();
  }
  if (static_cast<bool>(flags.value & COMPRESSION_VERSION_FLAG)) {
    kphp::log::assertion(opt_compression_version.has_value());
    footprint += opt_compression_version->footprint();
  }
  if (static_cast<bool>(flags.value & STATS_FLAG)) {
    kphp::log::assertion(opt_stats.has_value());
    footprint += opt_stats->footprint();
  }
  if (static_cast<bool>(flags.value & EPOCH_NUMBER_FLAG)) {
    kphp::log::assertion(opt_epoch_number.has_value() && static_cast<bool>(flags.value & VIEW_NUMBER_FLAG) && opt_view_number.has_value());
    footprint += opt_epoch_number->footprint() + opt_view_number->footprint();
  }
  return footprint;
}

} // namespace tl

namespace tl2 {

bool string::fetch(tl::fetcher& tlf) noexcept {
  uint8_t first_byte{};
  if (const auto opt_first_byte{tlf.fetch_trivial<uint8_t>()}; opt_first_byte) [[likely]] {
    first_byte = *opt_first_byte;
  } else {
    return false;
  }

  uint64_t string_len{};
  switch (first_byte) {
  case HUGE_STRING_MAGIC: {
    if (tlf.remaining() < HUGE_STRING_SIZE_LEN) [[unlikely]] {
      return false;
    }
    // we allow non-canonical length to speed up some rare implementations
    string_len = *tlf.fetch_trivial<uint64_t>();
    break;
  }
  case MEDIUM_STRING_MAGIC: {
    if (tlf.remaining() < MEDIUM_STRING_SIZE_LEN) [[unlikely]] {
      return false;
    }
    string_len = MEDIUM_STRING_MAGIC + *tlf.fetch_trivial<uint16_t>();
    break;
  }
  default: {
    string_len = static_cast<uint64_t>(first_byte);
    break;
  }
  }

  if (auto remaining{tlf.remaining()}; remaining < string_len) [[unlikely]] {
    kphp::log::warning("not enough space in buffer to fetch string: required {} bytes, remain {} bytes", string_len, remaining);
    return false;
  }

  value = {reinterpret_cast<const char*>(std::next(tlf.view().data(), tlf.pos())), static_cast<size_t>(string_len)};
  tlf.adjust(string_len);
  return true;
}

void string::store(tl::storer& tls) const noexcept {
  const size_t str_len{value.size()};

  if (str_len <= TINY_STRING_MAX_LEN) {
    tls.store_trivial<uint8_t>(str_len);
  } else if (str_len <= MEDIUM_STRING_MAX_LEN) {
    tls.store_trivial<uint8_t>(MEDIUM_STRING_MAGIC);
    tls.store_trivial<uint16_t>(str_len - MEDIUM_STRING_MAGIC);
  } else {
    tls.store_trivial<uint8_t>(HUGE_STRING_MAGIC);
    tls.store_trivial<uint64_t>(str_len);
  }
  tls.store_bytes({reinterpret_cast<const std::byte*>(value.data()), str_len});
}

} // namespace tl2
