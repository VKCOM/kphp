// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/tl/tl-types.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <tuple>

#include "common/tl/constants/common.h"
#include "runtime-light/tl/tl-core.h"

namespace tl {

bool K2JobWorkerResponse::fetch(TLBuffer &tlb) noexcept {
  if (tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) != MAGIC || /* flags */ !tlb.fetch_trivial<uint32_t>().has_value()) {
    return false;
  }

  std::ignore = tlb.fetch_trivial<uint32_t>(); // ignore flags
  const auto opt_job_id{tlb.fetch_trivial<int64_t>()};
  if (!opt_job_id.has_value()) {
    return false;
  }

  job_id = *opt_job_id;
  body = tlb.fetch_string();
  return true;
}

void K2JobWorkerResponse::store(TLBuffer &tlb) const noexcept {
  tlb.store_trivial<uint32_t>(MAGIC);
  tlb.store_trivial<uint32_t>(0x0); // flags
  tlb.store_trivial<int64_t>(job_id);
  tlb.store_string(body);
}

bool GetPemCertInfoResponse::fetch(TLBuffer &tlb) noexcept {
  if (const auto magic = tlb.fetch_trivial<uint32_t>(); magic.value_or(TL_ZERO) != TL_MAYBE_TRUE) {
    return false;
  }

  if (const auto magic = tlb.fetch_trivial<uint32_t>(); magic.value_or(TL_ZERO) != TL_DICTIONARY) {
    return false;
  }

  const std::optional<uint32_t> size = tlb.fetch_trivial<uint32_t>();
  if (!size.has_value()) {
    return false;
  }

  auto response = array<mixed>::create();
  response.reserve(*size, false);

  for (uint32_t i = 0; i < *size; ++i) {
    const auto key_view = tlb.fetch_string();
    if (key_view.empty()) {
      return false;
    }

    const auto key = string(key_view.data(), key_view.length());

    const std::optional<uint32_t> magic = tlb.fetch_trivial<uint32_t>();
    if (!magic.has_value()) {
      return false;
    }

    switch (*magic) {
      case CertInfoItem::LONG_MAGIC: {
        const std::optional<int64_t> val = tlb.fetch_trivial<int64_t>();
        if (!val.has_value()) {
          return false;
        }
        response[key] = *val;
        break;
      }
      case CertInfoItem::STR_MAGIC: {
        const auto value_view = tlb.fetch_string();
        if (value_view.empty()) {
          return false;
        }
        const auto value = string(value_view.data(), value_view.size());

        response[key] = string(value_view.data(), value_view.size());
        break;
      }
      case CertInfoItem::DICT_MAGIC: {
        auto sub_array = array<string>::create();
        const std::optional<uint32_t> sub_size = tlb.fetch_trivial<uint32_t>();

        if (!sub_size.has_value()) {
          return false;
        }

        for (size_t j = 0; j < sub_size; ++j) {
          const auto sub_key_view = tlb.fetch_string();
          if (sub_key_view.empty()) {
            return false;
          }
          const auto sub_key = string(sub_key_view.data(), sub_key_view.size());

          const auto sub_value_view = tlb.fetch_string();
          if (sub_value_view.empty()) {
            return false;
          }
          const auto sub_value = string(sub_value_view.data(), sub_value_view.size());

          sub_array[sub_key] = sub_value;
        }
        response[key] = sub_array;

        break;
      }
      default:
        return false;
    }
  }

  data = std::move(response);
  return true;
}

bool confdataValue::fetch(TLBuffer &tlb) noexcept {
  const auto value_view{tlb.fetch_string()};
  Bool is_php_serialized_{};
  Bool is_json_serialized_{};
  if (!(is_php_serialized_.fetch(tlb) && is_json_serialized_.fetch(tlb))) {
    return false;
  }

  value = {value_view.data(), static_cast<string::size_type>(value_view.size())};
  is_php_serialized = is_php_serialized_.value;
  is_json_serialized = is_json_serialized_.value;
  return true;
}

} // namespace tl
