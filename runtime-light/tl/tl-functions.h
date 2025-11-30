// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

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

  bool fetch(tl::fetcher& tlf) noexcept;

  void store(tl::storer& tls) const noexcept;
};

// ===== CRYPTO =====

inline constexpr uint32_t GET_CRYPTOSECURE_PSEUDORANDOM_BYTES_MAGIC = 0x2491'b81d;
inline constexpr uint32_t GET_PEM_CERT_INFO_MAGIC = 0xa50c'fd6c;
inline constexpr uint32_t DIGEST_SIGN_MAGIC = 0xd345'f658;
inline constexpr uint32_t DIGEST_VERIFY_MAGIC = 0x5760'bd0e;
inline constexpr uint32_t CBC_DECRYPT_MAGIC = 0x7f2e'e1e4;
inline constexpr uint32_t CBC_ENCRYPT_MAGIC = 0x6d4e'e36a;
inline constexpr uint32_t GET_PUBLIC_KEY_MAGIC = 0x4b1e'7d3d;
inline constexpr uint32_t GET_PRIVATE_KEY_MAGIC = 0x34ea'dfdb;
inline constexpr uint32_t PUBLIC_ENCRYPT_MAGIC = 0x7612'f4ad;
inline constexpr uint32_t PRIVATE_DECRYPT_MAGIC = 0xc6e9'1d4d;
inline constexpr uint32_t HASH_MAGIC = 0x5073'2a27;
inline constexpr uint32_t HASH_HMAC_MAGIC = 0x8dcb'3d9d;

struct GetCryptosecurePseudorandomBytes final {
  tl::i32 size{};

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = GET_CRYPTOSECURE_PSEUDORANDOM_BYTES_MAGIC}.store(tls);
    size.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = GET_CRYPTOSECURE_PSEUDORANDOM_BYTES_MAGIC}.footprint() + size.footprint();
  }
};

struct GetPemCertInfo final {
  bool is_short{true};
  tl::string bytes;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = GET_PEM_CERT_INFO_MAGIC}.store(tls);
    tl::Bool{.value = is_short}.store(tls);
    bytes.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = GET_PEM_CERT_INFO_MAGIC}.footprint() + tl::Bool{.value = is_short}.footprint() + bytes.footprint();
  }
};

struct DigestSign final {
  tl::string data;
  tl::string private_key;
  tl::HashAlgorithm algorithm{};

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = DIGEST_SIGN_MAGIC}.store(tls);
    data.store(tls);
    private_key.store(tls);
    tls.store_trivial<uint32_t>(algorithm);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = DIGEST_SIGN_MAGIC}.footprint() + data.footprint() + private_key.footprint() + sizeof(uint32_t);
  }
};

struct DigestVerify final {
  tl::string data;
  tl::string public_key;
  tl::HashAlgorithm algorithm{};
  tl::string signature;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = DIGEST_VERIFY_MAGIC}.store(tls);
    data.store(tls);
    public_key.store(tls);
    tls.store_trivial<uint32_t>(algorithm);
    signature.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = DIGEST_VERIFY_MAGIC}.footprint() + data.footprint() + public_key.footprint() + sizeof(uint32_t) + signature.footprint();
  }
};

struct CbcDecrypt final {
  tl::CipherAlgorithm algorithm{};
  tl::BlockPadding padding{};
  tl::string passphrase;
  tl::string iv;
  tl::string data;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = CBC_DECRYPT_MAGIC}.store(tls);
    tls.store_trivial<uint32_t>(algorithm);
    tls.store_trivial<uint32_t>(padding);
    passphrase.store(tls);
    iv.store(tls);
    data.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = CBC_DECRYPT_MAGIC}.footprint() + sizeof(uint32_t) + sizeof(uint32_t) + passphrase.footprint() + iv.footprint() + data.footprint();
  }
};

struct CbcEncrypt final {
  tl::CipherAlgorithm algorithm{};
  tl::BlockPadding padding{};
  tl::string passphrase;
  tl::string iv;
  tl::string data;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = CBC_ENCRYPT_MAGIC}.store(tls);
    tls.store_trivial<uint32_t>(algorithm);
    tls.store_trivial<uint32_t>(padding);
    passphrase.store(tls);
    iv.store(tls);
    data.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = CBC_ENCRYPT_MAGIC}.footprint() + sizeof(uint32_t) + sizeof(uint32_t) + passphrase.footprint() + iv.footprint() + data.footprint();
  }
};

struct GetPublicKey final {
  tl::string key;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = GET_PUBLIC_KEY_MAGIC}.store(tls);
    key.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = GET_PUBLIC_KEY_MAGIC}.footprint() + key.footprint();
  }
};

struct GetPrivateKey final {
  tl::string key;
  tl::string passphrase;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = GET_PRIVATE_KEY_MAGIC}.store(tls);
    key.store(tls);
    passphrase.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = GET_PRIVATE_KEY_MAGIC}.footprint() + key.footprint() + passphrase.footprint();
  }
};

struct PublicEncrypt final {
  tl::string key;
  tl::string data;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = PUBLIC_ENCRYPT_MAGIC}.store(tls);
    key.store(tls);
    data.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = PUBLIC_ENCRYPT_MAGIC}.footprint() + key.footprint() + data.footprint();
  }
};

struct PrivateDecrypt final {
  tl::string key;
  tl::string data;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = PRIVATE_DECRYPT_MAGIC}.store(tls);
    key.store(tls);
    data.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = PRIVATE_DECRYPT_MAGIC}.footprint() + key.footprint() + data.footprint();
  }
};

struct Hash final {
  tl::HashAlgorithm algorithm{};
  tl::string data;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = HASH_MAGIC}.store(tls);
    tls.store_trivial<uint32_t>(algorithm);
    data.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = HASH_MAGIC}.footprint() + sizeof(uint32_t) + data.footprint();
  }
};

struct HashHmac final {
  tl::HashAlgorithm algorithm{};
  tl::string data;
  tl::string secret_key;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = HASH_HMAC_MAGIC}.store(tls);
    tls.store_trivial<uint32_t>(algorithm);
    data.store(tls);
    secret_key.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = HASH_HMAC_MAGIC}.footprint() + sizeof(uint32_t) + data.footprint() + secret_key.footprint();
  }
};

// ===== CONFDATA =====

inline constexpr uint32_t CONFDATA_GET_MAGIC = 0xf0eb'cd89;
inline constexpr uint32_t CONFDATA_GET_WILDCARD_MAGIC = 0x5759'bd9e;

struct ConfdataGet final {
  tl::string key;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = CONFDATA_GET_MAGIC}.store(tls);
    key.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = CONFDATA_GET_MAGIC}.footprint() + key.footprint();
  }
};

struct ConfdataGetWildcard final {
  tl::string wildcard;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = CONFDATA_GET_WILDCARD_MAGIC}.store(tls);
    wildcard.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = CONFDATA_GET_WILDCARD_MAGIC}.footprint() + wildcard.footprint();
  }
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
  std::span<const std::byte> body;

  bool fetch(tl::fetcher& tlf) noexcept;
};

// ===== RPC =====

inline constexpr uint32_t K2_INVOKE_RPC_MAGIC = 0x80c3'7baa;

class K2InvokeRpc final {
  static constexpr auto ACTOR_ID_FLAG = static_cast<uint32_t>(1U << 0U);
  static constexpr auto EXTRA_FLAG = static_cast<uint32_t>(1U << 1U);

public:
  tl::mask flags{};
  tl::i64 query_id{};
  tl::netPid net_pid{};
  std::optional<tl::i64> opt_actor_id;
  std::optional<tl::rpcInvokeReqExtra> opt_extra;
  std::span<const std::byte> query;

  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    bool ok{magic.fetch(tlf) && magic.expect(K2_INVOKE_RPC_MAGIC)};
    ok &= flags.fetch(tlf);
    ok &= query_id.fetch(tlf);
    ok &= net_pid.fetch(tlf);
    if (static_cast<bool>(flags.value & ACTOR_ID_FLAG)) {
      ok &= opt_actor_id.emplace().fetch(tlf);
    }
    if (static_cast<bool>(flags.value & EXTRA_FLAG)) {
      ok &= opt_extra.emplace().fetch(tlf);
    }
    const auto opt_query{tlf.fetch_bytes(tlf.remaining())};
    query = opt_query.value_or(std::span<const std::byte>{});
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

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = CACHE_STORE_MAGIC}.store(tls);
    key.store(tls);
    value.store(tls);
    ttl.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = CACHE_STORE_MAGIC}.footprint() + key.footprint() + value.footprint() + ttl.footprint();
  }
};

struct CacheUpdateTtl final {
  tl::string key;
  tl::u32 ttl;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = CACHE_UPDATE_TTL_MAGIC}.store(tls);
    key.store(tls);
    ttl.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = CACHE_UPDATE_TTL_MAGIC}.footprint() + key.footprint() + ttl.footprint();
  }
};

struct CacheDelete final {
  tl::string key;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = CACHE_DELETE_MAGIC}.store(tls);
    key.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = CACHE_DELETE_MAGIC}.footprint() + key.footprint();
  }
};

struct CacheFetch final {
  tl::string key;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = CACHE_FETCH_MAGIC}.store(tls);
    key.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = CACHE_FETCH_MAGIC}.footprint() + key.footprint();
  }
};

// ===== WEB TRANSFER LIB =====

class WebTransferGetProperties final {
  static constexpr uint32_t WEB_TRANSFER_GET_PROPERTIES_MAGIC = 0x72B7'16DD;

public:
  tl::u8 is_simple;
  tl::u64 descriptor;
  tl::Maybe<tl::u64> property_id;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = WEB_TRANSFER_GET_PROPERTIES_MAGIC}.store(tls);
    is_simple.store(tls);
    descriptor.store(tls);
    property_id.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = WEB_TRANSFER_GET_PROPERTIES_MAGIC}.footprint() + is_simple.footprint() + descriptor.footprint() + property_id.footprint();
  }
};

// ===== Simple =====

class SimpleWebTransferOpen final {
  static constexpr uint32_t SIMPLE_WEB_TRANSFER_OPEN_MAGIC = 0x24F8'98AA;

public:
  using web_backend_type = tl::u8;
  web_backend_type web_backend;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = SIMPLE_WEB_TRANSFER_OPEN_MAGIC}.store(tls);
    web_backend.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = SIMPLE_WEB_TRANSFER_OPEN_MAGIC}.footprint() + web_backend.footprint();
  }
};

class SimpleWebTransferPerform final {
  static constexpr uint32_t SIMPLE_WEB_TRANSFER_PERFORM_MAGIC = 0x24B8'98CC;

public:
  tl::u64 descriptor;
  tl::simpleWebTransferConfig config;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = SIMPLE_WEB_TRANSFER_PERFORM_MAGIC}.store(tls);
    descriptor.store(tls);
    config.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = SIMPLE_WEB_TRANSFER_PERFORM_MAGIC}.footprint() + descriptor.footprint() + config.footprint();
  }
};

class SimpleWebTransferGetResponse final {
  static constexpr uint32_t SIMPLE_WEB_TRANSFER_GET_RESPONSE_MAGIC = 0xAADD'FF24;

public:
  tl::u64 descriptor;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = SIMPLE_WEB_TRANSFER_GET_RESPONSE_MAGIC}.store(tls);
    descriptor.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = SIMPLE_WEB_TRANSFER_GET_RESPONSE_MAGIC}.footprint() + descriptor.footprint();
  }
};

class SimpleWebTransferClose final {
  static constexpr uint32_t SIMPLE_WEB_TRANSFER_CLOSE_MAGIC = 0x36F7'16BB;

public:
  tl::u64 descriptor;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = SIMPLE_WEB_TRANSFER_CLOSE_MAGIC}.store(tls);
    descriptor.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = SIMPLE_WEB_TRANSFER_CLOSE_MAGIC}.footprint() + descriptor.footprint();
  }
};

class SimpleWebTransferReset final {
  static constexpr uint32_t SIMPLE_WEB_TRANSFER_RESET_MAGIC = 0x36F8'98BB;

public:
  tl::u64 descriptor;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = SIMPLE_WEB_TRANSFER_RESET_MAGIC}.store(tls);
    descriptor.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = SIMPLE_WEB_TRANSFER_RESET_MAGIC}.footprint() + descriptor.footprint();
  }
};

// ===== Composite =====

class CompositeWebTransferOpen final {
  static constexpr uint32_t COMPOSITE_WEB_TRANSFER_OPEN_MAGIC = 0x428F'89FF;

public:
  using web_backend_type = tl::u8;
  web_backend_type web_backend;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = COMPOSITE_WEB_TRANSFER_OPEN_MAGIC}.store(tls);
    web_backend.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = COMPOSITE_WEB_TRANSFER_OPEN_MAGIC}.footprint() + web_backend.footprint();
  }
};

class CompositeWebTransferAdd final {
  static constexpr uint32_t COMPOSITE_WEB_TRANSFER_ADD_MAGIC = 0x1223'16CC;

public:
  tl::u64 composite_descriptor;
  tl::u64 simple_descriptor;
  tl::simpleWebTransferConfig simple_config;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = COMPOSITE_WEB_TRANSFER_ADD_MAGIC}.store(tls);
    composite_descriptor.store(tls);
    simple_descriptor.store(tls);
    simple_config.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = COMPOSITE_WEB_TRANSFER_ADD_MAGIC}.footprint() + composite_descriptor.footprint() + simple_descriptor.footprint() +
           simple_config.footprint();
  }
};

class CompositeWebTransferRemove final {
  static constexpr uint32_t COMPOSITE_WEB_TRANSFER_REMOVE_MAGIC = 0x8981'22AA;

public:
  tl::u64 composite_descriptor;
  tl::u64 simple_descriptor;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = COMPOSITE_WEB_TRANSFER_REMOVE_MAGIC}.store(tls);
    composite_descriptor.store(tls);
    simple_descriptor.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = COMPOSITE_WEB_TRANSFER_REMOVE_MAGIC}.footprint() + composite_descriptor.footprint() + simple_descriptor.footprint();
  }
};

class CompositeWebTransferPerform final {
  static constexpr uint32_t COMPOSITE_WEB_TRANSFER_PERFORM_MAGIC = 0xAA24'42BB;

public:
  tl::u64 descriptor;
  tl::compositeWebTransferConfig config;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = COMPOSITE_WEB_TRANSFER_PERFORM_MAGIC}.store(tls);
    descriptor.store(tls);
    config.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = COMPOSITE_WEB_TRANSFER_PERFORM_MAGIC}.footprint() + descriptor.footprint() + config.footprint();
  }
};

class CompositeWebTransferClose final {
  static constexpr uint32_t COMPOSITE_WEB_TRANSFER_CLOSE_MAGIC = 0x7162'22AB;

public:
  tl::u64 descriptor;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = COMPOSITE_WEB_TRANSFER_CLOSE_MAGIC}.store(tls);
    descriptor.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = COMPOSITE_WEB_TRANSFER_CLOSE_MAGIC}.footprint() + descriptor.footprint();
  }
};

} // namespace tl
