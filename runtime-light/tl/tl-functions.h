// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-types.h"

namespace tl {

// ===== JOB WORKERS =====

inline constexpr uint32_t K2_INVOKE_JOB_WORKER_MAGIC = 0x437d'7312;

class K2InvokeJobWorker final {
  static constexpr auto IGNORE_ANSWER_FLAG = static_cast<uint32_t>(1U << 0U);

public:
  uint64_t image_id{};
  int64_t job_id{};
  bool ignore_answer{};
  uint64_t timeout_ns{};
  std::string_view body;

  bool fetch(TLBuffer &tlb) noexcept;

  void store(TLBuffer &tlb) const noexcept;
};

// ===== CRYPTO =====

inline constexpr uint32_t GET_CRYPTOSECURE_PSEUDORANDOM_BYTES_MAGIC = 0x2491'b81d;
inline constexpr uint32_t GET_PEM_CERT_INFO_MAGIC = 0xa50c'fd6c;
inline constexpr uint32_t DIGEST_SIGN_MAGIC = 0xd345'f658;
inline constexpr uint32_t DIGEST_VERIFY_MAGIC = 0x5760'bd0e;
inline constexpr uint32_t CBC_DECRYPT_MAGIC = 0x7f2e'e1e4;
inline constexpr uint32_t CBC_ENCRYPT_MAGIC = 0x6d4e'e36a;

struct GetCryptosecurePseudorandomBytes final {
  int32_t size{};

  void store(TLBuffer &tlb) const noexcept;
};

struct GetPemCertInfo final {
  bool is_short{true};
  string bytes;

  void store(TLBuffer &tlb) const noexcept;
};

struct DigestSign final {
  string data;
  string private_key;
  DigestAlgorithm algorithm;

  void store(TLBuffer &tlb) const noexcept;
};

struct DigestVerify final {
  string data;
  string public_key;
  DigestAlgorithm algorithm;
  string signature;

  void store(TLBuffer &tlb) const noexcept;
};

struct CbcDecrypt final {
  CipherAlgorithm algorithm;
  BlockPadding padding;
  string passphrase;
  string iv;
  string data;

  void store(TLBuffer &tlb) const noexcept;
};

struct CbcEncrypt final {
  CipherAlgorithm algorithm;
  BlockPadding padding;
  string passphrase;
  string iv;
  string data;

  void store(TLBuffer &tlb) const noexcept;
};

// ===== CONFDATA =====

inline constexpr uint32_t CONFDATA_GET_MAGIC = 0xf0eb'cd89;
inline constexpr uint32_t CONFDATA_GET_WILDCARD_MAGIC = 0x5759'bd9e;

struct ConfdataGet final {
  std::string_view key;

  void store(TLBuffer &tlb) const noexcept;
};

struct ConfdataGetWildcard final {
  std::string_view wildcard;

  void store(TLBuffer &tlb) const noexcept;
};

// ===== HTTP =====

inline constexpr uint32_t K2_INVOKE_HTTP_MAGIC = 0xd909'efe8;

class K2InvokeHttp final {
  static constexpr auto SCHEME_FLAG = static_cast<uint32_t>(1U << 0U);
  static constexpr auto HOST_FLAG = static_cast<uint32_t>(1U << 1U);
  static constexpr auto QUERY_FLAG = static_cast<uint32_t>(1U << 2U);

public:
  httpConnection connection{};
  HttpVersion version{};
  string method;
  httpUri uri{};
  vector<httpHeaderEntry> headers{};
  string body;

  bool fetch(TLBuffer &tlb) noexcept;
};

} // namespace tl
