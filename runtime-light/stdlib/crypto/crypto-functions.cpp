//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/crypto/crypto-functions.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

#include "common/containers/final_action.h"
#include "common/crc32_generic.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/server/url-functions.h"
#include "runtime-common/stdlib/string/string-functions.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

Optional<string> f$openssl_random_pseudo_bytes(int64_t length) noexcept {
  if (length <= 0 || length > string::max_size()) [[unlikely]] {
    return false;
  }

  string buffer{static_cast<string::size_type>(length), false};
  if (!k2::openssl_random_pseudo_bytes({buffer.buffer(), buffer.size()})) [[unlikely]] {
    kphp::log::warning("Failed to generate pseudo-random bytes");
    return false;
  }

  return buffer;
}

namespace {

Optional<array<mixed>> x509_get_name_entries(k2::descriptor x509_d, bool is_issuer, bool shortnames) noexcept {
  auto count{k2::openssl_x509_name_entry_count(x509_d, is_issuer)};
  if (!count) [[unlikely]] {
    kphp::log::warning("Failed to get X509 name entry count");
    return false;
  }

  array<mixed> res{array_size{static_cast<int64_t>(count.value()), false}};
  for (size_t i = 0; i < count.value(); ++i) {
    auto key{k2::openssl_x509_name_entry_key(x509_d, is_issuer, shortnames, i)};
    if (!key) [[unlikely]] {
      kphp::log::warning("Failed to get X509 name entry key");
      return false;
    }

    auto value{k2::openssl_x509_name_entry_value(x509_d, is_issuer, i)};
    if (!value) [[unlikely]] {
      kphp::log::warning("Failed to get X509 name entry value");
      return false;
    }

    res.set_value(string{key.value().data(), static_cast<string::size_type>(key.value().size())},
                  string{value.value().data(), static_cast<string::size_type>(value.value().size())});
  }

  return res;
}

Optional<array<mixed>> x509_get_purposes(k2::descriptor x509_d, bool shortnames) noexcept {
  auto count{k2::openssl_x509_purpose_count()};
  if (!count) [[unlikely]] {
    kphp::log::warning("Failed to get X509 purpose count");
    return false;
  }

  array<mixed> res{array_size{static_cast<int64_t>(count.value()), false}};
  for (size_t i = 0; i < count.value(); ++i) {
    auto id{k2::openssl_x509_purpose_id(i)};
    if (!id) [[unlikely]] {
      kphp::log::warning("Failed to get X509 purpose id");
      return false;
    }

    auto purpset_0{k2::openssl_x509_purpose_check(x509_d, i, false)};
    auto purpset_1{k2::openssl_x509_purpose_check(x509_d, i, true)};
    if (!purpset_0 || !purpset_1) [[unlikely]] {
      kphp::log::warning("Failed to check X509 purpose");
      return false;
    }

    auto name{k2::openssl_x509_purpose_name(i, shortnames)};
    if (!name) [[unlikely]] {
      kphp::log::warning("Failed to get X509 purpose name");
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

  auto parsed{k2::openssl_x509_parse({data.c_str(), data.size()})};
  if (!parsed) [[unlikely]] {
    kphp::log::warning("Failed to parse X509 certificate");
    return false;
  }

  const k2::descriptor x509_d{parsed.value()};
  const auto x509_finalizer{vk::finally([x509_d]() noexcept { k2::free_descriptor(x509_d); })};

  auto name{k2::openssl_x509_get_subject_name(x509_d)};
  if (!name) [[unlikely]] {
    kphp::log::warning("Failed to get X509 subject name");
    return false;
  }

  auto subject{x509_get_name_entries(x509_d, false, shortnames)};
  if (!subject.has_value()) [[unlikely]] {
    return false;
  }

  string hash{SUBJECT_NAME_HASH_SIZE, false};
  if (!k2::openssl_x509_get_hash(x509_d, {hash.buffer(), hash.size()})) [[unlikely]] {
    kphp::log::warning("Failed to get X509 hash");
    return false;
  }

  auto issuer{x509_get_name_entries(x509_d, true, shortnames)};
  if (!issuer.has_value()) [[unlikely]] {
    return false;
  }

  auto version{k2::openssl_x509_get_version(x509_d)};
  if (!version) [[unlikely]] {
    kphp::log::warning("Failed to get X509 version");
    return false;
  }

  auto valid_from{k2::openssl_x509_get_valid_from(x509_d)};
  if (!valid_from) [[unlikely]] {
    kphp::log::warning("Failed to get X509 validFrom time");
    return false;
  }

  auto valid_to{k2::openssl_x509_get_valid_to(x509_d)};
  if (!valid_to) [[unlikely]] {
    kphp::log::warning("Failed to get X509 validTo time");
    return false;
  }

  auto purposes{x509_get_purposes(x509_d, shortnames)};
  if (!purposes.has_value()) [[unlikely]] {
    return false;
  }

  return std::initializer_list<std::pair<string, mixed>>{{string{"name", 4}, string{name.value().data(), static_cast<string::size_type>(name.value().size())}},
                                                         {string{"subject", 7}, subject.val()},
                                                         {string{"hash", 4}, hash},
                                                         {string{"issuer", 6}, issuer.val()},
                                                         {string{"version", 7}, version.value()},
                                                         {string{"validFrom_time_t", 16}, valid_from.value()},
                                                         {string{"validTo_time_t", 14}, valid_to.value()},
                                                         {string{"purposes", 8}, purposes.val()}};
}

namespace {

// A key argument is either a handle returned by openssl_pkey_get_{public,private} or a PEM string.
// A handle is the raw bytes of a platform descriptor that lives until the end of the instance.
// No valid PEM is that short, so the size alone tells them apart.
std::optional<k2::descriptor> get_pkey_handle(const string& key) noexcept {
  if (key.size() != sizeof(k2::descriptor)) {
    return std::nullopt;
  }

  k2::descriptor descriptor{};
  std::memcpy(std::addressof(descriptor), key.c_str(), sizeof(descriptor));

  return descriptor;
}

string make_pkey_handle(k2::descriptor descriptor) noexcept {
  return {reinterpret_cast<const char*>(std::addressof(descriptor)), static_cast<string::size_type>(sizeof(descriptor))};
}

std::optional<k2::descriptor> parse_public_key(const string& key) noexcept {
  const std::string_view pem{key.c_str(), key.size()};
  if (auto pkey_d{k2::openssl_pkey_get_public(pem)}; pkey_d) {
    return pkey_d.value();
  }

  auto x509_d{k2::openssl_x509_parse(pem)};
  if (!x509_d) {
    return std::nullopt;
  }

  const auto x509_finalizer{vk::finally([x509_d = x509_d.value()]() noexcept { k2::free_descriptor(x509_d); })};

  auto pkey_d{k2::openssl_x509_get_pubkey(x509_d.value())};
  if (!pkey_d) {
    return std::nullopt;
  }

  return pkey_d.value();
}

std::optional<k2::descriptor> parse_private_key(const string& key, const string& passphrase) noexcept {
  auto pkey_d{k2::openssl_pkey_get_private({key.c_str(), key.size()}, {passphrase.c_str(), passphrase.size()})};
  if (!pkey_d) {
    return std::nullopt;
  }

  return pkey_d.value();
}

} // namespace

bool f$openssl_sign(const string& data, string& signature, const string& private_key, int64_t algorithm) noexcept {
  const auto handle{get_pkey_handle(private_key)};
  const auto key_d{handle ? handle : parse_private_key(private_key, {})};
  const auto key_finalizer{vk::finally([&]() noexcept {
    if (!handle && key_d) {
      k2::free_descriptor(key_d.value());
    }
  })};
  if (!key_d) [[unlikely]] {
    kphp::log::warning("Failed to sign data");
    return false;
  }

  auto size{k2::openssl_pkey_size(key_d.value())};
  if (!size) [[unlikely]] {
    kphp::log::warning("Failed to sign data");
    return false;
  }

  string buffer{static_cast<string::size_type>(size.value()), false};
  auto written{k2::openssl_sign(static_cast<HashAlgorithm>(algorithm), {data.c_str(), data.size()}, key_d.value(), {buffer.buffer(), buffer.size()})};
  if (!written) [[unlikely]] {
    kphp::log::warning("Failed to sign data");
    return false;
  }

  buffer.shrink(static_cast<string::size_type>(written.value()));
  signature = std::move(buffer);

  return true;
}

int64_t f$openssl_verify(const string& data, const string& signature, const string& public_key, int64_t algorithm) noexcept {
  const auto handle{get_pkey_handle(public_key)};
  const auto key_d{handle ? handle : parse_public_key(public_key)};
  const auto key_finalizer{vk::finally([&]() noexcept {
    if (!handle && key_d) {
      k2::free_descriptor(key_d.value());
    }
  })};
  if (!key_d) [[unlikely]] {
    kphp::log::warning("Failed to verify signature");
    return -1;
  }

  auto res{k2::openssl_verify(static_cast<HashAlgorithm>(algorithm), {data.c_str(), data.size()}, key_d.value(), {signature.c_str(), signature.size()})};
  if (!res) [[unlikely]] {
    kphp::log::warning("Failed to verify signature");
    return -1;
  }

  return res.value() ? 1 : 0;
}

namespace {

constexpr std::string_view AES_128_CBC = "aes-128-cbc";
constexpr std::string_view AES_256_CBC = "aes-256-cbc";
constexpr std::string_view AES_128_GCM = "aes-128-gcm";
constexpr std::string_view AES_256_GCM = "aes-256-gcm";

enum class encryption_mode { BLOCK, STREAM, AEAD };

struct cipher_algorithm_traits {
  int64_t key_len;
  int64_t iv_len;
  int64_t block_size;
  encryption_mode mode;
  CipherAlgorithm algo;
};

constexpr auto CIPHER_ALGORITHMS = [] {
  auto arr{std::array{std::pair{AES_128_CBC, cipher_algorithm_traits{16, 16, 16, encryption_mode::BLOCK, AES128_CBC}},
                      std::pair{AES_256_CBC, cipher_algorithm_traits{32, 16, 16, encryption_mode::BLOCK, AES256_CBC}},
                      std::pair{AES_128_GCM, cipher_algorithm_traits{16, 12, 0, encryption_mode::AEAD, AES128_GCM}},
                      std::pair{AES_256_GCM, cipher_algorithm_traits{32, 12, 0, encryption_mode::AEAD, AES256_GCM}}}};
  std::ranges::sort(arr, std::less{}, &std::pair<std::string_view, cipher_algorithm_traits>::first);
  return arr;
}();

static_assert(CIPHER_ALGORITHMS.size() == CipherAlgorithm::COUNT_CIPHER);

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
      kphp::log::warning("Setting of IV length for AEAD mode failed");
      return std::nullopt;
    }
  } else {
    int64_t iv_required_len{traits.iv_len};
    if (iv.size() < iv_required_len) {
      kphp::log::warning("IV passed is only {} bytes long, cipher expects an IV of precisely {} bytes, padding with \\0", iv.size(), iv_required_len);
      iv.append(static_cast<string::size_type>(iv_required_len - iv.size()), '\0');
    } else if (iv.size() > iv_required_len) {
      kphp::log::warning("IV passed is {} bytes long which is longer than the {} expected by selected cipher, truncating", iv.size(), iv_required_len);
      iv.shrink(static_cast<string::size_type>(iv_required_len));
    }
  }

  auto key = source_key;
  int64_t key_required_len{traits.key_len};
  if (key.size() < key_required_len) {
    if (options & static_cast<int64_t>(cipher_opts::OPENSSL_DONT_ZERO_PAD_KEY)) [[unlikely]] {
      kphp::log::warning("Key length should be {} bytes long:\n", key_required_len);
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
    kphp::log::warning("Unknown cipher algorithm {}", algorithm.c_str());
    return false;
  }

  return algo->get().iv_len;
}

Optional<string> f$openssl_encrypt(const string& data, const string& algorithm, const string& key, int64_t options, const string& iv,
                                   std::optional<std::reference_wrapper<string>> tag, const string& aad, [[maybe_unused]] int64_t tag_length) noexcept {
  auto algo{parse_cipher_algorithm({algorithm.c_str(), algorithm.size()})};
  if (!algo) [[unlikely]] {
    kphp::log::warning("Unknown cipher algorithm {}", algorithm.c_str());
    return false;
  }

  const auto& traits{algo->get()};
  bool is_aead{traits.mode == encryption_mode::AEAD};
  if (is_aead && (!tag || tag_length == 0)) [[unlikely]] {
    kphp::log::warning("A tag must be provided when using AEAD mode");
    return false;
  }

  if (!is_aead) {
    if (tag) {
      kphp::log::warning("The authenticated tag cannot be provided for cipher that does not support AEAD");
    }

    if (!aad.empty()) {
      kphp::log::warning("The additional authenticated data cannot be provided for cipher that does not support AEAD");
    }
  }

  if (iv.empty()) {
    kphp::log::warning("Using an empty Initialization Vector (iv) is potentially insecure and not recommended");
  }

  auto key_iv{algorithm_pad_key_iv(traits, key, iv, options)};
  if (!key_iv) [[unlikely]] {
    return false;
  }

  BlockPadding padding{BlockPadding::PKCS7};
  if (options & static_cast<int64_t>(cipher_opts::OPENSSL_ZERO_PADDING)) {
    padding = BlockPadding::BLOCK_NO_PADDING;
  }

  auto encrypted_size{calc_encrypted_size(traits, tag_length, data.size())};
  string encrypted_data{static_cast<string::size_type>(encrypted_size.encrypted_size), false};
  string received_tag{static_cast<string::size_type>(encrypted_size.tag_size), false};
  auto res{k2::openssl_encrypt(traits.algo, padding, {(*key_iv).first.c_str(), (*key_iv).first.size()}, {(*key_iv).second.c_str(), (*key_iv).second.size()},
                               {aad.c_str(), aad.size()}, {data.c_str(), data.size()}, {encrypted_data.buffer(), encrypted_data.size()},
                               {received_tag.buffer(), received_tag.size()})};
  if (!res) [[unlikely]] {
    kphp::log::warning("Failed to encrypt data");
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
    kphp::log::warning("Unknown cipher algorithm {}", algorithm.c_str());
    return false;
  }

  const auto& traits{algo->get()};
  bool is_aead{traits.mode == encryption_mode::AEAD};
  if (is_aead && tag.empty()) [[unlikely]] {
    kphp::log::warning("A tag must be provided when using AEAD mode");
    return false;
  }

  if (!is_aead) {
    if (!tag.empty()) {
      kphp::log::warning("The authenticated tag cannot be provided for cipher that does not support AEAD");
    }

    if (!aad.empty()) {
      kphp::log::warning("The additional authenticated data cannot be provided for cipher that does not support AEAD");
    }
  }

  string input_data{data};
  if (!(options & static_cast<int64_t>(cipher_opts::OPENSSL_RAW_DATA))) {
    Optional<string> decoding_data{f$base64_decode(data, true)};
    if (!decoding_data.has_value()) [[unlikely]] {
      kphp::log::warning("Failed to base64 decode the input");
      return false;
    }

    input_data = std::move(decoding_data.val());
  }

  auto key_iv{algorithm_pad_key_iv(traits, key, iv, options)};
  if (!key_iv) [[unlikely]] {
    return false;
  }

  BlockPadding padding{BlockPadding::PKCS7};
  if (options & static_cast<int64_t>(cipher_opts::OPENSSL_ZERO_PADDING)) {
    padding = BlockPadding::BLOCK_NO_PADDING;
  }

  string decrypted_data{static_cast<string::size_type>(calc_decrypted_size(traits, input_data.size())), false};
  auto res{k2::openssl_decrypt(traits.algo, padding, {(*key_iv).first.c_str(), (*key_iv).first.size()}, {(*key_iv).second.c_str(), (*key_iv).second.size()},
                               {aad.c_str(), aad.size()}, {tag.c_str(), tag.size()}, {input_data.c_str(), input_data.size()},
                               {decrypted_data.buffer(), decrypted_data.size()})};
  if (!res) [[unlikely]] {
    kphp::log::warning("Failed to decrypt data");
    return false;
  }

  decrypted_data.shrink(static_cast<string::size_type>(*res));

  return std::move(decrypted_data);
}

Optional<string> f$openssl_pkey_get_public(const string& public_key) noexcept {
  if (const auto handle{get_pkey_handle(public_key)}; handle) {
    if (!k2::openssl_pkey_is_public(handle.value()).value_or(false)) [[unlikely]] {
      kphp::log::warning("Failed to get public key");
      return false;
    }

    return public_key;
  }

  auto key_d{parse_public_key(public_key)};
  if (!key_d) [[unlikely]] {
    kphp::log::warning("Failed to get public key");
    return false;
  }

  return make_pkey_handle(key_d.value());
}

Optional<string> f$openssl_pkey_get_private(const string& private_key, const string& passphrase) noexcept {
  if (const auto handle{get_pkey_handle(private_key)}; handle) {
    if (!k2::openssl_pkey_is_private(handle.value()).value_or(false)) [[unlikely]] {
      kphp::log::warning("Failed to get private key");
      return false;
    }

    return private_key;
  }

  auto key_d{parse_private_key(private_key, passphrase)};
  if (!key_d) [[unlikely]] {
    kphp::log::warning("Failed to get private key");
    return false;
  }

  return make_pkey_handle(key_d.value());
}

bool f$openssl_public_encrypt(const string& data, string& encrypted_data, const string& public_key) noexcept {
  const auto handle{get_pkey_handle(public_key)};
  const auto key_d{handle ? handle : parse_public_key(public_key)};
  const auto key_finalizer{vk::finally([&]() noexcept {
    if (!handle && key_d) {
      k2::free_descriptor(key_d.value());
    }
  })};
  if (!key_d) [[unlikely]] {
    kphp::log::warning("Failed to encrypt data with the given public key");
    return false;
  }

  auto size{k2::openssl_pkey_size(key_d.value())};
  if (!size) [[unlikely]] {
    kphp::log::warning("Failed to encrypt data with the given public key");
    return false;
  }

  string buffer{static_cast<string::size_type>(size.value()), false};
  auto written{k2::openssl_public_encrypt({data.c_str(), data.size()}, key_d.value(), RsaPadding::PKCS1, {buffer.buffer(), buffer.size()})};
  if (!written) [[unlikely]] {
    kphp::log::warning("Failed to encrypt data with the given public key");
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
  const auto handle{get_pkey_handle(private_key)};
  const auto key_d{handle ? handle : parse_private_key(private_key, {})};
  const auto key_finalizer{vk::finally([&]() noexcept {
    if (!handle && key_d) {
      k2::free_descriptor(key_d.value());
    }
  })};
  if (!key_d) [[unlikely]] {
    kphp::log::warning("Failed to decrypt data with the given private key");
    return false;
  }

  auto size{k2::openssl_pkey_size(key_d.value())};
  if (!size) [[unlikely]] {
    kphp::log::warning("Failed to decrypt data with the given private key");
    return false;
  }

  string buffer{static_cast<string::size_type>(size.value()), false};
  auto written{k2::openssl_private_decrypt({data.c_str(), data.size()}, key_d.value(), RsaPadding::PKCS1, {buffer.buffer(), buffer.size()})};
  if (!written) [[unlikely]] {
    kphp::log::warning("Failed to decrypt data with the given private key");
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

constexpr std::string_view DSS1 = "dss1";
constexpr std::string_view SHA1 = "sha1";
constexpr std::string_view SHA224 = "sha224";
constexpr std::string_view SHA256 = "sha256";
constexpr std::string_view SHA384 = "sha384";
constexpr std::string_view SHA512 = "sha512";
constexpr std::string_view MD5 = "md5";
constexpr std::string_view RMD160 = "rmd160";
constexpr std::string_view MD2 = "md2";
constexpr std::string_view MD4 = "md4";

constexpr size_t DSS1_HASHED_SIZE = 20;
constexpr size_t SHA1_HASHED_SIZE = 20;
constexpr size_t SHA224_HASHED_SIZE = 28;
constexpr size_t SHA256_HASHED_SIZE = 32;
constexpr size_t SHA384_HASHED_SIZE = 48;
constexpr size_t SHA512_HASHED_SIZE = 64;
constexpr size_t MD5_HASHED_SIZE = 16;
constexpr size_t RMD160_HASHED_SIZE = 20;
constexpr size_t MD2_HASHED_SIZE = 16;
constexpr size_t MD4_HASHED_SIZE = 16;

struct hash_algorithm_traits {
  int64_t hashed_size;
  HashAlgorithm algo;
};

constexpr auto HASH_ALGORITHMS = []() noexcept {
  auto arr{std::array{std::pair{DSS1, hash_algorithm_traits{DSS1_HASHED_SIZE, HashAlgorithm::DSS1}},
                      std::pair{SHA1, hash_algorithm_traits{SHA1_HASHED_SIZE, HashAlgorithm::SHA1}},
                      std::pair{SHA224, hash_algorithm_traits{SHA224_HASHED_SIZE, HashAlgorithm::SHA224}},
                      std::pair{SHA256, hash_algorithm_traits{SHA256_HASHED_SIZE, HashAlgorithm::SHA256}},
                      std::pair{SHA384, hash_algorithm_traits{SHA384_HASHED_SIZE, HashAlgorithm::SHA384}},
                      std::pair{SHA512, hash_algorithm_traits{SHA512_HASHED_SIZE, HashAlgorithm::SHA512}},
                      std::pair{MD5, hash_algorithm_traits{MD5_HASHED_SIZE, HashAlgorithm::MD5}},
                      std::pair{RMD160, hash_algorithm_traits{RMD160_HASHED_SIZE, HashAlgorithm::RMD160}},
                      std::pair{MD2, hash_algorithm_traits{MD2_HASHED_SIZE, HashAlgorithm::MD2}},
                      std::pair{MD4, hash_algorithm_traits{MD4_HASHED_SIZE, HashAlgorithm::MD4}}}};

  std::ranges::sort(arr, std::less{}, &std::pair<std::string_view, hash_algorithm_traits>::first);
  return arr;
}();

static_assert(HASH_ALGORITHMS.size() == HashAlgorithm::COUNT_HASH);

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
  auto res{k2::hash(traits.algo, {data.c_str(), data.size()}, {hashed_data.buffer(), static_cast<size_t>(traits.hashed_size)})};
  if (!res) [[unlikely]] {
    kphp::log::warning("Failed to generate hash");
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
  auto res{k2::hash_hmac(algo->algo, {data.c_str(), data.size()}, {key.c_str(), key.size()}, {hashed_data.buffer(), static_cast<size_t>(algo->hashed_size)})};
  if (!res) [[unlikely]] {
    kphp::log::warning("Failed to generate HMAC hash");
    return {};
  }

  if (!binary) {
    kphp::strings::bin2hex(hashed_data.buffer(), algo->hashed_size, hashed_data.size());
  }

  return hashed_data;
}

string f$sha1(const string& data, bool binary) noexcept {
  return hash_impl(hash_algorithm_traits{SHA1_HASHED_SIZE, HashAlgorithm::SHA1}, data, binary);
}

int64_t f$crc32(const string& s) noexcept {
  return crc32_partial_generic(static_cast<const void*>(s.c_str()), s.size(), -1) ^ -1;
}
