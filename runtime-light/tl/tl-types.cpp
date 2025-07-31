// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/tl/tl-types.h"

#include <array>
#include <cstddef>
#include <cstdint>
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
  case LARGE_STRING_MAGIC: {
    if (tlf.remaining() < LARGE_STRING_SIZE_LEN) [[unlikely]] {
      return false;
    }
    size_len = LARGE_STRING_SIZE_LEN + 1;
    const auto first{static_cast<uint64_t>(*tlf.fetch_trivial<uint8_t>())};
    const auto second{static_cast<uint64_t>(*tlf.fetch_trivial<uint8_t>()) << 8};
    const auto third{static_cast<uint64_t>(*tlf.fetch_trivial<uint8_t>()) << 16};
    const auto fourth{static_cast<uint64_t>(*tlf.fetch_trivial<uint8_t>()) << 24};
    const auto fifth{static_cast<uint64_t>(*tlf.fetch_trivial<uint8_t>()) << 32};
    const auto sixth{static_cast<uint64_t>(*tlf.fetch_trivial<uint8_t>()) << 40};
    const auto seventh{static_cast<uint64_t>(*tlf.fetch_trivial<uint8_t>()) << 48};
    string_len = first | second | third | fourth | fifth | sixth | seventh;

    const auto total_len_with_padding{(size_len + string_len + 3) & ~static_cast<uint64_t>(3)};
    tlf.adjust(total_len_with_padding - size_len);
    kphp::log::warning("large strings aren't supported (length = {})", string_len);
    return false;
  }
  case MEDIUM_STRING_MAGIC: {
    if (tlf.remaining() < MEDIUM_STRING_SIZE_LEN) [[unlikely]] {
      return false;
    }
    size_len = MEDIUM_STRING_SIZE_LEN + 1;
    const auto first{static_cast<uint64_t>(*tlf.fetch_trivial<uint8_t>())};
    const auto second{static_cast<uint64_t>(*tlf.fetch_trivial<uint8_t>()) << 8};
    const auto third{static_cast<uint64_t>(*tlf.fetch_trivial<uint8_t>()) << 16};
    string_len = first | second | third;

    if (string_len <= SMALL_STRING_MAX_LEN) [[unlikely]] {
      kphp::log::warning("long string's length is less than 254 (length = {})", string_len);
    }
    break;
  }
  default: {
    size_len = SMALL_STRING_SIZE_LEN;
    string_len = static_cast<uint64_t>(first_byte);
    break;
  }
  }
  const auto total_len_with_padding{(size_len + string_len + 3) & ~static_cast<uint64_t>(3)};
  if (tlf.remaining() < total_len_with_padding - size_len) [[unlikely]] {
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
  if (str_len <= SMALL_STRING_MAX_LEN) {
    size_len = SMALL_STRING_SIZE_LEN;
    tls.store_trivial<uint8_t>(str_len);
  } else if (str_len <= MEDIUM_STRING_MAX_LEN) {
    size_len = MEDIUM_STRING_SIZE_LEN + 1;
    tls.store_trivial<uint8_t>(MEDIUM_STRING_MAGIC);
    tls.store_trivial<uint8_t>(str_len & 0xff);
    tls.store_trivial<uint8_t>((str_len >> 8) & 0xff);
    tls.store_trivial<uint8_t>((str_len >> 16) & 0xff);
  } else {
    kphp::log::warning("large strings aren't supported");
    size_len = SMALL_STRING_SIZE_LEN;
    str_len = 0;
    tls.store_trivial<uint8_t>(str_len);
  }
  tls.store_bytes({reinterpret_cast<const std::byte*>(str_buf), str_len});

  const auto total_len{size_len + str_len};
  const auto total_len_with_padding{(total_len + 3) & ~3};
  const auto padding{total_len_with_padding - total_len};

  constexpr std::array padding_array{'\0', '\0', '\0', '\0'};
  tls.store_bytes({reinterpret_cast<const std::byte*>(padding_array.data()), padding});
}

size_t string::footprint() const noexcept {
  size_t str_len{value.size()};
  size_t size_len{};
  if (str_len <= SMALL_STRING_MAX_LEN) {
    size_len = SMALL_STRING_SIZE_LEN;
  } else if (str_len <= MEDIUM_STRING_MAX_LEN) {
    size_len = MEDIUM_STRING_SIZE_LEN + 1;
  } else {
    size_len = LARGE_STRING_SIZE_LEN + 1;
  }

  const auto total_len{size_len + str_len};
  const auto total_len_with_padding{(total_len + 3) & ~3};
  return total_len_with_padding;
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

bool rpcInvokeReqExtra::fetch(tl::fetcher& tlf) noexcept {
  bool ok{flags.fetch(tlf)};
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

void rpcReqResultExtra::store(tl::storer& tls) const noexcept {
  flags.store(tls);
  if (static_cast<bool>(flags.value & BINLOG_POS_FLAG)) {
    binlog_pos.store(tls);
  }
  if (static_cast<bool>(flags.value & BINLOG_TIME_FLAG)) {
    binlog_time.store(tls);
  }
  if (static_cast<bool>(flags.value) & ENGINE_PID_FLAG) {
    engine_pid.store(tls);
  }
  if (static_cast<bool>(flags.value & REQUEST_SIZE_FLAG)) {
    kphp::log::assertion(static_cast<bool>(flags.value & RESPONSE_SIZE_FLAG));
    request_size.store(tls), response_size.store(tls);
  }
  if (static_cast<bool>(flags.value & FAILED_SUBQUERIES_FLAG)) {
    failed_subqueries.store(tls);
  }
  if (static_cast<bool>(flags.value & COMPRESSION_VERSION_FLAG)) {
    compression_version.store(tls);
  }
  if (static_cast<bool>(flags.value & STATS_FLAG)) {
    stats.store(tls);
  }
  if (static_cast<bool>(flags.value & EPOCH_NUMBER_FLAG)) {
    kphp::log::assertion(static_cast<bool>(flags.value & VIEW_NUMBER_FLAG));
    epoch_number.store(tls), view_number.store(tls);
  }
}

size_t rpcReqResultExtra::footprint() const noexcept {
  size_t footprint{flags.footprint()};
  if (static_cast<bool>(flags.value & BINLOG_POS_FLAG)) {
    footprint += binlog_pos.footprint();
  }
  if (static_cast<bool>(flags.value & BINLOG_TIME_FLAG)) {
    footprint += binlog_time.footprint();
  }
  if (static_cast<bool>(flags.value) & ENGINE_PID_FLAG) {
    footprint += engine_pid.footprint();
  }
  if (static_cast<bool>(flags.value & REQUEST_SIZE_FLAG)) {
    kphp::log::assertion(static_cast<bool>(flags.value & RESPONSE_SIZE_FLAG));
    footprint += request_size.footprint() + response_size.footprint();
  }
  if (static_cast<bool>(flags.value & FAILED_SUBQUERIES_FLAG)) {
    footprint += failed_subqueries.footprint();
  }
  if (static_cast<bool>(flags.value & COMPRESSION_VERSION_FLAG)) {
    footprint += compression_version.footprint();
  }
  if (static_cast<bool>(flags.value & STATS_FLAG)) {
    footprint += stats.footprint();
  }
  if (static_cast<bool>(flags.value & EPOCH_NUMBER_FLAG)) {
    kphp::log::assertion(static_cast<bool>(flags.value & VIEW_NUMBER_FLAG));
    footprint += epoch_number.footprint() + view_number.footprint();
  }
  return footprint;
}

} // namespace tl
