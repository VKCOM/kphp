// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/tl/tl-types.h"

#include <cstdint>
#include <string_view>
#include <tuple>

#include "common/tl/constants/common.h"
#include "runtime-light/tl/tl-core.h"

namespace {

// magic + flags + job_id + minimum string size length
constexpr auto K2_JOB_WORKER_RESPONSE_MIN_SIZE = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(int64_t) + tl::SMALL_STRING_SIZE_LEN;

} // namespace

namespace tl {

bool K2JobWorkerResponse::fetch(TLBuffer &tlb) noexcept {
  if (tlb.size() < K2_JOB_WORKER_RESPONSE_MIN_SIZE || *tlb.fetch_trivial<uint32_t>() != K2_JOB_WORKER_RESPONSE_MAGIC) {
    return false;
  }

  std::ignore = tlb.fetch_trivial<uint32_t>(); // ignore flags
  job_id = *tlb.fetch_trivial<int64_t>();
  const std::string_view body_view{tlb.fetch_string()};
  body = string{body_view.data(), static_cast<string::size_type>(body_view.size())};
  return true;
}

void K2JobWorkerResponse::store(TLBuffer &tlb) const noexcept {
  tlb.store_trivial<uint32_t>(K2_JOB_WORKER_RESPONSE_MAGIC);
  tlb.store_trivial<uint32_t>(0x0); // flags
  tlb.store_trivial<int64_t>(job_id);
  tlb.store_string({body.c_str(), body.size()});
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

} // namespace tl
