// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/tl/tl-types.h"

#include <array>
#include <cstdint>
#include <utility>

#include "runtime-light/tl/tl-core.h"
#include "runtime-light/utils/logs.h"

namespace tl {

bool string::fetch(TLBuffer& tlb) noexcept {
  uint8_t first_byte{};
  if (const auto opt_first_byte{tlb.fetch_trivial<uint8_t>()}; opt_first_byte) [[likely]] {
    first_byte = *opt_first_byte;
  } else {
    return false;
  }

  uint8_t size_len{};
  uint64_t string_len{};
  switch (first_byte) {
  case LARGE_STRING_MAGIC: {
    if (tlb.remaining() < LARGE_STRING_SIZE_LEN) [[unlikely]] {
      return false;
    }
    size_len = LARGE_STRING_SIZE_LEN + 1;
    const auto first{static_cast<uint64_t>(tlb.fetch_trivial<uint8_t>().value())};
    const auto second{static_cast<uint64_t>(tlb.fetch_trivial<uint8_t>().value()) << 8};
    const auto third{static_cast<uint64_t>(tlb.fetch_trivial<uint8_t>().value()) << 16};
    const auto fourth{static_cast<uint64_t>(tlb.fetch_trivial<uint8_t>().value()) << 24};
    const auto fifth{static_cast<uint64_t>(tlb.fetch_trivial<uint8_t>().value()) << 32};
    const auto sixth{static_cast<uint64_t>(tlb.fetch_trivial<uint8_t>().value()) << 40};
    const auto seventh{static_cast<uint64_t>(tlb.fetch_trivial<uint8_t>().value()) << 48};
    string_len = first | second | third | fourth | fifth | sixth | seventh;

    const auto total_len_with_padding{(size_len + string_len + 3) & ~static_cast<uint64_t>(3)};
    tlb.adjust(total_len_with_padding - size_len);
    kphp::log::warning("large strings aren't supported (length = {})", string_len);
    return false;
  }
  case MEDIUM_STRING_MAGIC: {
    if (tlb.remaining() < MEDIUM_STRING_SIZE_LEN) [[unlikely]] {
      return false;
    }
    size_len = MEDIUM_STRING_SIZE_LEN + 1;
    const auto first{static_cast<uint64_t>(tlb.fetch_trivial<uint8_t>().value())};
    const auto second{static_cast<uint64_t>(tlb.fetch_trivial<uint8_t>().value()) << 8};
    const auto third{static_cast<uint64_t>(tlb.fetch_trivial<uint8_t>().value()) << 16};
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
  if (tlb.remaining() < total_len_with_padding - size_len) [[unlikely]] {
    return false;
  }

  value = {std::next(tlb.data(), tlb.pos()), static_cast<size_t>(string_len)};
  tlb.adjust(total_len_with_padding - size_len);
  return true;
}

void string::store(TLBuffer& tlb) const noexcept {
  const char* str_buf{value.data()};
  size_t str_len{value.size()};
  uint8_t size_len{};
  if (str_len <= SMALL_STRING_MAX_LEN) {
    size_len = SMALL_STRING_SIZE_LEN;
    tlb.store_trivial<uint8_t>(str_len);
  } else if (str_len <= MEDIUM_STRING_MAX_LEN) {
    size_len = MEDIUM_STRING_SIZE_LEN + 1;
    tlb.store_trivial<uint8_t>(MEDIUM_STRING_MAGIC);
    tlb.store_trivial<uint8_t>(str_len & 0xff);
    tlb.store_trivial<uint8_t>((str_len >> 8) & 0xff);
    tlb.store_trivial<uint8_t>((str_len >> 16) & 0xff);
  } else {
    kphp::log::warning("large strings aren't supported");
    size_len = SMALL_STRING_SIZE_LEN;
    str_len = 0;
    tlb.store_trivial<uint8_t>(str_len);
  }
  tlb.store_bytes({str_buf, str_len});

  const auto total_len{size_len + str_len};
  const auto total_len_with_padding{(total_len + 3) & ~3};
  const auto padding{total_len_with_padding - total_len};

  constexpr std::array padding_array{'\0', '\0', '\0', '\0'};
  tlb.store_bytes({padding_array.data(), padding});
}

bool K2JobWorkerResponse::fetch(TLBuffer& tlb) noexcept {
  tl::details::magic magic{};
  bool ok{magic.fetch(tlb) && magic.expect(MAGIC)};
  ok &= tl::details::mask{}.fetch(tlb);
  ok &= job_id.fetch(tlb);
  ok &= body.fetch(tlb);
  return ok;
}

void K2JobWorkerResponse::store(TLBuffer& tlb) const noexcept {
  tl::details::magic{.value = MAGIC}.store(tlb);
  tl::details::mask{}.store(tlb);
  job_id.store(tlb);
  body.store(tlb);
}

bool CertInfoItem::fetch(TLBuffer& tlb) noexcept {
  tl::details::magic magic{};
  if (!magic.fetch(tlb)) [[unlikely]] {
    return false;
  }

  switch (magic.value) {
  case Magic::LONG: {
    if (tl::i64 val{}; val.fetch(tlb)) [[likely]] {
      data = val;
      break;
    }
    return false;
  }
  case Magic::STR: {
    if (tl::string val{}; val.fetch(tlb)) [[likely]] {
      data = val;
      break;
    }
    return false;
  }
  case Magic::DICT: {
    if (tl::dictionary<tl::string> val{}; val.fetch(tlb)) [[likely]] {
      data = std::move(val);
      break;
    }
    return false;
  }
  }

  return true;
}

// ===== RPC =====

bool rpcInvokeReqExtra::fetch(tl::TLBuffer& tlb) noexcept {
  bool ok{flags.fetch(tlb)};
  if (ok && static_cast<bool>(flags.value & WAIT_BINLOG_POS_FLAG)) {
    ok &= opt_wait_binlog_pos.emplace().fetch(tlb);
  }
  if (ok && static_cast<bool>(flags.value & STRING_FORWARD_KEYS_FLAG)) {
    ok &= opt_string_forward_keys.emplace().fetch(tlb);
  }
  if (ok && static_cast<bool>(flags.value & INT_FORWARD_KEYS_FLAG)) {
    ok &= opt_int_forward_keys.emplace().fetch(tlb);
  }
  if (ok && static_cast<bool>(flags.value & STRING_FORWARD_FLAG)) {
    ok &= opt_string_forward.emplace().fetch(tlb);
  }
  if (ok && static_cast<bool>(flags.value & INT_FORWARD_FLAG)) {
    ok &= opt_int_forward.emplace().fetch(tlb);
  }
  if (ok && static_cast<bool>(flags.value & CUSTOM_TIMEOUT_MS_FLAG)) {
    ok &= opt_custom_timeout_ms.emplace().fetch(tlb);
  }
  if (ok && static_cast<bool>(flags.value & SUPPORTED_COMPRESSION_VERSION_FLAG)) {
    ok &= opt_supported_compression_version.emplace().fetch(tlb);
  }
  if (ok && static_cast<bool>(flags.value & RANDOM_DELAY_FLAG)) {
    ok &= opt_random_delay.emplace().fetch(tlb);
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

void rpcReqResultExtra::store(tl::TLBuffer& tlb) const noexcept {
  flags.store(tlb);
  if (static_cast<bool>(flags.value & BINLOG_POS_FLAG)) {
    binlog_pos.store(tlb);
  }
  if (static_cast<bool>(flags.value & BINLOG_TIME_FLAG)) {
    binlog_time.store(tlb);
  }
  if (static_cast<bool>(flags.value) & ENGINE_PID_FLAG) {
    engine_pid.store(tlb);
  }
  if (static_cast<bool>(flags.value & REQUEST_SIZE_FLAG)) {
    kphp::log::assertion(static_cast<bool>(flags.value & RESPONSE_SIZE_FLAG));
    request_size.store(tlb), response_size.store(tlb);
  }
  if (static_cast<bool>(flags.value & FAILED_SUBQUERIES_FLAG)) {
    failed_subqueries.store(tlb);
  }
  if (static_cast<bool>(flags.value & COMPRESSION_VERSION_FLAG)) {
    compression_version.store(tlb);
  }
  if (static_cast<bool>(flags.value & STATS_FLAG)) {
    stats.store(tlb);
  }
  if (static_cast<bool>(flags.value & EPOCH_NUMBER_FLAG)) {
    kphp::log::assertion(static_cast<bool>(flags.value & VIEW_NUMBER_FLAG));
    epoch_number.store(tlb), view_number.store(tlb);
  }
}

} // namespace tl
