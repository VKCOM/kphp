// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/tl/tl-types.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "common/tl/constants/common.h"
#include "runtime-light/tl/tl-core.h"

namespace tl {

bool K2JobWorkerResponse::fetch(TLBuffer &tlb) noexcept {
  if (tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) != MAGIC || /* flags */ !tlb.fetch_trivial<uint32_t>().has_value()) {
    return false;
  }

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

bool CertInfoItem::fetch(TLBuffer &tlb) noexcept {
  const std::optional<uint32_t> magic = tlb.fetch_trivial<uint32_t>();
  if (!magic.has_value()) {
    return false;
  }

  switch (*magic) {
    case Magic::LONG: {
      const std::optional<int64_t> val = tlb.fetch_trivial<int64_t>();
      if (!val.has_value()) {
        return false;
      }
      data = *val;
      break;
    }
    case Magic::STR: {
      auto val = tl::string{};
      if (!val.fetch(tlb)) {
        return false;
      }
      data = val;
      break;
    }
    case Magic::DICT: {
      auto val = dictionary<tl::string>();
      if (!val.fetch(tlb)) {
        return false;
      }
      data = std::move(val);
      break;
    }
  }

  return true;
}

bool confdataValue::fetch(TLBuffer &tlb) noexcept {
  value = tlb.fetch_string();
  Bool is_php_serialized_{};
  Bool is_json_serialized_{};
  if (!(is_php_serialized_.fetch(tlb) && is_json_serialized_.fetch(tlb))) {
    return false;
  }

  is_php_serialized = is_php_serialized_.value;
  is_json_serialized = is_json_serialized_.value;
  return true;
}

} // namespace tl
