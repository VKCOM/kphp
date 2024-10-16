// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/tl/tl-functions.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "common/tl/constants/common.h"
#include "runtime-light/tl/tl-core.h"

namespace {

constexpr auto K2_JOB_WORKER_IGNORE_ANSWER_FLAG = static_cast<uint32_t>(1U << 0U);

} // namespace

namespace tl {

bool K2InvokeJobWorker::fetch(TLBuffer &tlb) noexcept {
  if (tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) != K2_INVOKE_JOB_WORKER_MAGIC) {
    return false;
  }

  const auto opt_flags{tlb.fetch_trivial<uint32_t>()};
  const auto opt_image_id{tlb.fetch_trivial<uint64_t>()};
  const auto opt_job_id{tlb.fetch_trivial<int64_t>()};
  const auto opt_timeout_ns{tlb.fetch_trivial<uint64_t>()};
  if (!(opt_flags.has_value() && opt_image_id.has_value() && opt_job_id.has_value() && opt_timeout_ns.has_value())) {
    return false;
  }
  const auto body_view{tlb.fetch_string()};

  ignore_answer = static_cast<bool>(*opt_flags & K2_JOB_WORKER_IGNORE_ANSWER_FLAG);
  image_id = *opt_image_id;
  job_id = *opt_job_id;
  timeout_ns = *opt_timeout_ns;
  body = {body_view.data(), static_cast<string::size_type>(body_view.size())};
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

void DigestSign::store(TLBuffer &tlb) const noexcept {
  tlb.store_trivial<uint32_t>(DIGEST_SIGN_MAGIC);
  tlb.store_string(std::string_view{data.c_str(), data.size()});
  tlb.store_string(std::string_view{private_key.c_str(), private_key.size()});
  tlb.store_trivial<uint32_t>(algorithm);
}

void DigestVerify::store(TLBuffer &tlb) const noexcept {
  tlb.store_trivial<uint32_t>(DIGEST_VERIFY_MAGIC);
  tlb.store_string(std::string_view{data.c_str(), data.size()});
  tlb.store_string(std::string_view{public_key.c_str(), public_key.size()});
  tlb.store_trivial<uint32_t>(algorithm);
  tlb.store_string(std::string_view{signature.c_str(), signature.size()});
}

void ConfdataGet::store(TLBuffer &tlb) const noexcept {
  tlb.store_trivial<uint32_t>(CONFDATA_GET_MAGIC);
  tlb.store_string({key.c_str(), static_cast<size_t>(key.size())});
}

void ConfdataGetWildcard::store(TLBuffer &tlb) const noexcept {
  tlb.store_trivial<uint32_t>(CONFDATA_GET_WILDCARD_MAGIC);
  tlb.store_string({wildcard.c_str(), static_cast<size_t>(wildcard.size())});
}

// ===== HTTP =====

bool K2InvokeHttp::fetch(TLBuffer &tlb) noexcept {
  if (tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) != K2_INVOKE_HTTP_MAGIC) {
    return false;
  }
  const auto opt_flags{tlb.fetch_trivial<uint32_t>()};
  if (!opt_flags.has_value() || !http_version.fetch(tlb)) {
    return false;
  }

  const auto flags{*opt_flags};
  const auto method_view{tlb.fetch_string()};
  method = {method_view.data(), static_cast<string::size_type>(method_view.size())};
  if (static_cast<bool>(flags & SCHEME_FLAG)) {
    const auto scheme_view{tlb.fetch_string()};
    opt_scheme.emplace(scheme_view.data(), static_cast<string::size_type>(scheme_view.size()));
  }
  if (static_cast<bool>(flags & HOST_FLAG)) {
    const auto host_view{tlb.fetch_string()};
    opt_host.emplace(host_view.data(), static_cast<string::size_type>(host_view.size()));
  }
  const auto path_view{tlb.fetch_string()};
  path = {path_view.data(), static_cast<string::size_type>(path_view.size())};
  if (static_cast<bool>(flags & QUERY_FLAG)) {
    const auto query_view{tlb.fetch_string()};
    opt_query.emplace(query_view.data(), static_cast<string::size_type>(query_view.size()));
  }
  if (!headers.fetch(tlb)) {
    return false;
  }
  const auto body_view{tlb.fetch_bytes(tlb.remaining())};
  body = {body_view.data(), static_cast<string::size_type>(body_view.size())};

  return true;
}

} // namespace tl
