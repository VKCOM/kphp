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
#include <string>
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

Optional<string> f$openssl_random_pseudo_bytes(int64_t length) noexcept {
  if (length <= 0 || length > string::max_size()) [[unlikely]] {
    return false;
  }

  string buffer{static_cast<string::size_type>(length), false};
  k2::SystemTime now{};
  k2::system_time(std::addressof(now));
  // Error occurs only if arguments are invalid.
  std::ignore = k2::openssl_rand_add(std::as_bytes(std::span{std::addressof(now), 1}), 0.0);
  if (!k2::openssl_rand_bytes(std::as_writable_bytes(std::span{buffer.buffer(), buffer.size()}))) [[unlikely]] {
    return false;
  }

  return buffer;
}

namespace {

array<mixed> x509_get_name_entries(const kphp::openssl::x509_name& name, bool shortnames) noexcept {
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

array<mixed> x509_get_purposes(const kphp::openssl::x509& cert, bool shortnames) noexcept {
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

int64_t convert_asn1_time(const std::optional<kphp::openssl::asn1_time>& epoch, const kphp::openssl::asn1_time& time) noexcept {
  const auto diff{epoch ? epoch->diff(time) : time.diff_from_now()};
  return diff ? int64_t{diff->days} * 24 * 60 * 60 + diff->seconds : 0;
}

} // namespace

Optional<array<mixed>> f$openssl_x509_parse(const string& data, bool shortnames) noexcept {
  const auto cert{kphp::openssl::x509::from_pem(std::as_bytes(std::span{data.c_str(), data.size()}))};
  if (!cert) {
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
// A handle is the raw bytes of a platform descriptor, which lives until the end of the instance.
constexpr size_t PKEY_HANDLE_SIZE{sizeof(k2::descriptor)};

std::optional<kphp::openssl::pkey> borrow_pkey_handle(const string& key) noexcept {
  if (key.size() != PKEY_HANDLE_SIZE) {
    return std::nullopt;
  }

  k2::descriptor descriptor{};
  std::memcpy(std::addressof(descriptor), key.c_str(), sizeof(descriptor));

  auto handle{kphp::openssl::pkey::borrow(descriptor)};
  if (!handle.id()) {
    return std::nullopt;
  }

  return handle;
}

// Gives the key to the instance: the platform frees its descriptor when the instance ends.
string make_pkey_handle(kphp::openssl::pkey key) noexcept {
  const auto descriptor{key.release()};

  string handle{static_cast<string::size_type>(PKEY_HANDLE_SIZE), false};
  std::memcpy(handle.buffer(), std::addressof(descriptor), sizeof(descriptor));

  return handle;
}

// Accepts either a certificate, in which case the key is taken from it, or a public key.
std::expected<kphp::openssl::pkey, int32_t> parse_public_key(const string& key) noexcept {
  const auto pem{std::as_bytes(std::span{key.c_str(), key.size()})};
  if (const auto cert{kphp::openssl::x509::from_pem(pem)}; cert) {
    return cert->pubkey();
  }

  return kphp::openssl::pkey::public_from_pem(pem);
}

std::expected<kphp::openssl::pkey, int32_t> load_public_key(const string& key) noexcept {
  if (auto handle{borrow_pkey_handle(key)}; handle) {
    return std::move(*handle);
  }

  return parse_public_key(key);
}

std::expected<kphp::openssl::pkey, int32_t> parse_private_key(const string& key, const string& passphrase) noexcept {
  return kphp::openssl::pkey::private_from_pem(std::as_bytes(std::span{key.c_str(), key.size()}),
                                               std::as_bytes(std::span{passphrase.c_str(), passphrase.size()}));
}

std::expected<kphp::openssl::pkey, int32_t> load_private_key(const string& key) noexcept {
  if (auto handle{borrow_pkey_handle(key)}; handle) {
    return std::move(*handle);
  }

  return parse_private_key(key, string{});
}

std::optional<k2::DigestAlgorithm> get_digest_algorithm(int64_t algorithm) noexcept {
  if (algorithm < 0 || algorithm >= k2::DigestAlgorithm::COUNT_DIGEST) {
    return std::nullopt;
  }

  return static_cast<k2::DigestAlgorithm>(algorithm);
}

} // namespace

bool f$openssl_sign(const string& data, string& signature, const string& private_key, int64_t algorithm) noexcept {
  const auto digest{get_digest_algorithm(algorithm)};
  if (!digest) [[unlikely]] {
    kphp::log::warning("unknown signature algorithm");
    return false;
  }

  const auto key{load_private_key(private_key)};
  if (!key) [[unlikely]] {
    kphp::log::warning("parameter key cannot be converted into a private key");
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
  kphp::log::warning("can't create signature for data");

  return false;
}

int64_t f$openssl_verify(const string& data, const string& signature, const string& public_key, int64_t algorithm) noexcept {
  const auto digest{get_digest_algorithm(algorithm)};
  if (!digest) [[unlikely]] {
    kphp::log::warning("unknown signature algorithm");
    return 0;
  }

  const auto key{load_public_key(public_key)};
  if (!key) [[unlikely]] {
    kphp::log::warning("parameter key cannot be converted into a public key");
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

  return verified ? static_cast<int64_t>(*verified) : openssl_err;
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

std::optional<std::string_view> find_cipher_algorithm(std::string_view name) noexcept {
  const auto lowered{std::ranges::transform_view{name, vk::tolower_ascii}};
  const auto found{std::ranges::equal_range(CIPHER_ALGORITHMS, lowered, std::ranges::lexicographical_compare)};
  if (found.empty()) {
    return std::nullopt;
  }

  return found.front();
}

enum class cipher_direction : uint8_t { ENCRYPT, DECRYPT };

struct cipher_session {
  kphp::openssl::cipher cipher;
  // the context of the instance: the first `init_ex` with a cipher resets the state left by the previous session
  const kphp::openssl::cipher_ctx& ctx;
  cipher_direction direction;
  size_t key_length;
  size_t iv_length;
  size_t block_size;
  bool is_aead;
  // CCM: the length of the data has to be set before the data is processed
  bool is_single_run_aead;

  bool encrypts() const noexcept {
    return direction == cipher_direction::ENCRYPT;
  }
};

std::optional<cipher_session> open_cipher_session(const string& algorithm, cipher_direction direction) noexcept {
  const auto name{find_cipher_algorithm({algorithm.c_str(), algorithm.size()})};
  if (!name) {
    kphp::log::warning("unknown cipher algorithm '{}'", algorithm.c_str());
    return std::nullopt;
  }

  auto cipher{kphp::openssl::cipher::create_by_name(*name)};
  if (!cipher) {
    kphp::log::warning("failed to create cipher '{}'", algorithm.c_str());
    return std::nullopt;
  }

  const auto mode{cipher->mode()};
  const auto key_length{cipher->key_length()};
  const auto iv_length{cipher->iv_length()};
  const auto block_size{cipher->block_size()};
  if (!mode || !key_length || !iv_length || !block_size) [[unlikely]] {
    kphp::log::warning("failed to get the parameters of cipher '{}'", algorithm.c_str());
    return std::nullopt;
  }

  const auto ctx{kphp::openssl::crypto_instance_state::get().cipher_ctx()};
  if (!ctx) {
    kphp::log::warning("failed to create cipher context");
    return std::nullopt;
  }

  const bool is_gcm{*mode == k2::CipherMode::CIPHER_MODE_GCM};
  const bool is_ccm{*mode == k2::CipherMode::CIPHER_MODE_CCM};

  return cipher_session{.cipher = std::move(*cipher),
                        .ctx = **ctx,
                        .direction = direction,
                        .key_length = static_cast<size_t>(*key_length),
                        .iv_length = static_cast<size_t>(*iv_length),
                        .block_size = static_cast<size_t>(*block_size),
                        .is_aead = is_gcm || is_ccm,
                        .is_single_run_aead = is_ccm};
}

// Returns the iv the cipher is initialized with: the passed one, or its copy in `buffer` if it has to be padded with zeros.
std::optional<std::span<const std::byte>> get_cipher_iv(const cipher_session& session, const string& iv, std::span<std::byte> buffer) noexcept {
  const std::span<const std::byte> iv_bytes{std::as_bytes(std::span{iv.c_str(), iv.size()})};
  if (iv.size() == session.iv_length) {
    return iv_bytes;
  }

  if (session.is_aead) {
    if (!session.ctx.set_iv_length(static_cast<int32_t>(iv.size()))) {
      kphp::log::warning("setting of IV length for AEAD mode failed");
      return std::nullopt;
    }

    return iv_bytes;
  }

  if (iv.empty()) {
    kphp::log::assertion(session.iv_length <= buffer.size());
    return std::span<const std::byte>{buffer.data(), session.iv_length};
  }

  if (iv.size() < session.iv_length) {
    kphp::log::assertion(session.iv_length <= buffer.size());
    kphp::log::warning("IV passed is only {} bytes long, cipher expects an IV of precisely {} bytes, padding with \\0", iv.size(), session.iv_length);
    std::memcpy(buffer.data(), iv.c_str(), iv.size());
    return std::span<const std::byte>{buffer.data(), session.iv_length};
  }

  kphp::log::warning("IV passed is {} bytes long which is longer than the {} expected by selected cipher, truncating", iv.size(), session.iv_length);

  return iv_bytes.first(session.iv_length);
}

// Returns the key the cipher is initialized with: the passed one, or its copy in `buffer` if it has to be padded with zeros.
std::optional<std::span<const std::byte>> get_cipher_key(const cipher_session& session, const string& key, int64_t options,
                                                         std::span<std::byte> buffer) noexcept {
  const std::span<const std::byte> key_bytes{std::as_bytes(std::span{key.c_str(), key.size()})};
  if (key.size() < session.key_length) {
    // a short key is always padded with zeros, `OPENSSL_DONT_ZERO_PAD_KEY` only makes the cipher accept the key of the shorter length as well
    if ((options & OPENSSL_DONT_ZERO_PAD_KEY) && !session.ctx.set_key_length(static_cast<int32_t>(key.size()))) {
      kphp::log::warning("key length cannot be set for the cipher method");
      return std::nullopt;
    }

    kphp::log::assertion(session.key_length <= buffer.size());
    std::memcpy(buffer.data(), key.c_str(), key.size());

    return std::span<const std::byte>{buffer.data(), session.key_length};
  }

  if (key.size() > session.key_length && !session.ctx.set_key_length(static_cast<int32_t>(key.size()))) {
    kphp::log::warning("key length cannot be set for the cipher method");
  }

  return key_bytes;
}

bool set_tag(const cipher_session& session, const string& tag) noexcept {
  if (session.is_single_run_aead && session.encrypts()) {
    if (!session.ctx.set_tag_length(static_cast<int32_t>(tag.size()))) {
      kphp::log::warning("setting tag length for AEAD cipher failed");
      return false;
    }
  } else if (!session.encrypts() && !tag.empty()) {
    if (!session.is_aead) {
      kphp::log::warning("the tag cannot be used because the cipher method does not support AEAD");
    } else if (!session.ctx.set_tag(std::as_bytes(std::span{tag.c_str(), tag.size()}))) {
      kphp::log::warning("setting tag for AEAD cipher decryption failed");
      return false;
    }
  }

  return true;
}

bool init_cipher(const cipher_session& session, const string& key, const string& iv, int64_t options, const string& tag) noexcept {
  static constexpr size_t MAX_IV_LEN{16};
  static constexpr size_t MAX_KEY_LEN{64};

  const auto& ctx{session.ctx};
  const int32_t encrypt{session.encrypts()};

  if (session.encrypts() && iv.empty() && session.iv_length > 0) {
    kphp::log::warning("using an empty Initialization Vector (iv) is potentially insecure and not recommended");
  }

  if (!ctx.init_ex(session.cipher, {}, {}, encrypt)) {
    kphp::log::warning("cipher init error");
    return false;
  }

  std::array<std::byte, MAX_IV_LEN> iv_buffer{};
  const auto fitted_iv{get_cipher_iv(session, iv, iv_buffer)};
  if (!fitted_iv || !set_tag(session, tag)) {
    return false;
  }

  std::array<std::byte, MAX_KEY_LEN> key_buffer{};
  const auto fitted_key{get_cipher_key(session, key, options, key_buffer)};
  if (!fitted_key) {
    return false;
  }

  if (!ctx.init_ex(*fitted_key, *fitted_iv, encrypt)) {
    kphp::log::warning("cipher init error");
    return false;
  }

  if (options & OPENSSL_ZERO_PADDING) {
    std::ignore = ctx.set_padding(0);
  }

  return true;
}

std::optional<string> apply_cipher(const cipher_session& session, const string& data, const string& aad) noexcept {
  const auto& ctx{session.ctx};

  if (data.size() > std::numeric_limits<int32_t>::max()) [[unlikely]] {
    kphp::log::warning("data is too long for the cipher");
    return std::nullopt;
  }

  if (session.is_single_run_aead && !ctx.set_data_length(static_cast<int32_t>(data.size()))) {
    kphp::log::warning("setting of data length failed");
    return std::nullopt;
  }

  if (session.is_aead && !ctx.update({}, std::as_bytes(std::span{aad.c_str(), aad.size()}))) {
    kphp::log::warning("setting of additional application data failed");
    return std::nullopt;
  }

  string result{static_cast<string::size_type>(data.size() + session.block_size), false};
  const auto result_bytes{std::as_writable_bytes(std::span{result.buffer(), result.size()})};
  const auto updated_len{ctx.update(result_bytes, std::as_bytes(std::span{data.c_str(), data.size()}))};
  if (!updated_len) {
    kphp::log::warning("cipher update error");
    return std::nullopt;
  }

  const auto final_len{ctx.final_ex(result_bytes.subspan(*updated_len))};
  if (!final_len) {
    kphp::log::warning("cipher finalize error");
    return std::nullopt;
  }

  result.shrink(static_cast<string::size_type>(*updated_len + *final_len));

  return result;
}

bool receive_tag(const cipher_session& session, string& tag) noexcept {
  if (session.is_aead && !tag.empty()) {
    if (session.ctx.get_tag(std::as_writable_bytes(std::span{tag.buffer(), tag.size()}))) {
      return true;
    }

    kphp::log::warning("retrieving verification tag failed");

    return false;
  }

  if (!tag.empty()) {
    kphp::log::warning("the authenticated tag cannot be provided for cipher that doesn not support AEAD");
  } else if (session.is_aead) {
    kphp::log::warning("a tag should be provided when using AEAD mode");
    return false;
  }

  tag = string{};

  return true;
}

Optional<string> run_cipher(cipher_direction direction, const string& data, const string& algorithm, const string& key, int64_t options, const string& iv,
                            string& tag, const string& aad) noexcept {
  const auto session{open_cipher_session(algorithm, direction)};
  if (!session || !init_cipher(*session, key, iv, options, tag)) {
    return false;
  }

  auto result{apply_cipher(*session, data, aad)};
  if (!result || (session->encrypts() && !receive_tag(*session, tag))) {
    return false;
  }

  return std::move(*result);
}

} // namespace

array<string> f$openssl_get_cipher_methods([[maybe_unused]] bool aliases) noexcept {
  array<string> res{array_size{CIPHER_ALGORITHMS.size(), true}};
  for (const auto method : CIPHER_ALGORITHMS) {
    res.push_back(string{method.data(), static_cast<string::size_type>(method.size())});
  }

  return res;
}

Optional<int64_t> f$openssl_cipher_iv_length(const string& algorithm) noexcept {
  const auto name{find_cipher_algorithm({algorithm.c_str(), algorithm.size()})};
  if (!name) {
    kphp::log::warning("unknown cipher algorithm {}", algorithm.c_str());
    return false;
  }

  const auto iv_len{kphp::openssl::cipher::create_by_name(*name).and_then([](const kphp::openssl::cipher& c) noexcept { return c.iv_length(); })};
  if (!iv_len) {
    kphp::log::warning("failed to get iv length of cipher algorithm {}", algorithm.c_str());
    return false;
  }

  return static_cast<int64_t>(*iv_len);
}

Optional<string> f$openssl_encrypt(const string& data, const string& algorithm, const string& key, int64_t options, const string& iv,
                                   std::optional<std::reference_wrapper<string>> tag, const string& aad, int64_t tag_length) noexcept {
  string out_tag;
  if (tag) {
    if (tag_length < 0 || tag_length > string::max_size()) [[unlikely]] {
      kphp::log::warning("invalid tag length {}", tag_length);
      return false;
    }

    out_tag = string{static_cast<string::size_type>(tag_length), false};
  }

  auto result{run_cipher(cipher_direction::ENCRYPT, data, algorithm, key, options, iv, out_tag, aad)};
  if (!result.has_value()) {
    kphp::log::warning("failed to encrypt data");
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

Optional<string> f$openssl_decrypt(const string& data, const string& algorithm, const string& key, int64_t options, const string& iv, string tag,
                                   const string& aad) noexcept {
  if (options & OPENSSL_RAW_DATA) {
    return run_cipher(cipher_direction::DECRYPT, data, algorithm, key, options, iv, tag, aad);
  }

  auto decoded_data{f$base64_decode(data, true)};
  if (!decoded_data.has_value()) [[unlikely]] {
    kphp::log::warning("failed to base64 decode the input");
    return false;
  }

  return run_cipher(cipher_direction::DECRYPT, decoded_data.val(), algorithm, key, options, iv, tag, aad);
}

Optional<string> f$openssl_pkey_get_public(const string& public_key) noexcept {
  if (borrow_pkey_handle(public_key)) {
    return public_key;
  }

  auto key{parse_public_key(public_key)};
  if (!key) [[unlikely]] {
    kphp::log::warning("parameter key is not a valid key");
    return false;
  }

  return make_pkey_handle(std::move(*key));
}

Optional<string> f$openssl_pkey_get_private(const string& private_key, const string& passphrase) noexcept {
  if (borrow_pkey_handle(private_key)) {
    return private_key;
  }

  auto key{parse_private_key(private_key, passphrase)};
  if (!key) [[unlikely]] {
    kphp::log::warning("parameter key is not a valid key or passphrase is not a valid password");
    return false;
  }

  return make_pkey_handle(std::move(*key));
}

bool f$openssl_public_encrypt(const string& data, string& encrypted_data, const string& public_key) noexcept {
  const auto key{load_public_key(public_key)};
  if (!key) [[unlikely]] {
    kphp::log::warning("parameter key is not a valid public key");
    encrypted_data = string{};
    return false;
  }

  if (const auto is_rsa{key->is_rsa()}; !is_rsa || !*is_rsa) [[unlikely]] {
    kphp::log::warning("key type is neither RSA nor RSA2");
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
  kphp::log::warning("RSA public encrypt failed");

  return false;
}

bool f$openssl_public_encrypt(const string& data, mixed& encrypted_data, const string& public_key) noexcept {
  string result;
  if (f$openssl_public_encrypt(data, result, public_key)) {
    encrypted_data = std::move(result);
    return true;
  }

  encrypted_data = mixed{};

  return false;
}

bool f$openssl_private_decrypt(const string& data, string& decrypted_data, const string& private_key) noexcept {
  const auto key{load_private_key(private_key)};
  if (!key) [[unlikely]] {
    kphp::log::warning("parameter key is not a valid private key");
    return false;
  }

  if (const auto is_rsa{key->is_rsa()}; !is_rsa || !*is_rsa) [[unlikely]] {
    kphp::log::warning("key type is not an RSA nor RSA2");
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

  return false;
}

bool f$openssl_private_decrypt(const string& data, mixed& decrypted_data, const string& private_key) noexcept {
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

std::optional<hash_algorithm_traits> parse_hash_algorithm(std::string_view algorithm) noexcept {
  auto lowered{std::ranges::transform_view{algorithm, vk::tolower_ascii}};

  auto rng{
      std::ranges::equal_range(HASH_ALGORITHMS, lowered, std::ranges::lexicographical_compare, &std::pair<std::string_view, hash_algorithm_traits>::first)};
  if (!rng.empty()) [[likely]] {
    return rng.front().second;
  }

  return std::nullopt;
}

string hash(const hash_algorithm_traits& traits, const string& data, bool binary) noexcept {
  string hashed_data{binary ? static_cast<string::size_type>(traits.hashed_size) : static_cast<string::size_type>(traits.hashed_size * 2), false};
  const auto data_bytes{std::as_bytes(std::span{data.c_str(), data.size()})};
  const auto hashed_bytes{std::as_writable_bytes(std::span{hashed_data.buffer(), static_cast<size_t>(traits.hashed_size)})};
  kphp::log::assertion(traits.hash(data_bytes, hashed_bytes).has_value());

  if (!binary) {
    kphp::strings::bin2hex(hashed_data.buffer(), traits.hashed_size, hashed_data.size());
  }

  return hashed_data;
}

std::expected<uint32_t, int32_t> hmac(k2::HashAlgorithm algorithm, std::span<const std::byte> key, std::span<const std::byte> data,
                                      std::span<std::byte> md) noexcept {
  // Reuse hmac_ctx from cache to avoid allocation
  // It's safe, because we call init, that resets hmac_ctx state.
  return kphp::openssl::crypto_instance_state::get().hmac_ctx().and_then([&](const kphp::openssl::hmac_ctx* ctx) noexcept {
    return ctx->init_ex(key, algorithm).and_then([&]() noexcept { return ctx->update(data); }).and_then([&]() noexcept { return ctx->final(md); });
  });
}

} // namespace

array<string> f$hash_algos() noexcept {
  array<string> res{array_size{HASH_ALGORITHMS.size(), true}};
  for (const auto& [algo, _] : HASH_ALGORITHMS) {
    res.push_back(string{algo.data(), static_cast<string::size_type>(algo.size())});
  }

  return res;
}

array<string> f$hash_hmac_algos() noexcept {
  return f$hash_algos();
}

string f$hash(const string& algorithm, const string& data, bool binary) noexcept {
  const auto algo{parse_hash_algorithm({algorithm.c_str(), algorithm.size()})};
  if (!algo) [[unlikely]] {
    kphp::log::error("algorithm {} not supported in function hash", algorithm.c_str());
  }

  return hash(*algo, data, binary);
}

string f$hash_hmac(const string& algorithm, const string& data, const string& key, bool binary) noexcept {
  const auto algo{parse_hash_algorithm({algorithm.c_str(), algorithm.size()})};
  if (!algo) [[unlikely]] {
    kphp::log::error("algorithm {} not supported in function hash_hmac", algorithm.c_str());
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

string f$sha1(const string& data, bool binary) noexcept {
  return hash(hash_algorithm_traits{SHA1_HASHED_SIZE, k2::HashAlgorithm::HASH_SHA1, k2::openssl_sha1}, data, binary);
}

int64_t f$crc32(const string& s) noexcept {
  return crc32_partial_generic(static_cast<const void*>(s.c_str()), s.size(), -1) ^ -1;
}
