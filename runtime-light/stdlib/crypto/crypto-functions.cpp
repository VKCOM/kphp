//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/crypto/crypto-functions.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
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
  [[maybe_unused]] auto _ = k2::openssl_rand_add(std::as_bytes(std::span{std::addressof(now), 1}), 0.0);
  if (!k2::openssl_rand_bytes(std::as_writable_bytes(std::span{buffer.buffer(), buffer.size()}))) [[unlikely]] {
    return false;
  }

  return buffer;
}

namespace {

std::optional<array<mixed>> x509_get_name_entries(const kphp::openssl::x509_name& name, bool shortnames) noexcept {
  const auto count{name.entry_count()};
  if (!count) [[unlikely]] {
    return std::nullopt;
  }

  array<mixed> res{array_size{static_cast<int64_t>(*count), false}};
  for (int32_t i = 0; i < *count; ++i) {
    const auto entry{name.entry(i)};
    if (!entry) [[unlikely]] {
      return std::nullopt;
    }

    const auto key_object{entry->object()};
    const auto value{entry->data()};
    if (!key_object || !value) [[unlikely]] {
      return std::nullopt;
    }

    const auto nid{key_object->obj2nid()};
    if (!nid) [[unlikely]] {
      return std::nullopt;
    }

    const auto key_str{shortnames ? k2::openssl_obj_nid2sn(*nid) : k2::openssl_obj_nid2ln(*nid)};
    const auto value_data{value->data()};
    const auto value_len{value->length()};
    if (!key_str || !value_data || !value_len) [[unlikely]] {
      return std::nullopt;
    }

    res.set_value(string{*key_str}, *value_len == 0 ? string{} : string{static_cast<const char*>(*value_data), static_cast<string::size_type>(*value_len)});
  }

  return res;
}

std::optional<array<mixed>> x509_get_purposes(const kphp::openssl::x509& cert, bool shortnames) noexcept {
  const auto count{kphp::openssl::x509_purpose::count()};
  if (!count) [[unlikely]] {
    return std::nullopt;
  }

  array<mixed> res{array_size{static_cast<int64_t>(*count), false}};
  for (int32_t i = 0; i < *count; ++i) {
    const auto purpose{kphp::openssl::x509_purpose::get(i)};
    if (!purpose) [[unlikely]] {
      return std::nullopt;
    }

    const auto id{purpose->id()};
    const auto name{shortnames ? purpose->sname() : purpose->name()};
    if (!id || !name) [[unlikely]] {
      return std::nullopt;
    }

    const auto purpset_0{cert.check_purpose(*id, 0)};
    const auto purpset_1{cert.check_purpose(*id, 1)};
    if (!purpset_0 || !purpset_1) [[unlikely]] {
      return std::nullopt;
    }

    res.set_value(static_cast<int64_t>(*id), array<mixed>::create(*purpset_0 != 0, *purpset_1 != 0, string{*name}));
  }

  return res;
}

string hash_to_hex(uint64_t hash) noexcept {
  constexpr std::string_view hex_digits{"0123456789abcdef"};
  constexpr string::size_type hash_str_size{8};

  string res{hash_str_size, false};
  for (auto i{hash_str_size}; i > 0; --i, hash >>= 4) {
    res[i - 1] = hex_digits[hash & 0xf];
  }

  return res;
}

std::optional<int64_t> convert_asn1_time(const kphp::openssl::asn1_time& epoch, const kphp::openssl::asn1_time& time) noexcept {
  const auto diff{epoch.diff(time)};
  if (!diff) {
    return diff.error() == EIO ? std::optional<int64_t>{0} : std::nullopt;
  }

  return int64_t{diff->days} * 24 * 60 * 60 + diff->seconds;
}

} // namespace

Optional<array<mixed>> f$openssl_x509_parse(const string& data, bool shortnames) noexcept {
  const auto cert{kphp::openssl::x509::read_bio(std::as_bytes(std::span{data.c_str(), data.size()}))};
  if (!cert) {
    return false;
  }

  const auto subject_name{cert->subject_name()};
  const auto issuer_name{cert->issuer_name()};
  const auto hash{cert->subject_name_hash()};
  const auto version{cert->version()};
  if (!subject_name || !issuer_name || !hash || !version) [[unlikely]] {
    kphp::log::warning("failed to get X509 certificate fields");
    return false;
  }

  const auto name{subject_name->oneline()};
  auto subject{x509_get_name_entries(*subject_name, shortnames)};
  auto issuer{x509_get_name_entries(*issuer_name, shortnames)};
  const auto epoch{kphp::openssl::asn1_time::create()};
  if (!epoch || !epoch->set(0)) [[unlikely]] {
    kphp::log::warning("failed to get X509 certificate fields");
    return false;
  }

  const auto not_before{cert->not_before()};
  const auto not_after{cert->not_after()};
  if (!not_before || !not_after) [[unlikely]] {
    kphp::log::warning("failed to get X509 certificate fields");
    return false;
  }

  const auto valid_from{convert_asn1_time(*epoch, *not_before)};
  const auto valid_to{convert_asn1_time(*epoch, *not_after)};
  auto purposes{x509_get_purposes(*cert, shortnames)};
  if (!name || !subject || !issuer || !valid_from || !valid_to || !purposes) [[unlikely]] {
    kphp::log::warning("failed to get X509 certificate fields");
    return false;
  }

  const auto& crypto_state{kphp::openssl::crypto_image_state::get()};

  return std::initializer_list<std::pair<string, mixed>>{{crypto_state.NAME_STR, string{name->c_str()}}, {crypto_state.SUBJECT_STR, std::move(*subject)},
                                                         {crypto_state.HASH_STR, hash_to_hex(*hash)},    {crypto_state.ISSUER_STR, std::move(*issuer)},
                                                         {crypto_state.VERSION_STR, *version},           {crypto_state.VALID_FROM_TIME_T_STR, *valid_from},
                                                         {crypto_state.VALID_TO_TIME_T_STR, *valid_to},  {crypto_state.PURPOSES_STR, std::move(*purposes)}};
}

namespace {

// A key argument is either a handle returned by openssl_pkey_get_{public,private} or a PEM string.
// A handle is a tag followed by the raw bytes of a platform descriptor, which lives until the end of the instance.
// The tag tells a public key from a private one. No valid PEM is that short, so the size alone tells a handle from a PEM.
constexpr char PUBLIC_KEY_HANDLE_TAG{';'};
constexpr char PRIVATE_KEY_HANDLE_TAG{':'};
constexpr size_t PKEY_HANDLE_SIZE{1 + sizeof(k2::descriptor)};

std::optional<kphp::openssl::pkey> borrow_pkey_handle(const string& key, char tag) noexcept {
  if (key.size() != PKEY_HANDLE_SIZE || key[0] != tag) {
    return std::nullopt;
  }

  k2::descriptor descriptor{};
  std::memcpy(std::addressof(descriptor), key.c_str() + 1, sizeof(descriptor));

  // a forged handle is rejected here: the platform knows what its descriptors are
  auto handle{kphp::openssl::pkey::borrow(descriptor)};
  if (!handle.id()) {
    return std::nullopt;
  }

  return handle;
}

// Gives the key to the instance: the platform frees its descriptor when the instance ends.
string make_pkey_handle(kphp::openssl::pkey key, char tag) noexcept {
  const auto descriptor{key.release()};

  string handle{static_cast<string::size_type>(PKEY_HANDLE_SIZE), false};
  handle[0] = tag;
  std::memcpy(handle.buffer() + 1, std::addressof(descriptor), sizeof(descriptor));

  return handle;
}

// Accepts either a certificate, in which case the key is taken from it, or a public key.
std::expected<kphp::openssl::pkey, int32_t> parse_public_key(const string& key) noexcept {
  const auto pem{std::as_bytes(std::span{key.c_str(), key.size()})};
  if (const auto cert{kphp::openssl::x509::read_bio(pem)}; cert) {
    return cert->pubkey();
  }

  return kphp::openssl::pkey::read_bio_pubkey(pem);
}

std::expected<kphp::openssl::pkey, int32_t> load_public_key(const string& key) noexcept {
  if (auto handle{borrow_pkey_handle(key, PUBLIC_KEY_HANDLE_TAG)}; handle) {
    return std::move(*handle);
  }

  return parse_public_key(key);
}

// an empty passphrase is no passphrase at all, as in the legacy runtime
std::expected<kphp::openssl::pkey, int32_t> read_private_key(const string& key, const string& passphrase) noexcept {
  return kphp::openssl::pkey::read_bio_private_key(std::as_bytes(std::span{key.c_str(), key.size()}),
                                                   std::as_bytes(std::span{passphrase.c_str(), passphrase.size()}));
}

std::expected<kphp::openssl::pkey, int32_t> load_private_key(const string& key) noexcept {
  if (auto handle{borrow_pkey_handle(key, PRIVATE_KEY_HANDLE_TAG)}; handle) {
    return std::move(*handle);
  }

  return read_private_key(key, string{});
}

std::optional<k2::DigestAlgorithm> parse_digest_algorithm(int64_t algorithm) noexcept {
  if (algorithm < 0 || algorithm >= k2::DigestAlgorithm::COUNT_DIGEST) {
    return std::nullopt;
  }

  return static_cast<k2::DigestAlgorithm>(algorithm);
}

} // namespace

bool f$openssl_sign(const string& data, string& signature, const string& private_key, int64_t algorithm) noexcept {
  const auto digest{parse_digest_algorithm(algorithm)};
  if (!digest) [[unlikely]] {
    kphp::log::warning("Unknown signature algorithm");
    return false;
  }

  // the failure of everything but the algorithm is reported after the key is loaded, as in the legacy runtime
  const auto ctx{kphp::openssl::crypto_instance_state::get().md_ctx()};
  const auto init{ctx.and_then([digest](const kphp::openssl::md_ctx* c) noexcept { return c->sign_init(*digest); })};
  if (!init && init.error() == EINVAL) [[unlikely]] {
    kphp::log::warning("Unknown signature algorithm");
    return false;
  }

  const auto key{load_private_key(private_key)};
  if (!key) [[unlikely]] {
    kphp::log::warning("Parameter key cannot be converted into a private key");
    return false;
  }

  if (const auto size{key->size()}; size) {
    string buffer{static_cast<string::size_type>(*size), false};
    const auto sig_len{
        init.and_then([&]() noexcept { return (*ctx)->sign_update(std::as_bytes(std::span{data.c_str(), data.size()})); }).and_then([&]() noexcept {
          return (*ctx)->sign_final(std::as_writable_bytes(std::span{buffer.buffer(), buffer.size()}), *key);
        })};
    if (sig_len) {
      buffer.shrink(static_cast<string::size_type>(*sig_len));
      signature = std::move(buffer);
      return true;
    }
  }

  signature = string{};
  kphp::log::warning("Can't create signature for data");
  return false;
}

int64_t f$openssl_verify(const string& data, const string& signature, const string& public_key, int64_t algorithm) noexcept {
  const auto digest{parse_digest_algorithm(algorithm)};
  if (!digest) [[unlikely]] {
    kphp::log::warning("Unknown signature algorithm");
    return 0;
  }

  const auto ctx{kphp::openssl::crypto_instance_state::get().md_ctx()};
  const auto init{ctx.and_then([digest](const kphp::openssl::md_ctx* c) noexcept { return c->verify_init(*digest); })};
  if (!init && init.error() == EINVAL) [[unlikely]] {
    kphp::log::warning("Unknown signature algorithm");
    return 0;
  }

  const auto key{load_public_key(public_key)};
  if (!key) [[unlikely]] {
    kphp::log::warning("Parameter key cannot be converted into a public key");
    return 0;
  }

  // the result of `EVP_VerifyFinal` is returned as is, `0` is the result if the verification did not get to it
  int32_t openssl_err{0};
  const auto verified{
      init.and_then([&]() noexcept { return (*ctx)->verify_update(std::as_bytes(std::span{data.c_str(), data.size()})); }).and_then([&]() noexcept {
        return (*ctx)->verify_final(std::as_bytes(std::span{signature.c_str(), signature.size()}), *key, std::addressof(openssl_err));
      })};
  return verified ? static_cast<int64_t>(*verified) : openssl_err;
}

namespace {

constexpr int64_t OPENSSL_RAW_DATA{1};
constexpr int64_t OPENSSL_ZERO_PADDING{2};
constexpr int64_t OPENSSL_DONT_ZERO_PAD_KEY{4};

// The largest IV and key of the ciphers of OpenSSL, `EVP_MAX_IV_LENGTH` and `EVP_MAX_KEY_LENGTH`.
constexpr size_t MAX_IV_LEN{16};
constexpr size_t MAX_KEY_LEN{64};

constexpr auto CIPHER_METHODS{[]() noexcept {
  auto arr{std::array<std::string_view, 4>{"aes-128-cbc", "aes-256-cbc", "aes-128-gcm", "aes-256-gcm"}};
  std::ranges::sort(arr);
  return arr;
}()};

// returns the name from `CIPHER_METHODS` that matches `method` ignoring the case
std::optional<std::string_view> find_cipher_method(std::string_view method) noexcept {
  auto lowered{std::ranges::transform_view{method, vk::tolower_ascii}};

  auto rng{std::ranges::equal_range(CIPHER_METHODS, lowered, std::ranges::lexicographical_compare)};
  if (!rng.empty()) [[likely]] {
    return rng.front();
  }

  return std::nullopt;
}

std::expected<kphp::openssl::cipher, int32_t> cipher_by_method(const string& method) noexcept {
  const auto name{find_cipher_method({method.c_str(), method.size()})};
  if (!name) {
    return std::unexpected{EINVAL};
  }

  return kphp::openssl::cipher::create_by_name(*name);
}

struct cipher_session {
  kphp::openssl::cipher type;
  // the context of the instance: the first `init_ex` with a cipher resets the state left by the previous session
  const kphp::openssl::cipher_ctx& ctx;
  size_t key_len;
  size_t iv_len;
  size_t block_size;
  bool encrypt;
  bool aead;
  bool single_run_aead;
};

std::optional<cipher_session> open_cipher_session(const string& method, bool encrypt) noexcept {
  auto type{cipher_by_method(method)};
  if (!type) {
    kphp::log::warning("Unknown cipher algorithm '{}'", method.c_str());
    return std::nullopt;
  }

  const auto mode{type->mode()};
  const auto key_len{type->key_length()};
  const auto iv_len{type->iv_length()};
  const auto block_size{type->block_size()};
  if (!mode || !key_len || !iv_len || !block_size) [[unlikely]] {
    kphp::log::warning("Failed to get the parameters of cipher '{}'", method.c_str());
    return std::nullopt;
  }

  const auto ctx{kphp::openssl::crypto_instance_state::get().cipher_ctx()};
  if (!ctx) {
    kphp::log::warning("Failed to create cipher context");
    return std::nullopt;
  }

  const bool is_gcm{*mode == k2::CipherMode::CIPHER_MODE_GCM};
  const bool is_ccm{*mode == k2::CipherMode::CIPHER_MODE_CCM};
  return cipher_session{.type = std::move(*type),
                        .ctx = **ctx,
                        .key_len = static_cast<size_t>(*key_len),
                        .iv_len = static_cast<size_t>(*iv_len),
                        .block_size = static_cast<size_t>(*block_size),
                        .encrypt = encrypt,
                        .aead = is_gcm || is_ccm,
                        .single_run_aead = is_ccm};
}

// `tag` is the tag to check when decrypting and the requested tag, the length of which matters, when encrypting
bool init_cipher(const cipher_session& session, const string& key, const string& iv, int64_t options, const string& tag) noexcept {
  const auto& ctx{session.ctx};
  const int32_t enc{session.encrypt ? 1 : 0};

  if (session.encrypt && iv.empty() && session.iv_len > 0) {
    kphp::log::warning("Using an empty Initialization Vector (iv) is potentially insecure and not recommended");
  }

  if (!ctx.init_ex(session.type, {}, {}, enc)) {
    kphp::log::warning("Cipher init error");
    return false;
  }

  // the IV is the passed one unless it has to be padded or truncated
  std::array<std::byte, MAX_IV_LEN> padded_iv{};
  std::span<const std::byte> iv_bytes{std::as_bytes(std::span{iv.c_str(), iv.size()})};
  if (iv.size() != session.iv_len) {
    if (session.aead) {
      if (!ctx.set_iv_length(static_cast<int32_t>(iv.size()))) {
        kphp::log::warning("Setting of IV length for AEAD mode failed");
        return false;
      }
    } else if (iv.empty()) {
      kphp::log::assertion(session.iv_len <= padded_iv.size());
      iv_bytes = {padded_iv.data(), session.iv_len};
    } else if (iv.size() < session.iv_len) {
      kphp::log::assertion(session.iv_len <= padded_iv.size());
      kphp::log::warning("IV passed is only {} bytes long, cipher expects an IV of precisely {} bytes, padding with \\0", iv.size(), session.iv_len);
      std::memcpy(padded_iv.data(), iv.c_str(), iv.size());
      iv_bytes = {padded_iv.data(), session.iv_len};
    } else {
      kphp::log::warning("IV passed is {} bytes long which is longer than the {} expected by selected cipher, truncating", iv.size(), session.iv_len);
      iv_bytes = iv_bytes.first(session.iv_len);
    }
  }

  if (session.single_run_aead && session.encrypt) {
    if (!ctx.set_tag_length(static_cast<int32_t>(tag.size()))) {
      kphp::log::warning("Setting tag length for AEAD cipher failed");
      return false;
    }
  } else if (!session.encrypt && !tag.empty()) {
    if (!session.aead) {
      kphp::log::warning("The tag cannot be used because the cipher method does not support AEAD");
    } else if (!ctx.set_tag(std::as_bytes(std::span{tag.c_str(), tag.size()}))) {
      kphp::log::warning("Setting tag for AEAD cipher decryption failed");
      return false;
    }
  }

  // a short key is always padded with zeros, `OPENSSL_DONT_ZERO_PAD_KEY` only makes the cipher accept the key of the shorter length as well
  std::array<std::byte, MAX_KEY_LEN> padded_key{};
  std::span<const std::byte> key_bytes{std::as_bytes(std::span{key.c_str(), key.size()})};
  if (session.key_len > key.size()) {
    if ((options & OPENSSL_DONT_ZERO_PAD_KEY) && !ctx.set_key_length(static_cast<int32_t>(key.size()))) {
      kphp::log::warning("Key length cannot be set for the cipher method");
      return false;
    }

    kphp::log::assertion(session.key_len <= padded_key.size());
    std::memcpy(padded_key.data(), key.c_str(), key.size());
    key_bytes = {padded_key.data(), session.key_len};
  } else if (key.size() > session.key_len && !ctx.set_key_length(static_cast<int32_t>(key.size()))) {
    kphp::log::warning("Key length cannot be set for the cipher method");
  }

  if (!ctx.init_ex(key_bytes, iv_bytes, enc)) {
    kphp::log::warning("Cipher init error");
    return false;
  }

  if (options & OPENSSL_ZERO_PADDING) {
    ctx.set_padding(0);
  }

  return true;
}

std::optional<string> process_cipher_data(const cipher_session& session, const string& data, const string& aad) noexcept {
  const auto& ctx{session.ctx};

  if (data.size() > std::numeric_limits<int32_t>::max()) [[unlikely]] {
    kphp::log::warning("Data is too long for the cipher");
    return std::nullopt;
  }

  if (session.single_run_aead && !ctx.set_data_length(static_cast<int32_t>(data.size()))) {
    kphp::log::warning("Setting of data length failed");
    return std::nullopt;
  }

  if (session.aead && !ctx.update({}, std::as_bytes(std::span{aad.c_str(), aad.size()}))) {
    kphp::log::warning("Setting of additional application data failed");
    return std::nullopt;
  }

  string out{static_cast<string::size_type>(data.size() + session.block_size), false};
  const auto out_bytes{std::as_writable_bytes(std::span{out.buffer(), out.size()})};
  const auto out_len{ctx.update(out_bytes, std::as_bytes(std::span{data.c_str(), data.size()}))};
  if (!out_len) {
    kphp::log::warning("Cipher update error");
    return std::nullopt;
  }

  const auto final_len{ctx.final_ex(out_bytes.subspan(*out_len))};
  if (!final_len) {
    kphp::log::warning("Cipher finalize error");
    return std::nullopt;
  }

  out.shrink(static_cast<string::size_type>(*out_len + *final_len));
  return out;
}

// the tag is received only after the data is processed
bool make_tag(const cipher_session& session, string& tag) noexcept {
  if (session.aead && !tag.empty()) {
    if (session.ctx.get_tag(std::as_writable_bytes(std::span{tag.buffer(), tag.size()}))) {
      return true;
    }

    kphp::log::warning("Retrieving verification tag failed");
    return false;
  }

  if (!tag.empty()) {
    kphp::log::warning("The authenticated tag cannot be provided for cipher that doesn not support AEAD");
  } else if (session.aead) {
    kphp::log::warning("A tag should be provided when using AEAD mode");
    return false;
  }

  tag = string{};
  return true;
}

// `tag_out` is `nullptr` when decrypting and points to `tag` when encrypting
Optional<string> eval_cipher(bool encrypt, const string& data, const string& method, const string& key, int64_t options, const string& iv, const string& tag,
                             string* tag_out, const string& aad) noexcept {
  const auto session{open_cipher_session(method, encrypt)};
  if (!session || !init_cipher(*session, key, iv, options, tag)) {
    return false;
  }

  auto result{process_cipher_data(*session, data, aad)};
  if (!result || (encrypt && !make_tag(*session, *tag_out))) {
    return false;
  }

  return std::move(*result);
}

} // namespace

array<string> f$openssl_get_cipher_methods([[maybe_unused]] bool aliases) noexcept {
  array<string> res{array_size{CIPHER_METHODS.size(), true}};
  for (const auto method : CIPHER_METHODS) {
    res.push_back(string{method.data(), static_cast<string::size_type>(method.size())});
  }

  return res;
}

Optional<int64_t> f$openssl_cipher_iv_length(const string& algorithm) noexcept {
  const auto iv_len{cipher_by_method(algorithm).and_then([](const kphp::openssl::cipher& c) noexcept { return c.iv_length(); })};
  if (!iv_len) {
    kphp::log::warning("Unknown cipher algorithm");
    return false;
  }

  return static_cast<int64_t>(*iv_len);
}

Optional<string> f$openssl_encrypt(const string& data, const string& algorithm, const string& key, int64_t options, const string& iv,
                                   std::optional<std::reference_wrapper<string>> tag, const string& aad, int64_t tag_length) noexcept {
  // the length matters for the tag only when it is requested, the tag itself is received by `eval_cipher`
  string out_tag;
  if (tag) {
    if (tag_length < 0 || tag_length > string::max_size()) [[unlikely]] {
      kphp::log::warning("Invalid tag length {}", tag_length);
      return false;
    }

    out_tag = string{static_cast<string::size_type>(tag_length), false};
  }

  auto result{eval_cipher(true, data, algorithm, key, options, iv, out_tag, std::addressof(out_tag), aad)};
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

Optional<string> f$openssl_decrypt(const string& data, const string& algorithm, const string& key, int64_t options, const string& iv, const string& tag,
                                   const string& aad) noexcept {
  if (options & OPENSSL_RAW_DATA) {
    return eval_cipher(false, data, algorithm, key, options, iv, tag, nullptr, aad);
  }

  auto decoded_data{f$base64_decode(data, true)};
  if (!decoded_data.has_value()) [[unlikely]] {
    kphp::log::warning("failed to base64 decode the input");
    return false;
  }

  return eval_cipher(false, decoded_data.val(), algorithm, key, options, iv, tag, nullptr, aad);
}

Optional<string> f$openssl_pkey_get_public(const string& public_key) noexcept {
  if (borrow_pkey_handle(public_key, PUBLIC_KEY_HANDLE_TAG)) {
    return public_key;
  }

  auto key{parse_public_key(public_key)};
  if (!key) [[unlikely]] {
    kphp::log::warning("Parameter key is not a valid key");
    return false;
  }

  return make_pkey_handle(std::move(*key), PUBLIC_KEY_HANDLE_TAG);
}

Optional<string> f$openssl_pkey_get_private(const string& private_key, const string& passphrase) noexcept {
  if (borrow_pkey_handle(private_key, PRIVATE_KEY_HANDLE_TAG)) {
    return private_key;
  }

  auto key{read_private_key(private_key, passphrase)};
  if (!key) [[unlikely]] {
    kphp::log::warning("Parameter key is not a valid key or passphrase is not a valid password");
    return false;
  }

  return make_pkey_handle(std::move(*key), PRIVATE_KEY_HANDLE_TAG);
}

bool f$openssl_public_encrypt(const string& data, string& encrypted_data, const string& public_key) noexcept {
  const auto key{load_public_key(public_key)};
  if (!key) [[unlikely]] {
    kphp::log::warning("Parameter key is not a valid public key");
    encrypted_data = string{};
    return false;
  }

  if (const auto is_rsa{key->is_rsa()}; !is_rsa || !*is_rsa) [[unlikely]] {
    kphp::log::warning("Key type is neither RSA nor RSA2");
    encrypted_data = string{};
    return false;
  }

  const auto size{key->size()};
  const auto rsa{key->get1_rsa()};
  if (size && rsa) {
    string buffer{static_cast<string::size_type>(*size), false};
    const auto from{std::as_bytes(std::span{data.c_str(), data.size()})};
    const auto to{std::as_writable_bytes(std::span{buffer.buffer(), buffer.size()})};
    const auto written{rsa->public_encrypt(from, to, k2::RsaPadding::RSA_PADDING_PKCS1)};
    if (written && *written == *size) {
      encrypted_data = std::move(buffer);
      return true;
    }
  }

  kphp::log::warning("RSA public encrypt failed");
  encrypted_data = string{};
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
    kphp::log::warning("Parameter key is not a valid private key");
    return false;
  }

  if (const auto is_rsa{key->is_rsa()}; !is_rsa || !*is_rsa) [[unlikely]] {
    kphp::log::warning("Key type is not an RSA nor RSA2");
    return false;
  }

  const auto size{key->size()};
  const auto rsa{key->get1_rsa()};
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

// the one-shot function of OpenSSL of the algorithm, as `algo_function` of the legacy runtime
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

string hash_impl(const hash_algorithm_traits& traits, const string& data, bool binary) noexcept {
  string hashed_data{binary ? static_cast<string::size_type>(traits.hashed_size) : static_cast<string::size_type>(traits.hashed_size * 2), false};
  const auto data_bytes{std::as_bytes(std::span{data.c_str(), data.size()})};
  const auto hashed_bytes{std::as_writable_bytes(std::span{hashed_data.buffer(), static_cast<size_t>(traits.hashed_size)})};
  if (!traits.hash(data_bytes, hashed_bytes)) [[unlikely]] {
    kphp::log::warning("failed to generate hash");
    return {};
  }

  if (!binary) {
    kphp::strings::bin2hex(hashed_data.buffer(), traits.hashed_size, hashed_data.size());
  }

  return hashed_data;
}

// `HMAC` of OpenSSL is `HMAC_Init_ex`, `HMAC_Update` and `HMAC_Final` on a fresh context, here they are called on the context of the instance
std::expected<uint32_t, int32_t> hmac(k2::HashAlgorithm algorithm, std::span<const std::byte> key, std::span<const std::byte> data,
                                      std::span<std::byte> md) noexcept {
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

  return hash_impl(*algo, data, binary);
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
  if (!md_len) [[unlikely]] {
    kphp::log::warning("failed to generate HMAC hash");
    return {};
  }

  kphp::log::assertion(static_cast<int64_t>(*md_len) == algo->hashed_size);

  if (!binary) {
    kphp::strings::bin2hex(hashed_data.buffer(), algo->hashed_size, hashed_data.size());
  }

  return hashed_data;
}

string f$sha1(const string& data, bool binary) noexcept {
  return hash_impl(hash_algorithm_traits{SHA1_HASHED_SIZE, k2::HashAlgorithm::HASH_SHA1, k2::openssl_sha1}, data, binary);
}

int64_t f$crc32(const string& s) noexcept {
  return crc32_partial_generic(static_cast<const void*>(s.c_str()), s.size(), -1) ^ -1;
}
