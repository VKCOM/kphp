//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/crypto/crypto-functions.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <functional>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <tuple>
#include <utility>

#include "common/algorithms/string-algorithms.h"
#include "common/crc32_generic.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/server/url-functions.h"
#include "runtime-common/stdlib/string/string-functions.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/crypto/crypto-state.h"
#include "runtime-light/stdlib/crypto/openssl.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

auto f$openssl_random_pseudo_bytes(int64_t length) noexcept -> Optional<string> {
  if (length <= 0 || length > string::max_size()) [[unlikely]] {
    return false;
  }

  string buffer{static_cast<string::size_type>(length), false};
  k2::SystemTime now{};
  k2::system_time(std::addressof(now));
  // Error occurs only if arguments are invalid.
  std::ignore = k2::openssl_rand_add(std::as_bytes(std::span{std::addressof(now), 1}), 0.0);
  if (!k2::openssl_rand_bytes(std::as_writable_bytes(std::span{buffer.buffer(), buffer.size()}))) [[unlikely]] {
    kphp::log::warning("failed to generate random bytes");
    return false;
  }

  return buffer;
}

namespace {

auto x509_get_name_entries(const kphp::openssl::x509_name& name, bool shortnames) noexcept -> array<mixed> {
  // These getters cannot fail for a parsed certificate (the old runtime does not check them either).
  const auto count{name.entry_count()};
  kphp::log::assertion(count.has_value());

  array<mixed> res{array_size{static_cast<int64_t>(*count), false}};
  for (int32_t i = 0; i < *count; ++i) {
    const auto entry{name.entry(i)};
    kphp::log::assertion(entry.has_value());
    const auto key_object{entry->object()};
    kphp::log::assertion(key_object.has_value());
    const auto value{entry->data()};
    kphp::log::assertion(value.has_value());
    const auto nid{key_object->obj2nid()};
    kphp::log::assertion(nid.has_value());
    const auto key_str{shortnames ? k2::openssl_obj_nid2sn(*nid) : k2::openssl_obj_nid2ln(*nid)};
    kphp::log::assertion(key_str.has_value());
    const auto value_data{value->data()};
    kphp::log::assertion(value_data.has_value());
    const auto value_len{value->length()};
    kphp::log::assertion(value_len.has_value());

    res.set_value(string{*key_str}, string{static_cast<const char*>(*value_data), static_cast<string::size_type>(*value_len)});
  }

  return res;
}

auto x509_get_purposes(const kphp::openssl::x509& cert, bool shortnames) noexcept -> array<mixed> {
  // These getters cannot fail for a parsed certificate (the old runtime does not check them either).
  const auto count{kphp::openssl::x509_purpose::count()};
  kphp::log::assertion(count.has_value());
  array<mixed> res{array_size{static_cast<int64_t>(*count), false}};
  for (int32_t i = 0; i < *count; ++i) {
    const auto purpose{kphp::openssl::x509_purpose::get(i)};
    kphp::log::assertion(purpose.has_value());
    const auto id{purpose->id()};
    kphp::log::assertion(id.has_value());
    const auto name{shortnames ? purpose->sname() : purpose->name()};
    kphp::log::assertion(name.has_value());
    const auto purpset_0{cert.check_purpose(*id, 0)};
    kphp::log::assertion(purpset_0.has_value());
    const auto purpset_1{cert.check_purpose(*id, 1)};
    kphp::log::assertion(purpset_1.has_value());

    res.set_value(static_cast<int64_t>(*id), array<mixed>::create(*purpset_0 != 0, *purpset_1 != 0, string{*name}));
  }

  return res;
}

auto convert_asn1_time(const std::optional<kphp::openssl::asn1_time>& epoch, const kphp::openssl::asn1_time& time) noexcept -> int64_t {
  const auto diff{epoch ? epoch->diff(time) : time.diff_from_now()};
  return diff ? int64_t{diff->days} * 24 * 60 * 60 + diff->seconds : 0;
}

} // namespace

auto f$openssl_x509_parse(const string& data, bool shortnames) noexcept -> Optional<array<mixed>> {
  const auto cert{kphp::openssl::x509::from_pem(std::as_bytes(std::span{data.c_str(), data.size()}))};
  if (!cert) {
    kphp::log::warning("failed to parse x509 certificate");
    return false;
  }

  // These getters cannot fail for a parsed certificate (the old runtime does not check them either).
  const auto subject_name{cert->subject_name()};
  kphp::log::assertion(subject_name.has_value());
  const auto issuer_name{cert->issuer_name()};
  kphp::log::assertion(issuer_name.has_value());
  const auto hash{cert->subject_name_hash()};
  kphp::log::assertion(hash.has_value());
  const auto version{cert->version()};
  kphp::log::assertion(version.has_value());

  const auto name{subject_name->oneline()};
  if (!name) [[unlikely]] {
    kphp::log::warning("failed to get X509 subject name");
    return false;
  }

  auto subject{x509_get_name_entries(*subject_name, shortnames)};
  auto issuer{x509_get_name_entries(*issuer_name, shortnames)};

  auto epoch{kphp::openssl::asn1_time::create()};
  std::optional<kphp::openssl::asn1_time> epoch_opt{std::nullopt};
  if (epoch) [[likely]] {
    std::ignore = epoch->set(0);
    epoch_opt = *std::move(epoch);
  }

  const auto not_before{cert->not_before()};
  kphp::log::assertion(not_before.has_value());
  const auto not_after{cert->not_after()};
  kphp::log::assertion(not_after.has_value());

  const auto valid_from{convert_asn1_time(epoch_opt, *not_before)};
  const auto valid_to{convert_asn1_time(epoch_opt, *not_after)};

  auto purposes{x509_get_purposes(*cert, shortnames)};

  const auto& crypto_state{kphp::openssl::crypto_image_state::get()};

  const std::array<char, 4> hash_bytes{static_cast<char>(*hash >> 24), static_cast<char>(*hash >> 16), static_cast<char>(*hash >> 8), static_cast<char>(*hash)};

  return std::initializer_list<std::pair<string, mixed>>{
      {crypto_state.NAME_STR, string{name->c_str()}},
      {crypto_state.SUBJECT_STR, std::move(subject)},
      {crypto_state.HASH_STR, kphp::strings::bin2hex(std::string_view{hash_bytes.data(), hash_bytes.size()})},
      {crypto_state.ISSUER_STR, std::move(issuer)},
      {crypto_state.VERSION_STR, *version},
      {crypto_state.VALID_FROM_TIME_T_STR, valid_from},
      {crypto_state.VALID_TO_TIME_T_STR, valid_to},
      {crypto_state.PURPOSES_STR, std::move(purposes)}};
}

namespace {

// A key argument is either a handle returned by openssl_pkey_get_{public,private} or a PEM string.
// A handle is the kind of the key followed by the raw bytes of a platform descriptor, which lives until the end of the instance.
// The kind keeps a public handle from being used as a private key and vice versa.
enum class pkey_kind : char { PUBLIC = 'P', PRIVATE = 'S' };

constexpr size_t PKEY_HANDLE_SIZE{sizeof(pkey_kind) + sizeof(k2::descriptor)};

auto pkey_from_handle(pkey_kind kind, const string& key) noexcept -> std::optional<kphp::openssl::pkey> {
  if (key.size() != PKEY_HANDLE_SIZE || key[0] != static_cast<char>(kind)) {
    return std::nullopt;
  }

  k2::descriptor descriptor{};
  std::memcpy(std::addressof(descriptor), key.c_str() + sizeof(pkey_kind), sizeof(descriptor));

  auto handle{kphp::openssl::pkey::borrow(descriptor)};
  if (!handle.id()) {
    return std::nullopt;
  }

  return handle;
}

// Gives the key to the instance: the platform frees its descriptor when the instance ends.
auto make_pkey_handle(pkey_kind kind, kphp::openssl::pkey key) noexcept -> string {
  const auto descriptor{key.release()};

  string handle{static_cast<string::size_type>(PKEY_HANDLE_SIZE), false};
  handle[0] = static_cast<char>(kind);
  std::memcpy(handle.buffer() + sizeof(pkey_kind), std::addressof(descriptor), sizeof(descriptor));

  return handle;
}

// Accepts either a certificate, in which case the key is taken from it, or a public key.
auto parse_public_key(const string& key) noexcept -> std::expected<kphp::openssl::pkey, int32_t> {
  const auto pem{std::as_bytes(std::span{key.c_str(), key.size()})};
  if (const auto cert{kphp::openssl::x509::from_pem(pem)}; cert) {
    return cert->pubkey();
  }

  return kphp::openssl::pkey::public_from_pem(pem);
}

auto load_public_key(const string& key) noexcept -> std::expected<kphp::openssl::pkey, int32_t> {
  if (auto pkey{pkey_from_handle(pkey_kind::PUBLIC, key)}; pkey) {
    return std::move(*pkey);
  }

  return parse_public_key(key);
}

auto parse_private_key(const string& key, const string& passphrase) noexcept -> std::expected<kphp::openssl::pkey, int32_t> {
  return kphp::openssl::pkey::private_from_pem(std::as_bytes(std::span{key.c_str(), key.size()}),
                                               std::as_bytes(std::span{passphrase.c_str(), passphrase.size()}));
}

auto load_private_key(const string& key) noexcept -> std::expected<kphp::openssl::pkey, int32_t> {
  if (auto pkey{pkey_from_handle(pkey_kind::PRIVATE, key)}; pkey) {
    return std::move(*pkey);
  }

  return parse_private_key(key, string{});
}

auto get_digest_algorithm(int64_t algorithm) noexcept -> std::optional<k2::DigestAlgorithm> {
  if (algorithm < 0 || algorithm >= k2::DigestAlgorithm::COUNT_DIGEST) {
    return std::nullopt;
  }

  return static_cast<k2::DigestAlgorithm>(algorithm);
}

} // namespace

auto f$openssl_sign(const string& data, string& signature, const string& private_key, int64_t algorithm) noexcept -> bool {
  const auto digest{get_digest_algorithm(algorithm)};
  if (!digest) [[unlikely]] {
    kphp::log::warning("unknown signature algorithm");
    return false;
  }

  const auto key{load_private_key(private_key)};
  if (!key) [[unlikely]] {
    kphp::log::warning("invalid private key");
    return false;
  }

  // Reuse md_ctx from cache to avoid allocation
  // It's safe, because we call init, that resets md_ctx state.
  const auto ctx{kphp::openssl::crypto_instance_state::get().md_ctx()};
  if (const auto size{key->size()}; size) {
    string buffer{static_cast<string::size_type>(*size), false};
    const auto sig_len{ctx.and_then([&](const kphp::openssl::md_ctx* c) noexcept { return c->sign_init(*digest); })
                           .and_then([&]() noexcept { return (*ctx)->sign_update(std::as_bytes(std::span{data.c_str(), data.size()})); })
                           .and_then([&]() noexcept { return (*ctx)->sign_final(std::as_writable_bytes(std::span{buffer.buffer(), buffer.size()}), *key); })};
    if (sig_len) {
      buffer.shrink(static_cast<string::size_type>(*sig_len));
      signature = std::move(buffer);
      return true;
    }
  }

  signature = string{};
  kphp::log::warning("failed to create signature");

  return false;
}

auto f$openssl_verify(const string& data, const string& signature, const string& public_key, int64_t algorithm) noexcept -> int64_t {
  const auto digest{get_digest_algorithm(algorithm)};
  if (!digest) [[unlikely]] {
    kphp::log::warning("unknown signature algorithm");
    return 0;
  }

  const auto key{load_public_key(public_key)};
  if (!key) [[unlikely]] {
    kphp::log::warning("invalid public key");
    return 0;
  }

  int32_t openssl_err{0};
  // Reuse md_ctx from cache to avoid allocation
  // It's safe, because we call init, that resets md_ctx state.
  const auto ctx{kphp::openssl::crypto_instance_state::get().md_ctx()};
  const auto verified{ctx.and_then([&](const kphp::openssl::md_ctx* c) noexcept { return c->verify_init(*digest); })
                          .and_then([&]() noexcept { return (*ctx)->verify_update(std::as_bytes(std::span{data.c_str(), data.size()})); })
                          .and_then([&]() noexcept {
                            return (*ctx)->verify_final(std::as_bytes(std::span{signature.c_str(), signature.size()}), *key, std::addressof(openssl_err));
                          })};

  if (verified) {
    return static_cast<int64_t>(*verified);
  }

  kphp::log::warning("failed to verify signature");

  return openssl_err;
}

namespace {

constexpr int64_t OPENSSL_RAW_DATA{1};
constexpr int64_t OPENSSL_ZERO_PADDING{2};
constexpr int64_t OPENSSL_DONT_ZERO_PAD_KEY{4};

constexpr auto CIPHER_ALGORITHMS{[]() noexcept {
  auto arr{std::array<std::string_view, 4>{"aes-128-cbc", "aes-256-cbc", "aes-128-gcm", "aes-256-gcm"}};
  std::ranges::sort(arr);
  return arr;
}()};

auto find_cipher_algorithm(std::string_view name) noexcept -> std::optional<std::string_view> {
  const auto lowered{std::ranges::transform_view{name, vk::tolower_ascii}};
  const auto found{std::ranges::equal_range(CIPHER_ALGORITHMS, lowered, std::ranges::lexicographical_compare)};
  if (found.empty()) {
    return std::nullopt;
  }

  return found.front();
}

// A single encryption or decryption on the cipher context of the instance.
class cipher_operation {
public:
  enum class direction : uint8_t { ENCRYPT, DECRYPT };

private:
  static constexpr size_t MAX_IV_LEN{16};
  static constexpr size_t MAX_KEY_LEN{64};

  std::string_view m_name;
  kphp::openssl::cipher m_cipher;
  // the context of the instance: the first `init_ex` with a cipher resets the state left by the previous operation
  const kphp::openssl::cipher_ctx& m_ctx;
  direction m_direction;
  size_t m_key_length;
  size_t m_iv_length;
  size_t m_block_size;
  bool m_is_aead;
  // CCM: the length of the data has to be set before the data is processed
  bool m_is_single_run_aead;

  cipher_operation(std::string_view name, kphp::openssl::cipher cipher, const kphp::openssl::cipher_ctx& ctx, direction direction, size_t key_length,
                   size_t iv_length, size_t block_size, bool is_aead, bool is_single_run_aead) noexcept
      : m_name(name),
        m_cipher(std::move(cipher)),
        m_ctx(ctx),
        m_direction(direction),
        m_key_length(key_length),
        m_iv_length(iv_length),
        m_block_size(block_size),
        m_is_aead(is_aead),
        m_is_single_run_aead(is_single_run_aead) {}

  // Returns the iv the cipher is initialized with: the passed one, or its copy in `buffer` (zero initialized) if it has to be padded with zeros.
  auto get_iv(const string& iv, std::span<std::byte> buffer) const noexcept -> std::optional<std::span<const std::byte>> {
    const std::span<const std::byte> iv_bytes{std::as_bytes(std::span{iv.c_str(), iv.size()})};
    if (iv.size() == m_iv_length) {
      return iv_bytes;
    }

    if (m_is_aead) {
      if (!m_ctx.set_iv_length(static_cast<int32_t>(iv.size()))) {
        kphp::log::warning("failed to set IV length for AEAD cipher");
        return std::nullopt;
      }

      return iv_bytes;
    }

    if (iv.empty()) {
      kphp::log::assertion(m_iv_length <= buffer.size());
      return std::span<const std::byte>{buffer.data(), m_iv_length};
    }

    if (iv.size() < m_iv_length) {
      kphp::log::assertion(m_iv_length <= buffer.size());
      kphp::log::warning("IV passed is only {} bytes long, cipher expects an IV of precisely {} bytes, padding with \\0", iv.size(), m_iv_length);
      std::memcpy(buffer.data(), iv.c_str(), iv.size());
      return std::span<const std::byte>{buffer.data(), m_iv_length};
    }

    kphp::log::warning("IV passed is {} bytes long which is longer than the {} expected by selected cipher, truncating", iv.size(), m_iv_length);

    return iv_bytes.first(m_iv_length);
  }

  // Returns the key the cipher is initialized with: the passed one, or its copy in `buffer` (zero initialized) if it has to be padded with zeros.
  auto get_key(const string& key, int64_t options, std::span<std::byte> buffer) const noexcept -> std::optional<std::span<const std::byte>> {
    const std::span<const std::byte> key_bytes{std::as_bytes(std::span{key.c_str(), key.size()})};
    if (key.size() < m_key_length) {
      if ((options & OPENSSL_DONT_ZERO_PAD_KEY) && !m_ctx.set_key_length(static_cast<int32_t>(key.size()))) {
        kphp::log::warning("failed to set key length for cipher '{}'", m_name);
        return std::nullopt;
      }

      kphp::log::assertion(m_key_length <= buffer.size());
      std::memcpy(buffer.data(), key.c_str(), key.size());

      return std::span<const std::byte>{buffer.data(), m_key_length};
    }

    if (key.size() > m_key_length && !m_ctx.set_key_length(static_cast<int32_t>(key.size()))) {
      kphp::log::warning("failed to set key length for cipher '{}'", m_name);
    }

    return key_bytes;
  }

  auto set_tag(const string& tag) const noexcept -> bool {
    if (m_is_single_run_aead && encrypts()) {
      if (!m_ctx.set_tag_length(static_cast<int32_t>(tag.size()))) {
        kphp::log::warning("failed to set tag length for AEAD cipher");
        return false;
      }
    } else if (!encrypts() && !tag.empty()) {
      if (!m_is_aead) {
        kphp::log::warning("tag cannot be used with non-AEAD cipher '{}'", m_name);
      } else if (!m_ctx.set_tag(std::as_bytes(std::span{tag.c_str(), tag.size()}))) {
        kphp::log::warning("failed to set tag for AEAD cipher");
        return false;
      }
    }

    return true;
  }

public:
  static auto create(const string& algorithm, direction direction) noexcept -> std::optional<cipher_operation> {
    const auto name{find_cipher_algorithm({algorithm.c_str(), algorithm.size()})};
    if (!name) {
      kphp::log::warning("unknown cipher algorithm '{}'", algorithm.c_str());
      return std::nullopt;
    }

    auto cipher{kphp::openssl::cipher::from_name(*name)};
    if (!cipher) {
      kphp::log::warning("failed to get cipher '{}'", algorithm.c_str());
      return std::nullopt;
    }

    const auto mode{cipher->mode()};
    const auto key_length{cipher->key_length()};
    const auto iv_length{cipher->iv_length()};
    const auto block_size{cipher->block_size()};
    if (!mode || !key_length || !iv_length || !block_size) [[unlikely]] {
      kphp::log::warning("failed to get parameters of cipher '{}'", algorithm.c_str());
      return std::nullopt;
    }

    const auto ctx{kphp::openssl::crypto_instance_state::get().cipher_ctx()};
    if (!ctx) {
      kphp::log::warning("failed to create cipher context");
      return std::nullopt;
    }

    const bool is_gcm{*mode == k2::CipherMode::CIPHER_MODE_GCM};
    const bool is_ccm{*mode == k2::CipherMode::CIPHER_MODE_CCM};

    return cipher_operation{*name,
                            std::move(*cipher),
                            **ctx,
                            direction,
                            static_cast<size_t>(*key_length),
                            static_cast<size_t>(*iv_length),
                            static_cast<size_t>(*block_size),
                            is_gcm || is_ccm,
                            is_ccm};
  }

  auto encrypts() const noexcept -> bool {
    return m_direction == direction::ENCRYPT;
  }

  auto init(const string& key, const string& iv, int64_t options, const string& tag) const noexcept -> bool {
    if (encrypts() && iv.empty() && m_iv_length > 0) {
      kphp::log::warning("using an empty Initialization Vector (iv) is potentially insecure and not recommended");
    }

    const int32_t encrypt{encrypts()};
    if (!m_ctx.init_ex(m_cipher, {}, {}, encrypt)) {
      kphp::log::warning("failed to initialize cipher");
      return false;
    }

    std::array<std::byte, MAX_IV_LEN> iv_buffer{};
    const auto init_iv{get_iv(iv, iv_buffer)};
    if (!init_iv || !set_tag(tag)) {
      return false;
    }

    std::array<std::byte, MAX_KEY_LEN> key_buffer{};
    const auto init_key{get_key(key, options, key_buffer)};
    if (!init_key) {
      return false;
    }

    if (!m_ctx.init_ex(*init_key, *init_iv, encrypt)) {
      kphp::log::warning("failed to initialize cipher");
      return false;
    }

    if (options & OPENSSL_ZERO_PADDING) {
      std::ignore = m_ctx.set_padding(0);
    }

    return true;
  }

  auto apply(const string& data, const string& aad) const noexcept -> std::optional<string> {
    if (data.size() > std::numeric_limits<int32_t>::max()) [[unlikely]] {
      kphp::log::warning("data is too long for cipher '{}'", m_name);
      return std::nullopt;
    }

    if (m_is_single_run_aead && !m_ctx.set_data_length(static_cast<int32_t>(data.size()))) {
      kphp::log::warning("failed to set data length");
      return std::nullopt;
    }

    if (m_is_aead && !m_ctx.update({}, std::as_bytes(std::span{aad.c_str(), aad.size()}))) {
      kphp::log::warning("failed to set AAD");
      return std::nullopt;
    }

    string result{static_cast<string::size_type>(data.size() + m_block_size), false};
    const auto result_bytes{std::as_writable_bytes(std::span{result.buffer(), result.size()})};
    const auto updated_len{m_ctx.update(result_bytes, std::as_bytes(std::span{data.c_str(), data.size()}))};
    if (!updated_len) {
      kphp::log::warning("failed to update cipher");
      return std::nullopt;
    }

    const auto final_len{m_ctx.final_ex(result_bytes.subspan(*updated_len))};
    if (!final_len) {
      kphp::log::warning("failed to finalize cipher");
      return std::nullopt;
    }

    result.shrink(static_cast<string::size_type>(*updated_len + *final_len));

    return result;
  }

  auto receive_tag(string& tag) const noexcept -> bool {
    if (m_is_aead && !tag.empty()) {
      if (m_ctx.get_tag(std::as_writable_bytes(std::span{tag.buffer(), tag.size()}))) {
        return true;
      }

      kphp::log::warning("failed to get AEAD tag");

      return false;
    }

    if (!tag.empty()) {
      kphp::log::warning("tag cannot be used with non-AEAD cipher '{}'", m_name);
    } else if (m_is_aead) {
      kphp::log::warning("tag must be provided for AEAD cipher");
      return false;
    }

    tag = string{};

    return true;
  }
};

auto eval_cipher(cipher_operation::direction direction, const string& data, const string& algorithm, const string& key, int64_t options, const string& iv,
                 string& tag, const string& aad) noexcept -> Optional<string> {
  const auto operation{cipher_operation::create(algorithm, direction)};
  if (!operation || !operation->init(key, iv, options, tag)) {
    return false;
  }

  auto result{operation->apply(data, aad)};
  if (!result || (operation->encrypts() && !operation->receive_tag(tag))) {
    return false;
  }

  return std::move(*result);
}

} // namespace

auto f$openssl_get_cipher_methods([[maybe_unused]] bool aliases) noexcept -> array<string> {
  array<string> res{array_size{CIPHER_ALGORITHMS.size(), true}};
  for (const auto method : CIPHER_ALGORITHMS) {
    res.push_back(string{method.data(), static_cast<string::size_type>(method.size())});
  }

  return res;
}

auto f$openssl_cipher_iv_length(const string& algorithm) noexcept -> Optional<int64_t> {
  const auto name{find_cipher_algorithm({algorithm.c_str(), algorithm.size()})};
  if (!name) {
    kphp::log::warning("unknown cipher algorithm '{}'", algorithm.c_str());
    return false;
  }

  const auto iv_len{kphp::openssl::cipher::from_name(*name).and_then([](const kphp::openssl::cipher& c) noexcept { return c.iv_length(); })};
  if (!iv_len) {
    kphp::log::warning("failed to get IV length of cipher '{}'", algorithm.c_str());
    return false;
  }

  return static_cast<int64_t>(*iv_len);
}

auto f$openssl_encrypt(const string& data, const string& algorithm, const string& key, int64_t options, const string& iv,
                       std::optional<std::reference_wrapper<string>> tag, const string& aad, int64_t tag_length) noexcept -> Optional<string> {
  string out_tag;
  if (tag) {
    if (tag_length < 0 || tag_length > string::max_size()) [[unlikely]] {
      kphp::log::warning("invalid tag length {}", tag_length);
      return false;
    }

    out_tag = string{static_cast<string::size_type>(tag_length), false};
  }

  auto result{eval_cipher(cipher_operation::direction::ENCRYPT, data, algorithm, key, options, iv, out_tag, aad)};
  if (!result.has_value()) {
    return false;
  }

  if (tag) {
    tag->get() = std::move(out_tag);
  }

  if (options & OPENSSL_RAW_DATA) {
    return result;
  }

  return f$base64_encode(result.val());
}

auto f$openssl_decrypt(const string& data, const string& algorithm, const string& key, int64_t options, const string& iv, string tag,
                       const string& aad) noexcept -> Optional<string> {
  if (options & OPENSSL_RAW_DATA) {
    return eval_cipher(cipher_operation::direction::DECRYPT, data, algorithm, key, options, iv, tag, aad);
  }

  auto decoded_data{f$base64_decode(data, true)};
  if (!decoded_data.has_value()) [[unlikely]] {
    kphp::log::warning("failed to decode base64 data");
    return false;
  }

  return eval_cipher(cipher_operation::direction::DECRYPT, decoded_data.val(), algorithm, key, options, iv, tag, aad);
}

auto f$openssl_pkey_get_public(const string& public_key) noexcept -> Optional<string> {
  if (pkey_from_handle(pkey_kind::PUBLIC, public_key)) {
    return public_key;
  }

  auto key{parse_public_key(public_key)};
  if (!key) [[unlikely]] {
    kphp::log::warning("invalid key");
    return false;
  }

  return make_pkey_handle(pkey_kind::PUBLIC, std::move(*key));
}

auto f$openssl_pkey_get_private(const string& private_key, const string& passphrase) noexcept -> Optional<string> {
  if (pkey_from_handle(pkey_kind::PRIVATE, private_key)) {
    return private_key;
  }

  auto key{parse_private_key(private_key, passphrase)};
  if (!key) [[unlikely]] {
    kphp::log::warning("invalid key or passphrase");
    return false;
  }

  return make_pkey_handle(pkey_kind::PRIVATE, std::move(*key));
}

auto f$openssl_public_encrypt(const string& data, string& encrypted_data, const string& public_key) noexcept -> bool {
  const auto key{load_public_key(public_key)};
  if (!key) [[unlikely]] {
    kphp::log::warning("invalid public key");
    encrypted_data = string{};
    return false;
  }

  if (const auto is_rsa{key->is_rsa()}; !is_rsa || !*is_rsa) [[unlikely]] {
    kphp::log::warning("key is not an RSA key");
    encrypted_data = string{};
    return false;
  }

  const auto key_size{key->size()};
  const auto rsa{key->rsa()};
  if (key_size && rsa) {
    string buffer{static_cast<string::size_type>(*key_size), false};
    const auto from{std::as_bytes(std::span{data.c_str(), data.size()})};
    const auto to{std::as_writable_bytes(std::span{buffer.buffer(), buffer.size()})};
    const auto written{rsa->public_encrypt(from, to, k2::RsaPadding::RSA_PADDING_PKCS1)};
    if (written && *written == *key_size) {
      encrypted_data = std::move(buffer);
      return true;
    }
  }

  encrypted_data = string{};
  kphp::log::warning("failed to encrypt data with RSA public key");

  return false;
}

auto f$openssl_public_encrypt(const string& data, mixed& encrypted_data, const string& public_key) noexcept -> bool {
  string result;
  if (f$openssl_public_encrypt(data, result, public_key)) {
    encrypted_data = std::move(result);
    return true;
  }

  encrypted_data = mixed{};

  return false;
}

auto f$openssl_private_decrypt(const string& data, string& decrypted_data, const string& private_key) noexcept -> bool {
  const auto key{load_private_key(private_key)};
  if (!key) [[unlikely]] {
    kphp::log::warning("invalid private key");
    return false;
  }

  if (const auto is_rsa{key->is_rsa()}; !is_rsa || !*is_rsa) [[unlikely]] {
    kphp::log::warning("key is not an RSA key");
    return false;
  }

  const auto size{key->size()};
  const auto rsa{key->rsa()};
  if (size && rsa) {
    string buffer{static_cast<string::size_type>(*size), false};
    const auto from{std::as_bytes(std::span{data.c_str(), data.size()})};
    const auto to{std::as_writable_bytes(std::span{buffer.buffer(), buffer.size()})};
    if (const auto written{rsa->private_decrypt(from, to, k2::RsaPadding::RSA_PADDING_PKCS1)}; written) {
      buffer.shrink(static_cast<string::size_type>(*written));
      decrypted_data = std::move(buffer);
      return true;
    }
  }

  decrypted_data = string{};
  kphp::log::warning("failed to decrypt data with RSA private key");

  return false;
}

auto f$openssl_private_decrypt(const string& data, mixed& decrypted_data, const string& private_key) noexcept -> bool {
  string result;
  if (f$openssl_private_decrypt(data, result, private_key)) {
    decrypted_data = std::move(result);
    return true;
  }

  decrypted_data = mixed{};

  return false;
}

namespace {

constexpr std::string_view SHA1{"sha1"};
constexpr std::string_view SHA224{"sha224"};
constexpr std::string_view SHA256{"sha256"};
constexpr std::string_view SHA384{"sha384"};
constexpr std::string_view SHA512{"sha512"};
constexpr std::string_view MD5{"md5"};
constexpr std::string_view RIPEMD160{"ripemd160"};
constexpr std::string_view MD2{"md2"};
constexpr std::string_view MD4{"md4"};

constexpr size_t SHA1_HASHED_SIZE{20};
constexpr size_t SHA224_HASHED_SIZE{28};
constexpr size_t SHA256_HASHED_SIZE{32};
constexpr size_t SHA384_HASHED_SIZE{48};
constexpr size_t SHA512_HASHED_SIZE{64};
constexpr size_t MD5_HASHED_SIZE{16};
constexpr size_t RIPEMD160_HASHED_SIZE{20};
constexpr size_t MD2_HASHED_SIZE{16};
constexpr size_t MD4_HASHED_SIZE{16};

using hash_function = std::expected<void, int32_t> (*)(std::span<const std::byte>, std::span<std::byte>);

struct hash_algorithm_traits {
  int64_t hashed_size;
  k2::HashAlgorithm algo;
  hash_function hash;
};

constexpr auto HASH_ALGORITHMS{[]() noexcept {
  auto arr{std::array{std::pair{SHA1, hash_algorithm_traits{SHA1_HASHED_SIZE, k2::HashAlgorithm::HASH_SHA1, k2::openssl_sha1}},
                      std::pair{SHA224, hash_algorithm_traits{SHA224_HASHED_SIZE, k2::HashAlgorithm::HASH_SHA224, k2::openssl_sha224}},
                      std::pair{SHA256, hash_algorithm_traits{SHA256_HASHED_SIZE, k2::HashAlgorithm::HASH_SHA256, k2::openssl_sha256}},
                      std::pair{SHA384, hash_algorithm_traits{SHA384_HASHED_SIZE, k2::HashAlgorithm::HASH_SHA384, k2::openssl_sha384}},
                      std::pair{SHA512, hash_algorithm_traits{SHA512_HASHED_SIZE, k2::HashAlgorithm::HASH_SHA512, k2::openssl_sha512}},
                      std::pair{MD5, hash_algorithm_traits{MD5_HASHED_SIZE, k2::HashAlgorithm::HASH_MD5, k2::openssl_md5}},
                      std::pair{RIPEMD160, hash_algorithm_traits{RIPEMD160_HASHED_SIZE, k2::HashAlgorithm::HASH_RMD160, k2::openssl_ripemd160}},
                      std::pair{MD2, hash_algorithm_traits{MD2_HASHED_SIZE, k2::HashAlgorithm::HASH_MD2, k2::openssl_md2}},
                      std::pair{MD4, hash_algorithm_traits{MD4_HASHED_SIZE, k2::HashAlgorithm::HASH_MD4, k2::openssl_md4}}}};

  std::ranges::sort(arr, std::less{}, &std::pair<std::string_view, hash_algorithm_traits>::first);
  return arr;
}()};

static_assert(HASH_ALGORITHMS.size() == k2::HashAlgorithm::COUNT_HASH);

auto parse_hash_algorithm(std::string_view algorithm) noexcept -> std::optional<hash_algorithm_traits> {
  auto lowered{std::ranges::transform_view{algorithm, vk::tolower_ascii}};

  auto rng{
      std::ranges::equal_range(HASH_ALGORITHMS, lowered, std::ranges::lexicographical_compare, &std::pair<std::string_view, hash_algorithm_traits>::first)};
  if (!rng.empty()) [[likely]] {
    return rng.front().second;
  }

  return std::nullopt;
}

auto hash(const hash_algorithm_traits& traits, const string& data, bool binary) noexcept -> string {
  string hashed_data{binary ? static_cast<string::size_type>(traits.hashed_size) : static_cast<string::size_type>(traits.hashed_size * 2), false};
  const auto data_bytes{std::as_bytes(std::span{data.c_str(), data.size()})};
  const auto hashed_bytes{std::as_writable_bytes(std::span{hashed_data.buffer(), static_cast<size_t>(traits.hashed_size)})};
  kphp::log::assertion(traits.hash(data_bytes, hashed_bytes).has_value());

  if (!binary) {
    kphp::strings::bin2hex(hashed_data.buffer(), traits.hashed_size, hashed_data.size());
  }

  return hashed_data;
}

auto hmac(k2::HashAlgorithm algorithm, std::span<const std::byte> key, std::span<const std::byte> data,
          std::span<std::byte> md) noexcept -> std::expected<uint32_t, int32_t> {
  // Reuse hmac_ctx from cache to avoid allocation
  // It's safe, because we call init, that resets hmac_ctx state.
  return kphp::openssl::crypto_instance_state::get().hmac_ctx().and_then([&](const kphp::openssl::hmac_ctx* ctx) noexcept {
    return ctx->init_ex(key, algorithm).and_then([&]() noexcept { return ctx->update(data); }).and_then([&]() noexcept { return ctx->final(md); });
  });
}

} // namespace

auto f$hash_algos() noexcept -> array<string> {
  array<string> res{array_size{HASH_ALGORITHMS.size(), true}};
  for (const auto& [algo, _] : HASH_ALGORITHMS) {
    res.push_back(string{algo.data(), static_cast<string::size_type>(algo.size())});
  }

  return res;
}

auto f$hash_hmac_algos() noexcept -> array<string> {
  return f$hash_algos();
}

auto f$hash(const string& algorithm, const string& data, bool binary) noexcept -> string {
  const auto algo{parse_hash_algorithm({algorithm.c_str(), algorithm.size()})};
  if (!algo) [[unlikely]] {
    kphp::log::error("unsupported hash algorithm '{}'", algorithm.c_str());
  }

  return hash(*algo, data, binary);
}

auto f$hash_hmac(const string& algorithm, const string& data, const string& key, bool binary) noexcept -> string {
  const auto algo{parse_hash_algorithm({algorithm.c_str(), algorithm.size()})};
  if (!algo) [[unlikely]] {
    kphp::log::error("unsupported hash algorithm '{}'", algorithm.c_str());
  }

  string hashed_data{binary ? static_cast<string::size_type>(algo->hashed_size) : static_cast<string::size_type>(algo->hashed_size * 2), false};
  const auto data_bytes{std::as_bytes(std::span{data.c_str(), data.size()})};
  const auto key_bytes{std::as_bytes(std::span{key.c_str(), key.size()})};
  const auto hashed_bytes{std::as_writable_bytes(std::span{hashed_data.buffer(), static_cast<size_t>(algo->hashed_size)})};
  const auto md_len{hmac(algo->algo, key_bytes, data_bytes, hashed_bytes)};
  kphp::log::assertion(md_len.has_value() && static_cast<int64_t>(*md_len) == algo->hashed_size);

  if (!binary) {
    kphp::strings::bin2hex(hashed_data.buffer(), algo->hashed_size, hashed_data.size());
  }

  return hashed_data;
}

auto f$sha1(const string& data, bool binary) noexcept -> string {
  return hash(hash_algorithm_traits{SHA1_HASHED_SIZE, k2::HashAlgorithm::HASH_SHA1, k2::openssl_sha1}, data, binary);
}

auto f$crc32(const string& s) noexcept -> int64_t {
  return crc32_partial_generic(static_cast<const void*>(s.c_str()), s.size(), -1) ^ -1;
}
