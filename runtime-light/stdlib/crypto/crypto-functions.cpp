//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/crypto/crypto-functions.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <expected>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "common/crc32_generic.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/server/url-functions.h"
#include "runtime-common/stdlib/string/string-functions.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/crypto/crypto-state.h"
#include "runtime-light/stdlib/crypto/openssl.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/string/string-functions.h"

Optional<string> f$openssl_random_pseudo_bytes(int64_t length) noexcept {
  if (length <= 0 || length > string::max_size()) [[unlikely]] {
    return false;
  }

  string buffer{static_cast<string::size_type>(length), false};
  if (!k2::openssl_rand_bytes(kphp::strings::as_writable_bytes(buffer))) [[unlikely]] {
    kphp::log::warning("failed to generate pseudo-random bytes");
    return false;
  }

  return buffer;
}

namespace {

Optional<array<mixed>> x509_get_name_entries(const kphp::openssl::x509& cert, bool is_issuer, bool shortnames) noexcept {
  auto count{cert.name_entry_count(is_issuer)};
  if (!count) [[unlikely]] {
    kphp::log::warning("failed to get X509 name entry count");
    return false;
  }

  array<mixed> res{array_size{static_cast<int64_t>(count.value()), false}};
  for (size_t i = 0; i < count.value(); ++i) {
    auto key{cert.name_entry_key(is_issuer, shortnames, i)};
    if (!key) [[unlikely]] {
      kphp::log::warning("failed to get X509 name entry key");
      return false;
    }

    auto value{cert.name_entry_value(is_issuer, i)};
    if (!value) [[unlikely]] {
      kphp::log::warning("failed to get X509 name entry value");
      return false;
    }

    res.set_value(string{key.value().data(), static_cast<string::size_type>(key.value().size())},
                  string{value.value().data(), static_cast<string::size_type>(value.value().size())});
  }

  return res;
}

Optional<array<mixed>> x509_get_purposes(const kphp::openssl::x509& cert, bool shortnames) noexcept {
  auto count{k2::openssl_x509_purpose_count()};
  if (!count) [[unlikely]] {
    kphp::log::warning("failed to get X509 purpose count");
    return false;
  }

  array<mixed> res{array_size{static_cast<int64_t>(count.value()), false}};
  for (size_t i = 0; i < count.value(); ++i) {
    auto id{k2::openssl_x509_purpose_id(i)};
    if (!id) [[unlikely]] {
      kphp::log::warning("failed to get X509 purpose id");
      return false;
    }

    auto purpset_0{cert.purpose_check(i, false)};
    auto purpset_1{cert.purpose_check(i, true)};
    if (!purpset_0 || !purpset_1) [[unlikely]] {
      kphp::log::warning("failed to check X509 purpose");
      return false;
    }

    auto name{k2::openssl_x509_purpose_name(i, shortnames)};
    if (!name) [[unlikely]] {
      kphp::log::warning("failed to get X509 purpose name");
      return false;
    }

    res.set_value(id.value(),
                  array<mixed>::create(purpset_0.value(), purpset_1.value(), string{name.value().data(), static_cast<string::size_type>(name.value().size())}));
  }

  return res;
}

} // namespace

Optional<array<mixed>> f$openssl_x509_parse(const string& data, bool shortnames) noexcept {
  static constexpr size_t SUBJECT_NAME_HASH_SIZE{8};

  const auto cert{kphp::openssl::x509::parse(kphp::strings::as_bytes(data))};
  if (!cert) [[unlikely]] {
    kphp::log::warning("failed to parse X509 certificate");
    return false;
  }

  auto name{cert->subject_name()};
  if (!name) [[unlikely]] {
    kphp::log::warning("failed to get X509 subject name");
    return false;
  }

  auto subject{x509_get_name_entries(*cert, false, shortnames)};
  if (!subject.has_value()) [[unlikely]] {
    return false;
  }

  string hash{SUBJECT_NAME_HASH_SIZE, false};
  if (!cert->hash(kphp::strings::as_writable_bytes(hash))) [[unlikely]] {
    kphp::log::warning("failed to get X509 hash");
    return false;
  }

  auto issuer{x509_get_name_entries(*cert, true, shortnames)};
  if (!issuer.has_value()) [[unlikely]] {
    return false;
  }

  auto version{cert->version()};
  if (!version) [[unlikely]] {
    kphp::log::warning("failed to get X509 version");
    return false;
  }

  auto valid_from{cert->valid_from()};
  if (!valid_from) [[unlikely]] {
    kphp::log::warning("failed to get X509 validFrom time");
    return false;
  }

  auto valid_to{cert->valid_to()};
  if (!valid_to) [[unlikely]] {
    kphp::log::warning("failed to get X509 validTo time");
    return false;
  }

  auto purposes{x509_get_purposes(*cert, shortnames)};
  if (!purposes.has_value()) [[unlikely]] {
    return false;
  }

  const auto& crypto_state{CryptoImageState::get()};

  return std::initializer_list<std::pair<string, mixed>>{
      {crypto_state.NAME_STR, string{name.value().data(), static_cast<string::size_type>(name.value().size())}},
      {crypto_state.SUBJECT_STR, subject.val()},
      {crypto_state.HASH_STR, hash},
      {crypto_state.ISSUER_STR, issuer.val()},
      {crypto_state.VERSION_STR, version.value()},
      {crypto_state.VALID_FROM_TIME_T_STR, valid_from.value()},
      {crypto_state.VALID_TO_TIME_T_STR, valid_to.value()},
      {crypto_state.PURPOSES_STR, purposes.val()}};
}

namespace {

// A key argument is either a handle returned by openssl_pkey_get_{public,private} or a PEM string.
// A handle is the raw bytes of a platform descriptor that lives until the end of the instance.
// No valid PEM is that short, so the size alone tells them apart.
std::expected<kphp::openssl::pkey, int32_t> borrow_pkey_handle(const string& key) noexcept {
  if (key.size() != sizeof(k2::descriptor)) {
    return std::unexpected{k2::errno_einval};
  }

  k2::descriptor descriptor{};
  std::memcpy(std::addressof(descriptor), key.c_str(), sizeof(descriptor));

  return kphp::openssl::pkey::borrow(descriptor);
}

// Gives the key to the instance: the platform frees its descriptor when the instance ends.
string make_pkey_handle(kphp::openssl::pkey&& key) noexcept {
  const auto descriptor{key.release()};
  return {reinterpret_cast<const char*>(std::addressof(descriptor)), static_cast<string::size_type>(sizeof(descriptor))};
}

// Accepts either a public key or a certificate, in which case the key is taken from it.
std::expected<kphp::openssl::pkey, int32_t> parse_public_key(const string& key) noexcept {
  const auto pem{kphp::strings::as_bytes(key)};
  return kphp::openssl::pkey::get_public(pem).or_else(
      [pem](int32_t) noexcept { return kphp::openssl::x509::parse(pem).and_then([](const kphp::openssl::x509& cert) noexcept { return cert.pubkey(); }); });
}

std::expected<kphp::openssl::pkey, int32_t> parse_private_key(const string& key, const string& passphrase) noexcept {
  return kphp::openssl::pkey::get_private(kphp::strings::as_bytes(key), kphp::strings::as_bytes(passphrase));
}

std::expected<kphp::openssl::pkey, int32_t> load_public_key(const string& key) noexcept {
  return borrow_pkey_handle(key).or_else([&key](int32_t) noexcept { return parse_public_key(key); });
}

std::expected<kphp::openssl::pkey, int32_t> load_private_key(const string& key) noexcept {
  return borrow_pkey_handle(key).or_else([&key](int32_t) noexcept { return parse_private_key(key, {}); });
}

} // namespace

bool f$openssl_sign(const string& data, string& signature, const string& private_key, int64_t algorithm) noexcept {
  const auto key{load_private_key(private_key)};
  if (!key) [[unlikely]] {
    kphp::log::warning("failed to sign data");
    return false;
  }

  auto size{key->size()};
  if (!size) [[unlikely]] {
    kphp::log::warning("failed to sign data");
    return false;
  }

  string buffer{static_cast<string::size_type>(size.value()), false};
  auto written{key->sign(static_cast<k2::HashAlgorithm>(algorithm), kphp::strings::as_bytes(data), kphp::strings::as_writable_bytes(buffer))};
  if (!written) [[unlikely]] {
    kphp::log::warning("failed to sign data");
    return false;
  }

  buffer.shrink(static_cast<string::size_type>(written.value()));
  signature = std::move(buffer);

  return true;
}

int64_t f$openssl_verify(const string& data, const string& signature, const string& public_key, int64_t algorithm) noexcept {
  const auto key{load_public_key(public_key)};
  if (!key) [[unlikely]] {
    kphp::log::warning("failed to verify signature");
    return -1;
  }

  auto res{key->verify(static_cast<k2::HashAlgorithm>(algorithm), kphp::strings::as_bytes(data), kphp::strings::as_bytes(signature))};
  if (!res) [[unlikely]] {
    kphp::log::warning("failed to verify signature");
    return -1;
  }

  return res.value() ? 1 : 0;
}

namespace {

constexpr std::string_view AES_128_CBC{"aes-128-cbc"};
constexpr std::string_view AES_256_CBC{"aes-256-cbc"};
constexpr std::string_view AES_128_GCM{"aes-128-gcm"};
constexpr std::string_view AES_256_GCM{"aes-256-gcm"};

enum class encryption_mode { BLOCK, STREAM, AEAD };

struct cipher_algorithm_traits {
  int64_t key_len;
  int64_t iv_len;
  int64_t block_size;
  encryption_mode mode;
  k2::CipherAlgorithm algo;
};

constexpr auto CIPHER_ALGORITHMS{[] {
  auto arr{std::array{std::pair{AES_128_CBC, cipher_algorithm_traits{16, 16, 16, encryption_mode::BLOCK, AES128_CBC}},
                      std::pair{AES_256_CBC, cipher_algorithm_traits{32, 16, 16, encryption_mode::BLOCK, AES256_CBC}},
                      std::pair{AES_128_GCM, cipher_algorithm_traits{16, 12, 0, encryption_mode::AEAD, AES128_GCM}},
                      std::pair{AES_256_GCM, cipher_algorithm_traits{32, 12, 0, encryption_mode::AEAD, AES256_GCM}}}};
  std::ranges::sort(arr, std::less{}, &std::pair<std::string_view, cipher_algorithm_traits>::first);
  return arr;
}()};

static_assert(CIPHER_ALGORITHMS.size() == k2::CipherAlgorithm::COUNT_CIPHER);

std::optional<std::reference_wrapper<const cipher_algorithm_traits>> parse_cipher_algorithm(std::string_view algorithm) noexcept {
  auto lowered{std::ranges::transform_view{algorithm, [](char c) noexcept -> char {
                                             if (c >= 'A' && c <= 'Z') {
                                               return c + ('a' - 'A');
                                             }
                                             return c;
                                           }}};

  auto rng{
      std::ranges::equal_range(CIPHER_ALGORITHMS, lowered, std::ranges::lexicographical_compare, &std::pair<std::string_view, cipher_algorithm_traits>::first)};
  if (!rng.empty()) [[likely]] {
    return rng.front().second;
  }

  return std::nullopt;
}

auto calc_encrypted_size(const cipher_algorithm_traits& traits, int64_t tag_size, int64_t data_len) noexcept {
  struct encrypted_data_size_info {
    int64_t encrypted_size;
    int64_t tag_size;
  };

  switch (traits.mode) {
  case encryption_mode::BLOCK:
    return encrypted_data_size_info{data_len + traits.block_size, 0};
  case encryption_mode::STREAM:
    return encrypted_data_size_info{data_len, 0};
  case encryption_mode::AEAD:
    return encrypted_data_size_info{data_len, tag_size};
  }
}

auto calc_decrypted_size(const cipher_algorithm_traits& traits, int64_t data_len) noexcept {
  switch (traits.mode) {
  case encryption_mode::BLOCK:
    return data_len + traits.block_size;
  case encryption_mode::STREAM:
  case encryption_mode::AEAD:
    return data_len;
  }
}

enum class cipher_opts : int64_t { OPENSSL_RAW_DATA = 1, OPENSSL_ZERO_PADDING = 2, OPENSSL_DONT_ZERO_PAD_KEY = 4 };

std::optional<std::pair<string, string>> algorithm_pad_key_iv(const cipher_algorithm_traits& traits, const string& source_key, const string& source_iv,
                                                              int64_t options) noexcept {
  auto iv = source_iv;
  if (traits.mode == encryption_mode::AEAD) {
    if (source_iv.empty()) [[unlikely]] {
      kphp::log::warning("setting of IV length for AEAD mode failed");
      return std::nullopt;
    }
  } else {
    int64_t iv_required_len{traits.iv_len};
    if (iv.size() < iv_required_len) {
      kphp::log::warning("iv passed is only {} bytes long, cipher expects an IV of precisely {} bytes, padding with \\0", iv.size(), iv_required_len);
      iv.append(static_cast<string::size_type>(iv_required_len - iv.size()), '\0');
    } else if (iv.size() > iv_required_len) {
      kphp::log::warning("iv passed is {} bytes long which is longer than the {} expected by selected cipher, truncating", iv.size(), iv_required_len);
      iv.shrink(static_cast<string::size_type>(iv_required_len));
    }
  }

  auto key = source_key;
  int64_t key_required_len{traits.key_len};
  if (key.size() < key_required_len) {
    if (options & static_cast<int64_t>(cipher_opts::OPENSSL_DONT_ZERO_PAD_KEY)) [[unlikely]] {
      kphp::log::warning("key length should be {} bytes long:\n", key_required_len);
      return std::nullopt;
    }
    kphp::log::warning("passphrase passed is only {} bytes long, cipher expects an passphrase of precisely {} bytes, padding with \\0", key.size(),
                       key_required_len);
    key.append(key_required_len - key.size(), '\0');
  } else if (key.size() > key_required_len) {
    kphp::log::warning("passphrase passed is {} bytes long which is longer than the {} expected by selected cipher, truncating", key.size(), key_required_len);
    key.shrink(static_cast<string::size_type>(key_required_len));
  }

  return std::make_pair(std::move(key), std::move(iv));
}

} // namespace

array<string> f$openssl_get_cipher_methods([[maybe_unused]] bool aliases) noexcept {
  array<string> res{array_size{CIPHER_ALGORITHMS.size(), true}};
  for (const auto& [algo, _] : CIPHER_ALGORITHMS) {
    res.push_back(string{algo.data(), static_cast<string::size_type>(algo.size())});
  }

  return res;
}

Optional<int64_t> f$openssl_cipher_iv_length(const string& algorithm) noexcept {
  auto algo{parse_cipher_algorithm({algorithm.c_str(), algorithm.size()})};
  if (!algo) {
    kphp::log::warning("unknown cipher algorithm {}", algorithm.c_str());
    return false;
  }

  return algo->get().iv_len;
}

Optional<string> f$openssl_encrypt(const string& data, const string& algorithm, const string& key, int64_t options, const string& iv,
                                   std::optional<std::reference_wrapper<string>> tag, const string& aad, [[maybe_unused]] int64_t tag_length) noexcept {
  auto algo{parse_cipher_algorithm({algorithm.c_str(), algorithm.size()})};
  if (!algo) [[unlikely]] {
    kphp::log::warning("unknown cipher algorithm {}", algorithm.c_str());
    return false;
  }

  const auto& traits{algo->get()};
  bool is_aead{traits.mode == encryption_mode::AEAD};
  if (is_aead && (!tag || tag_length == 0)) [[unlikely]] {
    kphp::log::warning("a tag must be provided when using AEAD mode");
    return false;
  }

  if (!is_aead) {
    if (tag) {
      kphp::log::warning("the authenticated tag cannot be provided for cipher that does not support AEAD");
    }

    if (!aad.empty()) {
      kphp::log::warning("the additional authenticated data cannot be provided for cipher that does not support AEAD");
    }
  }

  if (iv.empty()) {
    kphp::log::warning("using an empty nitialization vector (iv) is potentially insecure and not recommended");
  }

  auto key_iv{algorithm_pad_key_iv(traits, key, iv, options)};
  if (!key_iv) [[unlikely]] {
    return false;
  }

  k2::BlockPadding padding{k2::BlockPadding::PKCS7};
  if (options & static_cast<int64_t>(cipher_opts::OPENSSL_ZERO_PADDING)) {
    padding = k2::BlockPadding::BLOCK_NO_PADDING;
  }

  auto encrypted_size{calc_encrypted_size(traits, tag_length, data.size())};
  string encrypted_data{static_cast<string::size_type>(encrypted_size.encrypted_size), false};
  string received_tag{static_cast<string::size_type>(encrypted_size.tag_size), false};
  auto res{k2::openssl_encrypt(traits.algo, padding, kphp::strings::as_bytes(key_iv->first), kphp::strings::as_bytes(key_iv->second),
                               kphp::strings::as_bytes(aad), kphp::strings::as_bytes(data), kphp::strings::as_writable_bytes(encrypted_data),
                               kphp::strings::as_writable_bytes(received_tag))};
  if (!res) [[unlikely]] {
    kphp::log::warning("failed to encrypt data");
    return false;
  }

  encrypted_data.shrink(static_cast<string::size_type>(*res));

  if (tag) {
    (*tag).get() = std::move(received_tag);
  }

  return (options & static_cast<int64_t>(cipher_opts::OPENSSL_RAW_DATA)) ? std::move(encrypted_data) : f$base64_encode(encrypted_data);
}

Optional<string> f$openssl_decrypt(const string& data, const string& algorithm, const string& key, int64_t options, const string& iv, const string& tag,
                                   const string& aad) noexcept {
  auto algo{parse_cipher_algorithm({algorithm.c_str(), algorithm.size()})};
  if (!algo) [[unlikely]] {
    kphp::log::warning("unknown cipher algorithm {}", algorithm.c_str());
    return false;
  }

  const auto& traits{algo->get()};
  bool is_aead{traits.mode == encryption_mode::AEAD};
  if (is_aead && tag.empty()) [[unlikely]] {
    kphp::log::warning("a tag must be provided when using AEAD mode");
    return false;
  }

  if (!is_aead) {
    if (!tag.empty()) {
      kphp::log::warning("the authenticated tag cannot be provided for cipher that does not support AEAD");
    }

    if (!aad.empty()) {
      kphp::log::warning("the additional authenticated data cannot be provided for cipher that does not support AEAD");
    }
  }

  string input_data{data};
  if (!(options & static_cast<int64_t>(cipher_opts::OPENSSL_RAW_DATA))) {
    Optional<string> decoding_data{f$base64_decode(data, true)};
    if (!decoding_data.has_value()) [[unlikely]] {
      kphp::log::warning("failed to base64 decode the input");
      return false;
    }

    input_data = std::move(decoding_data.val());
  }

  auto key_iv{algorithm_pad_key_iv(traits, key, iv, options)};
  if (!key_iv) [[unlikely]] {
    return false;
  }

  k2::BlockPadding padding{k2::BlockPadding::PKCS7};
  if (options & static_cast<int64_t>(cipher_opts::OPENSSL_ZERO_PADDING)) {
    padding = k2::BlockPadding::BLOCK_NO_PADDING;
  }

  string decrypted_data{static_cast<string::size_type>(calc_decrypted_size(traits, input_data.size())), false};
  auto res{k2::openssl_decrypt(traits.algo, padding, kphp::strings::as_bytes(key_iv->first), kphp::strings::as_bytes(key_iv->second),
                               kphp::strings::as_bytes(aad), kphp::strings::as_bytes(tag), kphp::strings::as_bytes(input_data),
                               kphp::strings::as_writable_bytes(decrypted_data))};
  if (!res) [[unlikely]] {
    kphp::log::warning("failed to decrypt data");
    return false;
  }

  decrypted_data.shrink(static_cast<string::size_type>(*res));

  return std::move(decrypted_data);
}

Optional<string> f$openssl_pkey_get_public(const string& public_key) noexcept {
  if (const auto handle{borrow_pkey_handle(public_key)}; handle) {
    if (!handle->is_public().value_or(false)) [[unlikely]] {
      kphp::log::warning("failed to get public key");
      return false;
    }

    return public_key;
  }

  auto key{parse_public_key(public_key)};
  if (!key) [[unlikely]] {
    kphp::log::warning("failed to get public key");
    return false;
  }

  return make_pkey_handle(std::move(*key));
}

Optional<string> f$openssl_pkey_get_private(const string& private_key, const string& passphrase) noexcept {
  if (const auto handle{borrow_pkey_handle(private_key)}; handle) {
    if (!handle->is_private().value_or(false)) [[unlikely]] {
      kphp::log::warning("failed to get private key");
      return false;
    }

    return private_key;
  }

  auto key{parse_private_key(private_key, passphrase)};
  if (!key) [[unlikely]] {
    kphp::log::warning("failed to get private key");
    return false;
  }

  return make_pkey_handle(std::move(*key));
}

bool f$openssl_public_encrypt(const string& data, string& encrypted_data, const string& public_key) noexcept {
  const auto key{load_public_key(public_key)};
  if (!key) [[unlikely]] {
    kphp::log::warning("failed to encrypt data with the given public key");
    return false;
  }

  auto size{key->size()};
  if (!size) [[unlikely]] {
    kphp::log::warning("failed to encrypt data with the given public key");
    return false;
  }

  string buffer{static_cast<string::size_type>(size.value()), false};
  auto written{key->public_encrypt(kphp::strings::as_bytes(data), k2::RsaPadding::PKCS1, kphp::strings::as_writable_bytes(buffer))};
  if (!written) [[unlikely]] {
    kphp::log::warning("failed to encrypt data with the given public key");
    return false;
  }

  buffer.shrink(static_cast<string::size_type>(written.value()));
  encrypted_data = std::move(buffer);

  return true;
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
    kphp::log::warning("failed to decrypt data with the given private key");
    return false;
  }

  auto size{key->size()};
  if (!size) [[unlikely]] {
    kphp::log::warning("failed to decrypt data with the given private key");
    return false;
  }

  string buffer{static_cast<string::size_type>(size.value()), false};
  auto written{key->private_decrypt(kphp::strings::as_bytes(data), k2::RsaPadding::PKCS1, kphp::strings::as_writable_bytes(buffer))};
  if (!written) [[unlikely]] {
    kphp::log::warning("failed to decrypt data with the given private key");
    return false;
  }

  buffer.shrink(static_cast<string::size_type>(written.value()));
  decrypted_data = std::move(buffer);

  return true;
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

constexpr std::string_view DSS1{"dss1"};
constexpr std::string_view SHA1{"sha1"};
constexpr std::string_view SHA224{"sha224"};
constexpr std::string_view SHA256{"sha256"};
constexpr std::string_view SHA384{"sha384"};
constexpr std::string_view SHA512{"sha512"};
constexpr std::string_view MD5{"md5"};
constexpr std::string_view RMD160{"rmd160"};
constexpr std::string_view MD2{"md2"};
constexpr std::string_view MD4{"md4"};

constexpr size_t DSS1_HASHED_SIZE{20};
constexpr size_t SHA1_HASHED_SIZE{20};
constexpr size_t SHA224_HASHED_SIZE{28};
constexpr size_t SHA256_HASHED_SIZE{32};
constexpr size_t SHA384_HASHED_SIZE{48};
constexpr size_t SHA512_HASHED_SIZE{64};
constexpr size_t MD5_HASHED_SIZE{16};
constexpr size_t RMD160_HASHED_SIZE{20};
constexpr size_t MD2_HASHED_SIZE{16};
constexpr size_t MD4_HASHED_SIZE{16};

struct hash_algorithm_traits {
  int64_t hashed_size;
  k2::HashAlgorithm algo;
};

constexpr auto HASH_ALGORITHMS{[]() noexcept {
  auto arr{std::array{std::pair{DSS1, hash_algorithm_traits{DSS1_HASHED_SIZE, k2::HashAlgorithm::DSS1}},
                      std::pair{SHA1, hash_algorithm_traits{SHA1_HASHED_SIZE, k2::HashAlgorithm::SHA1}},
                      std::pair{SHA224, hash_algorithm_traits{SHA224_HASHED_SIZE, k2::HashAlgorithm::SHA224}},
                      std::pair{SHA256, hash_algorithm_traits{SHA256_HASHED_SIZE, k2::HashAlgorithm::SHA256}},
                      std::pair{SHA384, hash_algorithm_traits{SHA384_HASHED_SIZE, k2::HashAlgorithm::SHA384}},
                      std::pair{SHA512, hash_algorithm_traits{SHA512_HASHED_SIZE, k2::HashAlgorithm::SHA512}},
                      std::pair{MD5, hash_algorithm_traits{MD5_HASHED_SIZE, k2::HashAlgorithm::MD5}},
                      std::pair{RMD160, hash_algorithm_traits{RMD160_HASHED_SIZE, k2::HashAlgorithm::RMD160}},
                      std::pair{MD2, hash_algorithm_traits{MD2_HASHED_SIZE, k2::HashAlgorithm::MD2}},
                      std::pair{MD4, hash_algorithm_traits{MD4_HASHED_SIZE, k2::HashAlgorithm::MD4}}}};

  std::ranges::sort(arr, std::less{}, &std::pair<std::string_view, hash_algorithm_traits>::first);
  return arr;
}()};

static_assert(HASH_ALGORITHMS.size() == k2::HashAlgorithm::COUNT_HASH);

std::optional<hash_algorithm_traits> parse_hash_algorithm(std::string_view algorithm) noexcept {
  auto lowered{std::ranges::transform_view{algorithm, [](char c) noexcept -> char {
                                             if (c >= 'A' && c <= 'Z') {
                                               return c + ('a' - 'A');
                                             }
                                             return c;
                                           }}};

  auto rng{
      std::ranges::equal_range(HASH_ALGORITHMS, lowered, std::ranges::lexicographical_compare, &std::pair<std::string_view, hash_algorithm_traits>::first)};
  if (!rng.empty()) [[likely]] {
    return rng.front().second;
  }

  return std::nullopt;
}

string hash_impl(hash_algorithm_traits traits, const string& data, bool binary) noexcept {
  string hashed_data{binary ? static_cast<string::size_type>(traits.hashed_size) : static_cast<string::size_type>(traits.hashed_size * 2), false};
  auto res{k2::hash(traits.algo, kphp::strings::as_bytes(data), kphp::strings::as_writable_bytes(hashed_data).first(static_cast<size_t>(traits.hashed_size)))};
  if (!res) [[unlikely]] {
    kphp::log::warning("failed to generate hash");
    return {};
  }

  if (!binary) {
    kphp::strings::bin2hex(hashed_data.buffer(), traits.hashed_size, hashed_data.size());
  }

  return hashed_data;
}

} // namespace

array<string> f$hash_algos() noexcept {
  array<string> res{array_size{HASH_ALGORITHMS.size(), true}};
  for (auto [algo, _] : HASH_ALGORITHMS) {
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
  auto res{k2::hash_hmac(algo->algo, kphp::strings::as_bytes(data), kphp::strings::as_bytes(key),
                         kphp::strings::as_writable_bytes(hashed_data).first(static_cast<size_t>(algo->hashed_size)))};
  if (!res) [[unlikely]] {
    kphp::log::warning("failed to generate HMAC hash");
    return {};
  }

  if (!binary) {
    kphp::strings::bin2hex(hashed_data.buffer(), algo->hashed_size, hashed_data.size());
  }

  return hashed_data;
}

string f$sha1(const string& data, bool binary) noexcept {
  return hash_impl(hash_algorithm_traits{SHA1_HASHED_SIZE, k2::HashAlgorithm::SHA1}, data, binary);
}

int64_t f$crc32(const string& s) noexcept {
  return crc32_partial_generic(static_cast<const void*>(s.c_str()), s.size(), -1) ^ -1;
}
