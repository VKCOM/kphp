//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/tl/tl-core.h"

namespace tl {
void TLBuffer::store_string(std::string_view str) noexcept {
  const char *str_buf{str.data()};
  size_t str_len{str.size()};
  uint8_t size_len{};
  if (str_len <= SMALL_STRING_MAX_LEN) {
    size_len = SMALL_STRING_SIZE_LEN;
    store_trivial<uint8_t>(str_len);
  } else if (str_len <= MEDIUM_STRING_MAX_LEN) {
    size_len = MEDIUM_STRING_SIZE_LEN + 1;
    store_trivial<uint8_t>(MEDIUM_STRING_MAGIC);
    store_trivial<uint8_t>(str_len & 0xff);
    store_trivial<uint8_t>((str_len >> 8) & 0xff);
    store_trivial<uint8_t>((str_len >> 16) & 0xff);
  } else {
    php_warning("large strings aren't supported");
    size_len = SMALL_STRING_SIZE_LEN;
    str_len = 0;
    store_trivial<uint8_t>(str_len);
  }
  store_bytes(str_buf, str_len);

  const auto total_len{size_len + str_len};
  const auto total_len_with_padding{(total_len + 3) & ~static_cast<string::size_type>(3)};
  const auto padding{total_len_with_padding - total_len};

  std::array padding_array{'\0', '\0', '\0', '\0'};
  store_bytes(padding_array.data(), padding);
}

std::string_view TLBuffer::fetch_string() noexcept {
  uint8_t first_byte{};
  if (const auto opt_first_byte{fetch_trivial<uint8_t>()}; opt_first_byte) {
    first_byte = opt_first_byte.value();
  } else {
    return {}; // TODO: error handling
  }

  uint8_t size_len{};
  uint64_t string_len{};
  switch (first_byte) {
    case LARGE_STRING_MAGIC: {
      if (remaining() < LARGE_STRING_SIZE_LEN) {
        return {}; // TODO: error handling
      }
      size_len = LARGE_STRING_SIZE_LEN + 1;
      const auto first{static_cast<uint64_t>(fetch_trivial<uint8_t>().value())};
      const auto second{static_cast<uint64_t>(fetch_trivial<uint8_t>().value()) << 8};
      const auto third{static_cast<uint64_t>(fetch_trivial<uint8_t>().value()) << 16};
      const auto fourth{static_cast<uint64_t>(fetch_trivial<uint8_t>().value()) << 24};
      const auto fifth{static_cast<uint64_t>(fetch_trivial<uint8_t>().value()) << 32};
      const auto sixth{static_cast<uint64_t>(fetch_trivial<uint8_t>().value()) << 40};
      const auto seventh{static_cast<uint64_t>(fetch_trivial<uint8_t>().value()) << 48};
      string_len = first | second | third | fourth | fifth | sixth | seventh;

      const auto total_len_with_padding{(size_len + string_len + 3) & ~static_cast<uint64_t>(3)};
      adjust(total_len_with_padding - size_len);
      php_warning("large strings aren't supported");
      return {};
    }
    case MEDIUM_STRING_MAGIC: {
      if (remaining() < MEDIUM_STRING_SIZE_LEN) {
        return {}; // TODO: error handling
      }
      size_len = MEDIUM_STRING_SIZE_LEN + 1;
      const auto first{static_cast<uint64_t>(fetch_trivial<uint8_t>().value())};
      const auto second{static_cast<uint64_t>(fetch_trivial<uint8_t>().value()) << 8};
      const auto third{static_cast<uint64_t>(fetch_trivial<uint8_t>().value()) << 16};
      string_len = first | second | third;

      if (string_len <= SMALL_STRING_MAX_LEN) {
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
  if (m_remaining < total_len_with_padding - size_len) {
    return {}; // TODO: error handling
  }

  std::string_view response{data() + m_pos, static_cast<size_t>(string_len)};
  adjust(total_len_with_padding - size_len);
  return response;
}
} // namespace tl_core
