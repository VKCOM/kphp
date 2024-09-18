// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-core/runtime-core.h"
#include "runtime-light/tl/tl-core.h"

namespace tl {

// ===== JOB WORKERS =====

inline constexpr uint32_t K2_JOB_WORKER_RESPONSE_MAGIC = 0x3afb'3a08;

struct K2JobWorkerResponse final {
  int64_t job_id{};
  string body;

  bool fetch(TLBuffer &tlb) noexcept;

  void store(TLBuffer &tlb) const noexcept;
};

// ===== CRYPTO =====

// Actually it's "Maybe (Dictionary CertInfoItem)"
// But I now want to have this logic separately
struct GetPemCertInfoResponse {
  array<mixed> data;

  bool fetch(TLBuffer &tlb) noexcept;
};

enum DigestAlgorithm : uint32_t {
  DSS1 = 0xf572'd6b6,
  SHA1 = 0x215f'b97d,
  SHA224 = 0x8bce'55e9,
  SHA256 = 0x6c97'7f8c,
  SHA384 = 0xf54c'2608,
  SHA512 = 0x225d'f2b6,
  RMD160 = 0x1887'e6b4,
  MD5 = 0x257d'df13,
  MD4 = 0x317f'e3d1,
  MD2 = 0x5aca'6998,
};

} // namespace tl
