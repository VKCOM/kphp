// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/tl/tl-functions.h"

#include <cstdint>
#include <string_view>

#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-types.h"

namespace tl {

// ===== JOB WORKERS =====

bool K2InvokeJobWorker::fetch(tl::TLBuffer& tlb) noexcept {
  tl::details::magic magic{};
  tl::details::mask flags{};
  bool ok{magic.fetch(tlb) && magic.expect(K2_INVOKE_JOB_WORKER_MAGIC)};
  ok &= flags.fetch(tlb);
  ok &= image_id.fetch(tlb);
  ok &= job_id.fetch(tlb);
  ok &= timeout_ns.fetch(tlb);
  ok &= body.fetch(tlb);
  ignore_answer = static_cast<bool>(flags.value & IGNORE_ANSWER_FLAG);
  return ok;
}

void K2InvokeJobWorker::store(tl::TLBuffer& tlb) const noexcept {
  tl::details::magic{.value = K2_INVOKE_JOB_WORKER_MAGIC}.store(tlb);
  tl::details::mask{.value = ignore_answer ? IGNORE_ANSWER_FLAG : 0x0}.store(tlb);
  image_id.store(tlb);
  job_id.store(tlb);
  timeout_ns.store(tlb);
  body.store(tlb);
}

// ===== CRYPTO =====

void GetCryptosecurePseudorandomBytes::store(tl::TLBuffer& tlb) const noexcept {
  tl::details::magic{.value = GET_CRYPTOSECURE_PSEUDORANDOM_BYTES_MAGIC}.store(tlb);
  size.store(tlb);
}

void GetPemCertInfo::store(tl::TLBuffer& tlb) const noexcept {
  tl::details::magic{.value = GET_PEM_CERT_INFO_MAGIC}.store(tlb);
  tl::Bool{.value = is_short}.store(tlb);
  bytes.store(tlb);
}

void DigestSign::store(tl::TLBuffer& tlb) const noexcept {
  tl::details::magic{.value = DIGEST_SIGN_MAGIC}.store(tlb);
  data.store(tlb);
  private_key.store(tlb);
  tlb.store_trivial<uint32_t>(algorithm);
}

void DigestVerify::store(tl::TLBuffer& tlb) const noexcept {
  tl::details::magic{.value = DIGEST_VERIFY_MAGIC}.store(tlb);
  data.store(tlb);
  public_key.store(tlb);
  tlb.store_trivial<uint32_t>(algorithm);
  signature.store(tlb);
}

void CbcDecrypt::store(tl::TLBuffer& tlb) const noexcept {
  tl::details::magic{.value = CBC_DECRYPT_MAGIC}.store(tlb);
  tlb.store_trivial<uint32_t>(algorithm);
  tlb.store_trivial<uint32_t>(padding);
  passphrase.store(tlb);
  iv.store(tlb);
  data.store(tlb);
}

void CbcEncrypt::store(tl::TLBuffer& tlb) const noexcept {
  tl::details::magic{.value = CBC_ENCRYPT_MAGIC}.store(tlb);
  tlb.store_trivial<uint32_t>(algorithm);
  tlb.store_trivial<uint32_t>(padding);
  passphrase.store(tlb);
  iv.store(tlb);
  data.store(tlb);
}

void Hash::store(tl::TLBuffer& tlb) const noexcept {
  tl::details::magic{.value = HASH_MAGIC}.store(tlb);
  tlb.store_trivial<uint32_t>(algorithm);
  data.store(tlb);
}

void HashHmac::store(tl::TLBuffer& tlb) const noexcept {
  tl::details::magic{.value = HASH_HMAC_MAGIC}.store(tlb);
  tlb.store_trivial<uint32_t>(algorithm);
  data.store(tlb);
  secret_key.store(tlb);
}

// ===== CONFDATA =====

void ConfdataGet::store(tl::TLBuffer& tlb) const noexcept {
  tl::details::magic{.value = CONFDATA_GET_MAGIC}.store(tlb);
  key.store(tlb);
}

void ConfdataGetWildcard::store(tl::TLBuffer& tlb) const noexcept {
  tl::details::magic{.value = CONFDATA_GET_WILDCARD_MAGIC}.store(tlb);
  wildcard.store(tlb);
}

// ===== HTTP =====

bool K2InvokeHttp::fetch(tl::TLBuffer& tlb) noexcept {
  tl::details::magic magic{};
  tl::details::mask flags{};
  bool ok{magic.fetch(tlb) && magic.expect(K2_INVOKE_HTTP_MAGIC)};
  ok &= flags.fetch(tlb);
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

// ===== RPC =====

bool rpcInvokeReq::fetch(tl::TLBuffer& tlb) noexcept {
  bool ok{query_id.fetch(tlb)};
  bool fetched{};
  while (ok && !fetched) {
    const auto magic{tlb.lookup_trivial<uint32_t>().value_or(TL_ZERO)};
    switch (magic) {
    case TL_RPC_DEST_ACTOR: {
      ok &= !opt_dest_actor.has_value() && opt_dest_actor.emplace().fetch(tlb);
      break;
    }
    case TL_RPC_DEST_FLAGS: {
      ok &= !opt_dest_flags.has_value() && opt_dest_flags.emplace().fetch(tlb);
      break;
    }
    case TL_RPC_DEST_ACTOR_FLAGS: {
      ok &= !opt_dest_actor_flags.has_value() && opt_dest_actor_flags.emplace().fetch(tlb);
      break;
    }
    default: {
      const auto opt_query{tlb.fetch_bytes(tlb.remaining())};
      query = opt_query.value_or(std::string_view{});
      ok &= opt_query.has_value();
      fetched = true;
      break;
    }
    }
  }
  return ok;
}

} // namespace tl
