// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/tl/tl-types.h"

#include <cstdint>
#include <utility>

#include "common/tl/constants/common.h"
#include "runtime-light/tl/tl-core.h"

namespace tl {

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
