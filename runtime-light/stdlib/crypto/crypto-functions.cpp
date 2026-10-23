//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/crypto/crypto-functions.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <functional>
#include <optional>
#include <ranges>
#include <string_view>
#include <utility>

#include "common/crc32_generic.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/stdlib/server/url-functions.h"
#include "runtime-common/stdlib/string/string-functions.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/component/component-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/streams/read-ext.h"
#include "runtime-light/streams/stream.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"

namespace {

constexpr std::string_view CRYPTO_COMPONENT_NAME = "crypto";

} // namespace

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

kphp::coro::task<Optional<array<mixed>>> f$openssl_x509_parse(string data, bool shortnames) noexcept {
  tl::GetPemCertInfo get_perm_cert_info{.is_short = shortnames, .bytes = {.value = {data.c_str(), data.size()}}};
  tl::storer tls{get_perm_cert_info.footprint()};
  get_perm_cert_info.store(tls);

  auto expected_stream{kphp::component::stream::open(CRYPTO_COMPONENT_NAME, k2::stream_kind::component)};
  if (!expected_stream) [[unlikely]] {
    co_return false;
  }

  auto stream{*std::move(expected_stream)};
  kphp::stl::vector<std::byte, kphp::memory::script_allocator> response_bytes{};
  if (!co_await kphp::forks::id_managed(kphp::component::query(stream, tls.view(), kphp::component::read_ext::append(response_bytes)))) [[unlikely]] {
    co_return false;
  }

  tl::fetcher tlf{response_bytes};
  tl::Maybe<tl::Dictionary<tl::CertInfoItem>> cert_items{};
  kphp::log::assertion(cert_items.fetch(tlf));
  if (!cert_items.opt_value) {
    co_return false;
  }

  array<mixed> response{array_size{static_cast<int64_t>((*cert_items.opt_value).size()), false}};
  auto item_to_mixed{tl::CertInfoItem::MakeVisitor{
      [](tl::i64 val) noexcept -> mixed { return val.value; }, [](tl::string val) noexcept -> mixed { return string(val.value.data(), val.value.size()); },
      [](const tl::dictionary<tl::string>& sub_dict) noexcept -> mixed {
        array<mixed> resp{array_size{static_cast<int64_t>(sub_dict.size()), false}};
        for (auto sub_item : sub_dict) {
          auto key{string{sub_item.key.value.data(), static_cast<string::size_type>(sub_item.key.value.size())}};
          auto value{string{sub_item.value.value.data(), static_cast<string::size_type>(sub_item.value.value.size())}};
          resp[key] = std::move(value);
        }
        return resp;
      }}};

  for (auto cert_kv : std::move(*cert_items.opt_value)) {
    auto key{string{cert_kv.key.value.data(), static_cast<string::size_type>(cert_kv.key.value.size())}};
    tl::CertInfoItem val{std::move(cert_kv.value)};
    response[string{cert_kv.key.value.data(), static_cast<string::size_type>(cert_kv.key.value.size())}] = std::visit(item_to_mixed, val.data);
  }
  co_return std::move(response);
}

bool f$openssl_sign(const string& data, string& signature, const string& private_key, int64_t algorithm) noexcept {
  auto size{k2::openssl_sign(static_cast<HashAlgorithm>(algorithm), {data.c_str(), data.size()}, {private_key.c_str(), private_key.size()}, {})};
  if (!size) [[unlikely]] {
    kphp::log::warning("Failed to sign data");
    return false;
  }

  string buffer{static_cast<string::size_type>(*size), false};
  auto written{k2::openssl_sign(static_cast<HashAlgorithm>(algorithm), {data.c_str(), data.size()}, {private_key.c_str(), private_key.size()},
                                {buffer.buffer(), buffer.size()}, true)};
  if (!written) [[unlikely]] {
    kphp::log::warning("Failed to sign data");
    return false;
  }

  buffer.shrink(static_cast<string::size_type>(*written));
  signature = std::move(buffer);

  return true;
}

int64_t f$openssl_verify(const string& data, const string& signature, const string& public_key, int64_t algorithm) noexcept {
  auto res{k2::openssl_verify(static_cast<HashAlgorithm>(algorithm), {data.c_str(), data.size()}, {public_key.c_str(), public_key.size()},
                              {signature.c_str(), signature.size()})};
  if (!res) [[unlikely]] {
    kphp::log::warning("Failed to verify signature");
    return -1;
  }

  return *res ? 1 : 0;
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
  auto size{k2::openssl_pkey_get_public({public_key.c_str(), public_key.size()}, {})};
  if (!size) [[unlikely]] {
    kphp::log::warning("Failed to get public key");
    return false;
  }

  string buffer{static_cast<string::size_type>(*size), false};
  if (!k2::openssl_pkey_get_public({public_key.c_str(), public_key.size()}, {buffer.buffer(), buffer.size()}, true)) [[unlikely]] {
    kphp::log::warning("Failed to get public key");
    return false;
  }

  return buffer;
}

Optional<string> f$openssl_pkey_get_private(const string& private_key, const string& passphrase) noexcept {
  auto size{k2::openssl_pkey_get_private({private_key.c_str(), private_key.size()}, {passphrase.c_str(), passphrase.size()}, {})};
  if (!size) [[unlikely]] {
    kphp::log::warning("Failed to get private key");
    return false;
  }

  string buffer{static_cast<string::size_type>(*size), false};
  if (!k2::openssl_pkey_get_private({private_key.c_str(), private_key.size()}, {passphrase.c_str(), passphrase.size()}, {buffer.buffer(), buffer.size()}, true))
      [[unlikely]] {
    kphp::log::warning("Failed to get private key");
    return false;
  }

  return buffer;
}

bool f$openssl_public_encrypt(const string& data, string& encrypted_data, const string& public_key) noexcept {
  auto size{k2::openssl_public_encrypt({data.c_str(), data.size()}, {}, {public_key.c_str(), public_key.size()}, RsaPadding::PKCS1)};
  if (!size) [[unlikely]] {
    kphp::log::warning("Failed to encrypt data with the given public key");
    return false;
  }

  string buffer{static_cast<string::size_type>(*size), false};
  if (!k2::openssl_public_encrypt({data.c_str(), data.size()}, {buffer.buffer(), buffer.size()}, {public_key.c_str(), public_key.size()}, RsaPadding::PKCS1,
                                  true)) [[unlikely]] {
    kphp::log::warning("Failed to encrypt data with the given public key");
    return false;
  }

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
  string buffer{static_cast<string::size_type>(data.size()), false};
  auto res{
      k2::openssl_private_decrypt({data.c_str(), data.size()}, {buffer.buffer(), buffer.size()}, {private_key.c_str(), private_key.size()}, RsaPadding::PKCS1)};
  if (!res) [[unlikely]] {
    kphp::log::warning("Failed to decrypt data with the given private key");
    return false;
  }

  buffer.shrink(static_cast<string::size_type>(*res));
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

constexpr size_t DSS1_HASHED_SIZE = 20;
constexpr size_t SHA1_HASHED_SIZE = 20;
constexpr size_t SHA224_HASHED_SIZE = 28;
constexpr size_t SHA256_HASHED_SIZE = 32;
constexpr size_t SHA384_HASHED_SIZE = 48;
constexpr size_t SHA512_HASHED_SIZE = 64;
constexpr size_t MD5_HASHED_SIZE = 16;
constexpr size_t RMD160_HASHED_SIZE = 20;

struct hash_algorithm_traits {
  int64_t hashed_size;
  HashAlgorithm algo;
};

constexpr auto HASH_ALGORITHMS = []() noexcept {
  auto arr{std::array{
      std::pair{DSS1, hash_algorithm_traits{DSS1_HASHED_SIZE, HashAlgorithm::DSS1}},
      std::pair{SHA1, hash_algorithm_traits{SHA1_HASHED_SIZE, HashAlgorithm::SHA1}},
      std::pair{SHA224, hash_algorithm_traits{SHA224_HASHED_SIZE, HashAlgorithm::SHA224}},
      std::pair{SHA256, hash_algorithm_traits{SHA256_HASHED_SIZE, HashAlgorithm::SHA256}},
      std::pair{SHA384, hash_algorithm_traits{SHA384_HASHED_SIZE, HashAlgorithm::SHA384}},
      std::pair{SHA512, hash_algorithm_traits{SHA512_HASHED_SIZE, HashAlgorithm::SHA512}},
      std::pair{MD5, hash_algorithm_traits{MD5_HASHED_SIZE, HashAlgorithm::MD5}},
      std::pair{RMD160, hash_algorithm_traits{RMD160_HASHED_SIZE, HashAlgorithm::RMD160}},
  }};

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
