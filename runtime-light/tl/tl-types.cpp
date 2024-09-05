// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/tl/tl-types.h"

#include <cstdint>
#include <string_view>

#include "runtime-light/tl/tl-core.h"

namespace {

// magic + flags + status_code + job_id + minimum string size length
constexpr auto K2_JOB_WORKER_RESPONSE_MIN_SIZE = 2 * sizeof(uint32_t) + 2 * sizeof(int64_t) + tl::SMALL_STRING_SIZE_LEN;

} // namespace

namespace tl {

bool K2JobWorkerResponse::fetch(TLBuffer &tlb) noexcept {
  if (tlb.size() < K2_JOB_WORKER_RESPONSE_MIN_SIZE || *tlb.fetch_trivial<uint32_t>() != K2_JOB_WORKER_RESPONSE_MAGIC) {
    return false;
  }

  flags = *tlb.fetch_trivial<uint32_t>();
  job_id = *tlb.fetch_trivial<int64_t>();
  const std::string_view body_view{tlb.fetch_string()};
  body = string{body_view.data(), static_cast<string::size_type>(body_view.size())};
  return true;
}

void K2JobWorkerResponse::store(TLBuffer &tlb) const noexcept {
  tlb.store_trivial<uint32_t>(K2_JOB_WORKER_RESPONSE_MAGIC);
  tlb.store_trivial<uint32_t>(flags);
  tlb.store_trivial<int64_t>(job_id);
  tlb.store_string({body.c_str(), body.size()});
}

} // namespace tl
