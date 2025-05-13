// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-types.h"

namespace tl {

// ===== JOB WORKERS =====

inline constexpr uint32_t K2_INVOKE_JOB_WORKER_MAGIC = 0x437d'7312;

class K2InvokeJobWorker final {
  static constexpr auto IGNORE_ANSWER_FLAG = static_cast<uint32_t>(1U << 0U);

public:
  tl::i64 image_id{};
  tl::i64 job_id{};
  bool ignore_answer{};
  tl::i64 timeout_ns{};
  tl::string body;

  bool fetch(tl::TLBuffer& tlb) noexcept;

  void store(tl::TLBuffer& tlb) const noexcept;
};

// ===== CRYPTO =====

inline constexpr uint32_t GET_CRYPTOSECURE_PSEUDORANDOM_BYTES_MAGIC = 0x2491'b81d;
inline constexpr uint32_t GET_PEM_CERT_INFO_MAGIC = 0xa50c'fd6c;
inline constexpr uint32_t DIGEST_SIGN_MAGIC = 0xd345'f658;
inline constexpr uint32_t DIGEST_VERIFY_MAGIC = 0x5760'bd0e;
inline constexpr uint32_t CBC_DECRYPT_MAGIC = 0x7f2e'e1e4;
inline constexpr uint32_t CBC_ENCRYPT_MAGIC = 0x6d4e'e36a;
inline constexpr uint32_t HASH_MAGIC = 0x5073'2a27;
inline constexpr uint32_t HASH_HMAC_MAGIC = 0x8dcb'3d9d;

struct GetCryptosecurePseudorandomBytes final {
  tl::i32 size{};

  void store(tl::TLBuffer& tlb) const noexcept;
};

struct GetPemCertInfo final {
  bool is_short{true};
  tl::string bytes;

  void store(tl::TLBuffer& tlb) const noexcept;
};

struct DigestSign final {
  tl::string data;
  tl::string private_key;
  tl::HashAlgorithm algorithm{};

  void store(tl::TLBuffer& tlb) const noexcept;
};

struct DigestVerify final {
  tl::string data;
  tl::string public_key;
  tl::HashAlgorithm algorithm{};
  tl::string signature;

  void store(tl::TLBuffer& tlb) const noexcept;
};

struct CbcDecrypt final {
  tl::CipherAlgorithm algorithm{};
  tl::BlockPadding padding{};
  tl::string passphrase;
  tl::string iv;
  tl::string data;

  void store(tl::TLBuffer& tlb) const noexcept;
};

struct CbcEncrypt final {
  tl::CipherAlgorithm algorithm{};
  tl::BlockPadding padding{};
  tl::string passphrase;
  tl::string iv;
  tl::string data;

  void store(tl::TLBuffer& tlb) const noexcept;
};

struct Hash final {
  tl::HashAlgorithm algorithm{};
  tl::string data;

  void store(tl::TLBuffer& tlb) const noexcept;
};

struct HashHmac final {
  tl::HashAlgorithm algorithm{};
  tl::string data;
  tl::string secret_key;

  void store(tl::TLBuffer& tlb) const noexcept;
};

// ===== CONFDATA =====

inline constexpr uint32_t CONFDATA_GET_MAGIC = 0xf0eb'cd89;
inline constexpr uint32_t CONFDATA_GET_WILDCARD_MAGIC = 0x5759'bd9e;

struct ConfdataGet final {
  tl::string key;

  void store(tl::TLBuffer& tlb) const noexcept;
};

struct ConfdataGetWildcard final {
  tl::string wildcard;

  void store(tl::TLBuffer& tlb) const noexcept;
};

// ===== HTTP =====

inline constexpr uint32_t K2_INVOKE_HTTP_MAGIC = 0xd909'efe8;

class K2InvokeHttp final {
  static constexpr auto SCHEME_FLAG = static_cast<uint32_t>(1U << 0U);
  static constexpr auto HOST_FLAG = static_cast<uint32_t>(1U << 1U);
  static constexpr auto QUERY_FLAG = static_cast<uint32_t>(1U << 2U);

public:
  tl::httpConnection connection{};
  tl::HttpVersion version{};
  tl::string method;
  tl::httpUri uri{};
  tl::vector<tl::httpHeaderEntry> headers{};
  std::string_view body;

  bool fetch(tl::TLBuffer& tlb) noexcept;
};

// ===== RPC =====

inline constexpr uint32_t K2_INVOKE_RPC_MAGIC = 0xd909'efe9;

class K2InvokeRpc final {
  static constexpr auto ACTOR_ID_FLAG = static_cast<uint32_t>(1U << 0U);
  static constexpr auto EXTRA_FLAG = static_cast<uint32_t>(1U << 1U);

public:
  tl::details::mask flags{};
  tl::i64 query_id{};
  tl::netPid net_pid{};
  std::optional<tl::i64> opt_actor_id;
  std::optional<tl::rpcInvokeReqExtra> opt_extra;
  std::string_view query;

  bool fetch(tl::TLBuffer& tlb) noexcept {
    tl::details::magic magic{};
    bool ok{magic.fetch(tlb) && magic.expect(K2_INVOKE_RPC_MAGIC)};
    ok &= flags.fetch(tlb);
    ok &= query_id.fetch(tlb);
    ok &= net_pid.fetch(tlb);
    if (static_cast<bool>(flags.value & ACTOR_ID_FLAG)) {
      ok &= opt_actor_id.emplace().fetch(tlb);
    }
    if (static_cast<bool>(flags.value & EXTRA_FLAG)) {
      ok &= opt_extra.emplace().fetch(tlb);
    }
    const auto opt_query{tlb.fetch_bytes(tlb.remaining())};
    query = opt_query.value_or(std::string_view{});
    return ok && opt_query.has_value();
  }
};

// ===== CACHE =====

inline constexpr uint32_t CACHE_STORE_MAGIC = 0x41e9'38a8;
inline constexpr uint32_t CACHE_UPDATE_TTL_MAGIC = 0x52cb'905c;
inline constexpr uint32_t CACHE_DELETE_MAGIC = 0x71a9'5f2b;
inline constexpr uint32_t CACHE_FETCH_MAGIC = 0xbd29'd526;

struct CacheStore final {
  tl::string key;
  tl::string value;
  tl::u32 ttl;

  void store(TLBuffer& tlb) const noexcept {
    tl::details::magic{.value = CACHE_STORE_MAGIC}.store(tlb);
    key.store(tlb);
    value.store(tlb);
    ttl.store(tlb);
  }
};

struct CacheUpdateTtl final {
  tl::string key;
  tl::u32 ttl;

  void store(TLBuffer& tlb) const noexcept {
    tl::details::magic{.value = CACHE_UPDATE_TTL_MAGIC}.store(tlb);
    key.store(tlb);
    ttl.store(tlb);
  }
};

struct CacheDelete final {
  tl::string key;

  void store(TLBuffer& tlb) const noexcept {
    tl::details::magic{.value = CACHE_DELETE_MAGIC}.store(tlb);
    key.store(tlb);
  }
};

struct CacheFetch final {
  tl::string key;

  void store(TLBuffer& tlb) const noexcept {
    tl::details::magic{.value = CACHE_FETCH_MAGIC}.store(tlb);
    key.store(tlb);
  }
};

} // namespace tl
