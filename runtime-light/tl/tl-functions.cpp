// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/tl/tl-functions.h"

#include <cstdint>
#include <string_view>

#include "common/tl/constants/common.h"
#include "runtime-light/tl/tl-core.h"

namespace tl {

// ===== JOB WORKERS =====

bool K2InvokeJobWorker::fetch(TLBuffer &tlb) noexcept {
  bool ok{tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) == K2_INVOKE_JOB_WORKER_MAGIC};
  const auto opt_flags{tlb.fetch_trivial<uint32_t>()};
  ok &= opt_flags.has_value();
  const auto opt_image_id{tlb.fetch_trivial<uint64_t>()};
  ok &= opt_image_id.has_value();
  const auto opt_job_id{tlb.fetch_trivial<int64_t>()};
  ok &= opt_job_id.has_value();
  const auto opt_timeout_ns{tlb.fetch_trivial<uint64_t>()};
  ok &= opt_timeout_ns.has_value();
  ok &= body.fetch(tlb);

  ignore_answer = static_cast<bool>(opt_flags.value_or(0x0) & IGNORE_ANSWER_FLAG);
  image_id = opt_image_id.value_or(0);
  job_id = opt_job_id.value_or(0);
  timeout_ns = opt_timeout_ns.value_or(0);

  return ok;
}

void K2InvokeJobWorker::store(TLBuffer &tlb) const noexcept {
  const uint32_t flags{ignore_answer ? IGNORE_ANSWER_FLAG : 0x0};
  tlb.store_trivial<uint32_t>(K2_INVOKE_JOB_WORKER_MAGIC);
  tlb.store_trivial<uint32_t>(flags);
  tlb.store_trivial<uint64_t>(image_id);
  tlb.store_trivial<int64_t>(job_id);
  tlb.store_trivial<uint64_t>(timeout_ns);
  body.store(tlb);
}

// ===== CRYPTO =====

void GetCryptosecurePseudorandomBytes::store(TLBuffer &tlb) const noexcept {
  tlb.store_trivial<uint32_t>(GET_CRYPTOSECURE_PSEUDORANDOM_BYTES_MAGIC);
  tlb.store_trivial<int32_t>(size);
}

void GetPemCertInfo::store(TLBuffer &tlb) const noexcept {
  tlb.store_trivial<uint32_t>(GET_PEM_CERT_INFO_MAGIC);
  tlb.store_trivial<uint32_t>(is_short ? TL_BOOL_TRUE : TL_BOOL_FALSE);
  bytes.store(tlb);
}

void DigestSign::store(TLBuffer &tlb) const noexcept {
  tlb.store_trivial<uint32_t>(DIGEST_SIGN_MAGIC);
  data.store(tlb);
  private_key.store(tlb);
  tlb.store_trivial<uint32_t>(algorithm);
}

void DigestVerify::store(TLBuffer &tlb) const noexcept {
  tlb.store_trivial<uint32_t>(DIGEST_VERIFY_MAGIC);
  data.store(tlb);
  public_key.store(tlb);
  tlb.store_trivial<uint32_t>(algorithm);
  signature.store(tlb);
}

void CbcDecrypt::store(TLBuffer &tlb) const noexcept {
  tlb.store_trivial<uint32_t>(CBC_DECRYPT_MAGIC);
  tlb.store_trivial<uint32_t>(algorithm);
  tlb.store_trivial<uint32_t>(padding);
  passphrase.store(tlb);
  iv.store(tlb);
  data.store(tlb);
}

void CbcEncrypt::store(TLBuffer &tlb) const noexcept {
  tlb.store_trivial<uint32_t>(CBC_ENCRYPT_MAGIC);
  tlb.store_trivial<uint32_t>(algorithm);
  tlb.store_trivial<uint32_t>(padding);
  passphrase.store(tlb);
  iv.store(tlb);
  data.store(tlb);
}

void Hash::store(TLBuffer &tlb) const noexcept {
  tlb.store_trivial<uint32_t>(HASH_MAGIC);
  tlb.store_trivial<uint32_t>(algorithm);
  data.store(tlb);
}

void HashHmac::store(TLBuffer &tlb) const noexcept {
  tlb.store_trivial<uint32_t>(HASH_HMAC_MAGIC);
  tlb.store_trivial<uint32_t>(algorithm);
  data.store(tlb);
  secret_key.store(tlb);
}

// ===== CONFDATA =====

void ConfdataGet::store(TLBuffer &tlb) const noexcept {
  tlb.store_trivial<uint32_t>(CONFDATA_GET_MAGIC);
  key.store(tlb);
}

void ConfdataGetWildcard::store(TLBuffer &tlb) const noexcept {
  tlb.store_trivial<uint32_t>(CONFDATA_GET_WILDCARD_MAGIC);
  wildcard.store(tlb);
}

// ===== HTTP =====

bool K2InvokeHttp::fetch(TLBuffer &tlb) noexcept {
  bool ok{tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) == K2_INVOKE_HTTP_MAGIC};
  ok &= tlb.fetch_trivial<uint32_t>().has_value(); // flags
  ok &= connection.fetch(tlb);
  ok &= version.fetch(tlb);
  ok &= method.fetch(tlb);
  ok &= uri.fetch(tlb);
  ok &= headers.fetch(tlb);
  const auto opt_body{tlb.fetch_bytes(tlb.remaining())};
  ok &= opt_body.has_value();

  body = opt_body.value_or(std::string_view{});

  return ok;
}

} // namespace tl
