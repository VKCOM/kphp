// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/tl/tl-types.h"

#include <array>
#include <cstdint>
#include <utility>

#include "common/tl/constants/common.h"
#include "runtime-light/tl/tl-core.h"

namespace tl {

bool string::fetch(TLBuffer &tlb) noexcept {
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
      php_warning("large strings aren't supported");
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
        php_warning("long string's length is less than 254");
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

void string::store(TLBuffer &tlb) const noexcept {
  const char *str_buf{value.data()};
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
    php_warning("large strings aren't supported");
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

bool K2JobWorkerResponse::fetch(TLBuffer &tlb) noexcept {
  bool ok{tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) == MAGIC};
  ok &= tlb.fetch_trivial<uint32_t>().has_value(); // flags
  const auto opt_job_id{tlb.fetch_trivial<int64_t>()};
  ok &= opt_job_id.has_value();
  ok &= body.fetch(tlb);

  job_id = opt_job_id.value_or(0);

  return ok;
}

void K2JobWorkerResponse::store(TLBuffer &tlb) const noexcept {
  tlb.store_trivial<uint32_t>(MAGIC);
  tlb.store_trivial<uint32_t>(0x0); // flags
  tlb.store_trivial<int64_t>(job_id);
  body.store(tlb);
}

bool CertInfoItem::fetch(TLBuffer &tlb) noexcept {
  const auto opt_magic{tlb.fetch_trivial<uint32_t>()};
  if (!opt_magic.has_value()) {
    return false;
  }

  switch (*opt_magic) {
    case Magic::LONG: {
      if (const auto opt_val{tlb.fetch_trivial<int64_t>()}; opt_val.has_value()) [[likely]] {
        data = *opt_val;
        break;
      }
      return false;
    }
    case Magic::STR: {
      if (string val{}; val.fetch(tlb)) [[unlikely]] {
        data = val;
        break;
      }
      return false;
    }
    case Magic::DICT: {
      if (dictionary<string> val{}; val.fetch(tlb)) [[likely]] {
        data = std::move(val);
        break;
      }
      return false;
    }
  }

  return true;
}

} // namespace tl
