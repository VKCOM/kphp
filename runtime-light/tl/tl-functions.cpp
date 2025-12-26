// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/tl/tl-functions.h"

#include <cstddef>
#include <span>

#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-types.h"

namespace tl {

// ===== JOB WORKERS =====

bool K2InvokeJobWorker::fetch(tl::fetcher& tlf) noexcept {
  tl::magic magic{};
  tl::mask flags{};
  bool ok{magic.fetch(tlf) && magic.expect(K2_INVOKE_JOB_WORKER_MAGIC)};
  ok = ok && flags.fetch(tlf);
  ok = ok && image_id.fetch(tlf);
  ok = ok && job_id.fetch(tlf);
  ok = ok && timeout_ns.fetch(tlf);
  ok = ok && body.fetch(tlf);
  ignore_answer = static_cast<bool>(flags.value & IGNORE_ANSWER_FLAG);
  return ok;
}

void K2InvokeJobWorker::store(tl::storer& tls) const noexcept {
  tl::magic{.value = K2_INVOKE_JOB_WORKER_MAGIC}.store(tls);
  tl::mask{.value = ignore_answer ? IGNORE_ANSWER_FLAG : 0x0}.store(tls);
  image_id.store(tls);
  job_id.store(tls);
  timeout_ns.store(tls);
  body.store(tls);
}

// ===== HTTP =====

bool K2InvokeHttp::fetch(tl::fetcher& tlf) noexcept {
  tl::magic magic{};
  tl::mask flags{};
  bool ok{magic.fetch(tlf) && magic.expect(K2_INVOKE_HTTP_MAGIC)};
  ok = ok && flags.fetch(tlf);
  ok = ok && connection.fetch(tlf);
  ok = ok && version.fetch(tlf);
  ok = ok && method.fetch(tlf);
  ok = ok && uri.fetch(tlf);
  ok = ok && headers.fetch(tlf);
  const auto opt_body{tlf.fetch_bytes(tlf.remaining())};
  ok = ok && opt_body.has_value();

  body = opt_body.value_or(std::span<const std::byte>{});

  return ok;
}

} // namespace tl
