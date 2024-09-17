// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/tl/tl-functions.h"

#include <cstdint>
#include <string_view>

#include "common/tl/constants/common.h"
#include "runtime-light/tl/tl-core.h"

namespace {

// magic + flags + image_id + job_id + minimum string size length
constexpr auto K2_INVOKE_JW_MIN_SIZE = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(int64_t) + sizeof(uint64_t) + tl::SMALL_STRING_SIZE_LEN;

constexpr auto K2_JOB_WORKER_IGNORE_ANSWER_FLAG = static_cast<uint32_t>(1U << 0U);

} // namespace

namespace tl {

bool K2InvokeJobWorker::fetch(TLBuffer &tlb) noexcept {
  if (tlb.size() < K2_INVOKE_JW_MIN_SIZE || *tlb.fetch_trivial<uint32_t>() != K2_INVOKE_JOB_WORKER_MAGIC) {
    return false;
  }

  const auto flags{*tlb.fetch_trivial<uint32_t>()};
  ignore_answer = static_cast<bool>(flags & K2_JOB_WORKER_IGNORE_ANSWER_FLAG);
  image_id = *tlb.fetch_trivial<uint64_t>();
  job_id = *tlb.fetch_trivial<int64_t>();
  timeout_ns = *tlb.fetch_trivial<uint64_t>();
  const std::string_view body_view{tlb.fetch_string()};
  body = string{body_view.data(), static_cast<string::size_type>(body_view.size())};
  return true;
}

void K2InvokeJobWorker::store(TLBuffer &tlb) const noexcept {
  const uint32_t flags{ignore_answer ? K2_JOB_WORKER_IGNORE_ANSWER_FLAG : 0x0};
  tlb.store_trivial<uint32_t>(K2_INVOKE_JOB_WORKER_MAGIC);
  tlb.store_trivial<uint32_t>(flags);
  tlb.store_trivial<uint64_t>(image_id);
  tlb.store_trivial<int64_t>(job_id);
  tlb.store_trivial<uint64_t>(timeout_ns);
  tlb.store_string({body.c_str(), body.size()});
}

void GetCryptosecurePseudorandomBytes::store(TLBuffer &tlb) const noexcept {
  tlb.store_trivial<uint32_t>(GET_CRYPTOSECURE_PSEUDORANDOM_BYTES_MAGIC);
  tlb.store_trivial<int32_t>(size);
}

void GetPemCertInfo::store(TLBuffer &tlb) const noexcept {
  tlb.store_trivial<uint32_t>(GET_PEM_CERT_INFO_MAGIC);
  tlb.store_trivial<uint32_t>(is_short ? TL_BOOL_TRUE : TL_BOOL_FALSE);
  tlb.store_string(std::string_view{bytes.c_str(), bytes.size()});
}

} // namespace tl
