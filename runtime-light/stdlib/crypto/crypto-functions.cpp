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
#include "common/tl/constants/common.h"
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

kphp::coro::task<Optional<string>> f$openssl_random_pseudo_bytes(int64_t length) noexcept {
  if (length <= 0 || length > string::max_size()) [[unlikely]] {
    co_return false;
  }

  tl::GetCryptosecurePseudorandomBytes get_pseudorandom_bytes{.size = {.value = static_cast<int32_t>(length)}};
  tl::storer tls{get_pseudorandom_bytes.footprint()};
  get_pseudorandom_bytes.store(tls);

  auto expected_stream{kphp::component::stream::open(CRYPTO_COMPONENT_NAME, k2::stream_kind::component)};
  if (!expected_stream) [[unlikely]] {
    co_return false;
  }

  auto stream{*std::move(expected_stream)};
  kphp::stl::vector<std::byte, kphp::memory::script_allocator> response{};
  if (!co_await kphp::forks::id_managed(kphp::component::query(stream, tls.view(), kphp::component::read_ext::append(response)))) [[unlikely]] {
    co_return false;
  }

  tl::fetcher tlf{response};
  tl::magic magic{};
  tl::string str{};
  kphp::log::assertion(magic.fetch(tlf) && magic.expect(TL_MAYBE_TRUE) && str.fetch(tlf));
  co_return string{str.value.data(), static_cast<string::size_type>(str.value.size())};
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

// FIXME it isn't safe to accept signature by reference
kphp::coro::task<bool> f$openssl_sign(string data, string& signature, string private_key, int64_t algo) noexcept {
  tl::DigestSign digest_sign{.data = {.value = {data.c_str(), data.size()}},
                             .private_key = {.value = {private_key.c_str(), private_key.size()}},
                             .algorithm = static_cast<tl::HashAlgorithm>(algo)};
  tl::storer tls{digest_sign.footprint()};
  digest_sign.store(tls);

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
  tl::magic magic{};
  tl::string str{};
  kphp::log::assertion(magic.fetch(tlf) && magic.expect(TL_MAYBE_TRUE) && str.fetch(tlf));
  signature = string{str.value.data(), static_cast<string::size_type>(str.value.size())};
  co_return true;
}

kphp::coro::task<int64_t> f$openssl_verify(string data, string signature, string pub_key, int64_t algo) noexcept {
  tl::DigestVerify digest_verify{.data = {.value = {data.c_str(), data.size()}},
                                 .public_key = {.value = {pub_key.c_str(), pub_key.size()}},
                                 .algorithm = static_cast<tl::HashAlgorithm>(algo),
                                 .signature = {.value = {signature.c_str(), signature.size()}}};
  tl::storer tls{digest_verify.footprint()};
  digest_verify.store(tls);

  auto expected_stream{kphp::component::stream::open(CRYPTO_COMPONENT_NAME, k2::stream_kind::component)};
  if (!expected_stream) [[unlikely]] {
    co_return 0;
  }

  auto stream{*std::move(expected_stream)};
  std::array<std::byte, tl::magic{}.footprint()> response{};
  if (!co_await kphp::forks::id_managed(kphp::component::query(stream, tls.view(), response))) [[unlikely]] {
    co_return 0;
  }

  tl::fetcher tlf{response};
  // For now returns only 1 or 0, -1 is never returned
  // Because it's currently impossible to distiguish error from negative verification
  if (tl::magic magic{}; !magic.fetch(tlf) || !magic.expect(TL_BOOL_TRUE)) {
    co_return 0;
  }
  co_return 1;
}

namespace {

constexpr std::string_view AES_128_CBC = "aes-128-cbc";
constexpr std::string_view AES_256_CBC = "aes-256-cbc";
constexpr std::string_view AES_128_GCM = "aes-128-gcm";
constexpr std::string_view AES_256_GCM = "aes-256-gcm";

std::optional<tl::CipherAlgorithm> parse_cipher_algorithm(const string& method) noexcept {
  using namespace std::string_view_literals;
  std::string_view method_sv{method.c_str(), method.size()};

  const auto ichar_equals = [](char a, char b) { return std::tolower(a) == std::tolower(b); };

  if (std::ranges::equal(method_sv, AES_128_CBC, ichar_equals)) {
    return tl::CipherAlgorithm::AES128_CBC;
  } else if (std::ranges::equal(method_sv, AES_256_CBC, ichar_equals)) {
    return tl::CipherAlgorithm::AES256_CBC;
  } else if (std::ranges::equal(method_sv, AES_128_GCM, ichar_equals)) {
    return tl::CipherAlgorithm::AES128_GCM;
  } else if (std::ranges::equal(method_sv, AES_256_GCM, ichar_equals)) {
    return tl::CipherAlgorithm::AES256_GCM;
  }
  return std::nullopt;
}

constexpr auto CIPHER_ALGORITHMS = [] {
  auto arr{std::array{std::pair{AES_128_CBC, AES128_CBC}, std::pair{AES_256_CBC, AES256_CBC}, std::pair{AES_128_GCM, AES128_GCM},
                      std::pair{AES_256_GCM, AES256_GCM}}};
  std::ranges::sort(arr, std::less{}, &std::pair<std::string_view, CipherAlgorithm>::first);
  return arr;
}();

static_assert(CIPHER_ALGORITHMS.size() == CipherAlgorithm::COUNT_CIPHER);

std::optional<CipherAlgorithm> parse_cipher_algorithm_test(std::string_view algorithm) noexcept {
  auto lowered{std::ranges::transform_view{algorithm, [](char c) noexcept -> char {
                                             if (c >= 'A' && c <= 'Z') {
                                               return c + ('a' - 'A');
                                             }
                                             return c;
                                           }}};

  auto rng{std::ranges::equal_range(CIPHER_ALGORITHMS, lowered, std::ranges::lexicographical_compare, &std::pair<std::string_view, CipherAlgorithm>::first)};
  if (!rng.empty()) [[likely]] {
    return rng.front().second;
  }

  return std::nullopt;
}

enum class encryption_mode { BLOCK, STREAM, AEAD };

struct cipher_algorithm_traits {
  int64_t key_len;
  int64_t iv_len;
  int64_t block_size;
  encryption_mode mode;
};

constexpr const cipher_algorithm_traits& get_cipher_algorithm_traits(CipherAlgorithm algorithm) noexcept {
  static constexpr std::array traits{
      cipher_algorithm_traits{16, 12, 0, encryption_mode::AEAD},   // AES128_GCM
      cipher_algorithm_traits{32, 12, 0, encryption_mode::AEAD},   // AES256_GCM
      cipher_algorithm_traits{16, 16, 16, encryption_mode::BLOCK}, // AES128_CBC
      cipher_algorithm_traits{32, 16, 16, encryption_mode::BLOCK}  // AES256_CBC
  };

  static_assert(traits.size() == CipherAlgorithm::COUNT_CIPHER);

  return traits[algorithm];
}

bool is_gcm_algorithm(tl::CipherAlgorithm algorithm) {
  return algorithm == tl::AES128_GCM || algorithm == tl::AES256_GCM;
}

bool is_aead_algorithm(tl::CipherAlgorithm algorithm) {
  return is_gcm_algorithm(algorithm);
}

auto calc_encrypted_size(const cipher_algorithm_traits& algorithm_traits, int64_t tag_size, int64_t data_len) noexcept {
  struct encrypted_data_size_info {
    int64_t encrypted_size;
    int64_t tag_size;
  };

  switch (algorithm_traits.mode) {
  case encryption_mode::BLOCK:
    return encrypted_data_size_info{data_len + algorithm_traits.block_size, 0};
  case encryption_mode::STREAM:
    return encrypted_data_size_info{data_len, 0};
  case encryption_mode::AEAD:
    return encrypted_data_size_info{data_len, tag_size};
  }
}

constexpr size_t GCM_NONCE_LEN = 12;
constexpr size_t AES_BLOCK_LEN = 16;
constexpr size_t AES_128_KEY_LEN = 16;
constexpr size_t AES_256_KEY_LEN = 32;

int64_t algorithm_iv_len([[maybe_unused]] tl::CipherAlgorithm algorithm) noexcept {
  if (is_gcm_algorithm(algorithm)) {
    return GCM_NONCE_LEN;
  }
  return AES_BLOCK_LEN;
}

int64_t algorithm_key_len(tl::CipherAlgorithm algorithm) noexcept {
  switch (algorithm) {
  case tl::CipherAlgorithm::AES128_GCM:
  case tl::CipherAlgorithm::AES128_CBC: {
    return AES_128_KEY_LEN;
  }
  case tl::CipherAlgorithm::AES256_GCM:
  case tl::CipherAlgorithm::AES256_CBC: {
    return AES_256_KEY_LEN;
  }
  default: {
    kphp::log::warning("unexpected cipher algorithm");
    return 0;
  }
  }
}

enum class cipher_opts : int64_t { OPENSSL_RAW_DATA = 1, OPENSSL_ZERO_PADDING = 2, OPENSSL_DONT_ZERO_PAD_KEY = 4 };

Optional<std::pair<string, string>> algorithm_pad_key_iv(tl::CipherAlgorithm algorithm, const string& source_key, const string& source_iv,
                                                         int64_t options) noexcept {
  const size_t iv_required_len = algorithm_iv_len(algorithm);
  const size_t key_required_len = algorithm_key_len(algorithm);
  auto iv = source_iv;
  if (is_aead_algorithm(algorithm)) {
    if (source_iv.empty()) {
      kphp::log::warning("Setting of IV length for AEAD mode failed");
      return false;
    }
  } else {
    if (iv.size() < iv_required_len) {
      kphp::log::warning("IV passed is only {} bytes long, cipher expects an IV of precisely {} bytes, padding with \\0", iv.size(), iv_required_len);
      iv.append(static_cast<string::size_type>(iv_required_len - iv.size()), '\0');
    } else if (iv.size() > iv_required_len) {
      kphp::log::warning("IV passed is {} bytes long which is longer than the {} expected by selected cipher, truncating", iv.size(), iv_required_len);
      iv.shrink(static_cast<string::size_type>(iv_required_len));
    }
  }

  auto key = source_key;
  if (key.size() < key_required_len) {
    if (options & static_cast<int64_t>(cipher_opts::OPENSSL_DONT_ZERO_PAD_KEY)) {
      kphp::log::warning("Key length should be {} bytes long:\n", key_required_len);
      return false;
    }
    kphp::log::warning("passphrase passed is only {} bytes long, cipher expects an passphrase of precisely {} bytes, padding with \\0", key.size(),
                       key_required_len);
    key.append(key_required_len - key.size(), '\0');
  } else if (key.size() > key_required_len) {
    kphp::log::warning("passphrase passed is {} bytes long which is longer than the {} expected by selected cipher, truncating", key.size(), key_required_len);
    key.shrink(static_cast<string::size_type>(key_required_len));
  }

  return std::make_pair(key, iv);
}

std::optional<std::pair<string, string>> algorithm_pad_key_iv_test(const cipher_algorithm_traits& algorithm_traits, const string& source_key,
                                                                   const string& source_iv, int64_t options) noexcept {
  auto iv = source_iv;
  if (algorithm_traits.mode == encryption_mode::AEAD) {
    if (source_iv.empty()) [[unlikely]] {
      kphp::log::warning("Setting of IV length for AEAD mode failed");
      return std::nullopt;
    }
  } else {
    int64_t iv_required_len{algorithm_traits.iv_len};
    if (iv.size() < iv_required_len) {
      kphp::log::warning("IV passed is only {} bytes long, cipher expects an IV of precisely {} bytes, padding with \\0", iv.size(), iv_required_len);
      iv.append(static_cast<string::size_type>(iv_required_len - iv.size()), '\0');
    } else if (iv.size() > iv_required_len) {
      kphp::log::warning("IV passed is {} bytes long which is longer than the {} expected by selected cipher, truncating", iv.size(), iv_required_len);
      iv.shrink(static_cast<string::size_type>(iv_required_len));
    }
  }

  auto key = source_key;
  int64_t key_required_len{algorithm_traits.key_len};
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
  for (auto [algo, _] : CIPHER_ALGORITHMS) {
    res.push_back(string{algo.data(), static_cast<string::size_type>(algo.size())});
  }

  return res;
}

Optional<int64_t> f$openssl_cipher_iv_length(const string& algorithm) noexcept {
  auto algo{parse_cipher_algorithm_test({algorithm.c_str(), algorithm.size()})};
  if (!algo) {
    kphp::log::warning("Unknown cipher algorithm {}", algorithm.c_str());
    return false;
  }

  return get_cipher_algorithm_traits(*algo).iv_len;
}

Optional<string> f$openssl_encrypt(const string& data, const string& algorithm, const string& source_key, int64_t options, const string& source_iv,
                                   std::optional<std::reference_wrapper<string>> tag, const string& aad, [[maybe_unused]] int64_t tag_length) noexcept {
  auto algo{parse_cipher_algorithm_test({algorithm.c_str(), algorithm.size()})};
  if (!algo) [[unlikely]] {
    kphp::log::warning("Unknown cipher algorithm {}", algorithm.c_str());
    return false;
  }

  const auto& algorithm_traits{get_cipher_algorithm_traits(*algo)};

  bool is_aead{algorithm_traits.mode == encryption_mode::AEAD};
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

  if (source_iv.empty()) {
    kphp::log::warning("Using an empty Initialization Vector (iv) is potentially insecure and not recommended");
  }

  auto key_iv{algorithm_pad_key_iv_test(algorithm_traits, source_key, source_iv, options)};
  if (!key_iv) [[unlikely]] {
    return false;
  }

  BlockPadding padding{PKCS7};
  if (options & static_cast<int64_t>(cipher_opts::OPENSSL_ZERO_PADDING)) {
    padding = NO_PADDING;
  }

  auto encrypted_size{calc_encrypted_size(algorithm_traits, tag_length, data.size())};
  string encrypted_data{static_cast<string::size_type>(encrypted_size.encrypted_size), false};
  string received_tag{static_cast<string::size_type>(encrypted_size.tag_size), false};
  auto res{k2::openssl_encrypt(*algo, padding, {(*key_iv).first.c_str(), (*key_iv).first.size()}, {(*key_iv).second.c_str(), (*key_iv).second.size()},
                               {aad.c_str(), aad.size()}, {data.c_str(), data.size()}, {encrypted_data.buffer(), encrypted_data.size()},
                               {received_tag.buffer(), received_tag.size()})};
  if (!res) [[unlikely]] {
    return false;
  }

  encrypted_data.shrink(static_cast<string::size_type>(*res));

  if (tag) {
    (*tag).get() = std::move(received_tag);
  }

  return (options & static_cast<int64_t>(cipher_opts::OPENSSL_RAW_DATA)) ? std::move(encrypted_data) : f$base64_encode(encrypted_data);
}

kphp::coro::task<Optional<string>> f$openssl_decrypt(string data, string method, string source_key, int64_t options, string source_iv, string tag,
                                                     string aad) noexcept {
  if (!(options & static_cast<int64_t>(cipher_opts::OPENSSL_RAW_DATA))) {
    Optional<string> decoding_data{f$base64_decode(data, true)};
    if (!decoding_data.has_value()) {
      kphp::log::warning("Failed to base64 decode the input");
      co_return false;
    }
    data = std::move(decoding_data.val());
  }

  auto algorithm{parse_cipher_algorithm(method)};
  if (!algorithm.has_value()) {
    kphp::log::warning("Unknown cipher algorithm {}", method.c_str());
    co_return false;
  }

  bool aead{is_aead_algorithm(*algorithm)};
  if (!tag.empty() && !aead) {
    kphp::log::warning("The authenticated tag cannot be provided for cipher that does not support AEAD");
  }
  if (!aad.empty() && !aead) {
    kphp::log::warning("The additional authenticated data cannot be provided for cipher that does not support AEAD");
  }

  auto key_iv{algorithm_pad_key_iv(*algorithm, source_key, source_iv, options)};
  if (!key_iv.has_value()) {
    co_return false;
  }

  tl::BlockPadding padding{tl::BlockPadding::PKCS7};
  if (options & static_cast<int64_t>(cipher_opts::OPENSSL_ZERO_PADDING)) {
    padding = tl::BlockPadding::NO_PADDING;
  }
  tl::Decrypt decrypt{.algorithm = *algorithm,
                      .padding = padding,
                      .passphrase = {.value = {key_iv.val().first.c_str(), key_iv.val().first.size()}},
                      .iv = {.value = {key_iv.val().second.c_str(), key_iv.val().second.size()}},
                      .tag = {.value = {tag.c_str(), tag.size()}},
                      .aad = {.value = {aad.c_str(), aad.size()}},
                      .data = {.value = {data.c_str(), data.size()}}};
  tl::storer tls{decrypt.footprint()};
  decrypt.store(tls);

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
  tl::Maybe<tl::string> response{};
  kphp::log::assertion(response.fetch(tlf));
  if (!response.opt_value) {
    co_return false;
  }

  co_return string{(*response.opt_value).value.data(), static_cast<string::size_type>((*response.opt_value).value.size())};
}

kphp::coro::task<Optional<string>> f$openssl_pkey_get_public(string key) noexcept {
  tl::GetPublicKey get_public_key{.key = {.value = {key.c_str(), key.size()}}};
  tl::storer tls{get_public_key.footprint()};
  get_public_key.store(tls);

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
  tl::Maybe<tl::string> response;
  kphp::log::assertion(response.fetch(tlf));
  if (!response.opt_value) {
    co_return false;
  }

  co_return string{(*response.opt_value).value.data(), static_cast<string::size_type>((*response.opt_value).value.size())};
}

kphp::coro::task<Optional<string>> f$openssl_pkey_get_private(string key, string passphrase) noexcept {
  tl::GetPrivateKey get_private_key{
      .key = {.value = {key.c_str(), key.size()}},
      .passphrase = {.value = {passphrase.c_str(), passphrase.size()}},
  };
  tl::storer tls{get_private_key.footprint()};
  get_private_key.store(tls);

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
  tl::Maybe<tl::string> response;
  kphp::log::assertion(response.fetch(tlf));
  if (!response.opt_value) {
    co_return false;
  }

  co_return string{(*response.opt_value).value.data(), static_cast<string::size_type>((*response.opt_value).value.size())};
}

kphp::coro::task<bool> f$openssl_public_encrypt(string data, string& encrypted_data, string public_key) noexcept {
  tl::PublicEncrypt public_encrypt{
      .key = {.value = {public_key.c_str(), public_key.size()}},
      .data = {.value = {data.c_str(), data.size()}},
  };
  tl::storer tls{public_encrypt.footprint()};
  public_encrypt.store(tls);

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
  tl::Maybe<tl::string> response{};
  kphp::log::assertion(response.fetch(tlf));
  if (!response.opt_value) {
    co_return false;
  }

  encrypted_data = {(*response.opt_value).value.data(), static_cast<string::size_type>((*response.opt_value).value.size())};
  co_return true;
}

kphp::coro::task<bool> f$openssl_public_encrypt(string data, mixed& result, string key) noexcept {
  string result_string;
  if (co_await f$openssl_public_encrypt(data, result_string, key)) {
    result = std::move(result_string);
    co_return true;
  }
  result = mixed{};
  co_return false;
}

kphp::coro::task<bool> f$openssl_private_decrypt(string data, string& decrypted_data, string private_key) noexcept {
  tl::PrivateDecrypt private_decrypt{
      .key = {.value = {private_key.c_str(), private_key.size()}},
      .data = {.value = {data.c_str(), data.size()}},
  };
  tl::storer tls{private_decrypt.footprint()};
  private_decrypt.store(tls);

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
  tl::Maybe<tl::string> response{};
  kphp::log::assertion(response.fetch(tlf));
  if (!response.opt_value) {
    co_return false;
  }

  decrypted_data = {(*response.opt_value).value.data(), static_cast<string::size_type>((*response.opt_value).value.size())};
  co_return true;
}

kphp::coro::task<bool> f$openssl_private_decrypt(string data, mixed& result, string key) noexcept {
  string result_string;
  if (co_await f$openssl_private_decrypt(data, result_string, key)) {
    result = std::move(result_string);
    co_return true;
  }
  result = mixed{};
  co_return false;
}

namespace {

constexpr std::string_view DSS1 = "dss1";
constexpr std::string_view SHA1 = "sha1";
constexpr std::string_view SHA224 = "sha224";
constexpr std::string_view SHA256 = "sha256";
constexpr std::string_view SHA384 = "sha384";
constexpr std::string_view SHA512 = "sha512";
constexpr std::string_view RMD160 = "rmd160";
constexpr std::string_view MD2 = "md2";
constexpr std::string_view MD4 = "md4";
constexpr std::string_view MD5 = "md5";

constexpr auto HASH_ALGORITHMS = []() noexcept {
  auto arr{std::array{std::pair{DSS1, HashAlgorithm::DSS1}, std::pair{SHA1, HashAlgorithm::SHA1}, std::pair{SHA224, HashAlgorithm::SHA224},
                      std::pair{SHA256, HashAlgorithm::SHA256}, std::pair{SHA384, HashAlgorithm::SHA384}, std::pair{SHA512, HashAlgorithm::SHA512},
                      std::pair{RMD160, HashAlgorithm::RMD160}, std::pair{MD2, HashAlgorithm::MD2}, std::pair{MD4, HashAlgorithm::MD4},
                      std::pair{MD5, HashAlgorithm::MD5}}};

  std::ranges::sort(arr, std::less{}, &std::pair<std::string_view, HashAlgorithm>::first);
  return arr;
}();

static_assert(HASH_ALGORITHMS.size() == HashAlgorithm::COUNT_HASH);

std::optional<HashAlgorithm> parse_hash_algorithm(std::string_view algorithm) noexcept {
  auto lowered{std::ranges::transform_view{algorithm, [](char c) noexcept -> char {
                                             if (c >= 'A' && c <= 'Z') {
                                               return c + ('a' - 'A');
                                             }
                                             return c;
                                           }}};

  auto rng{std::ranges::equal_range(HASH_ALGORITHMS, lowered, std::ranges::lexicographical_compare, &std::pair<std::string_view, HashAlgorithm>::first)};
  if (!rng.empty()) [[likely]] {
    return rng.front().second;
  }

  return std::nullopt;
}

auto calc_hashed_size(HashAlgorithm algorithm) noexcept -> size_t {
  static constexpr std::array sizes{
      20, // DSS1
      20, // SHA1
      28, // SHA224
      32, // SHA256
      48, // SHA384
      64, // SHA512
      20, // RMD160
      16, // MD2
      16, // MD4
      16  //MD5
  };

  static_assert(sizes.size() == HashAlgorithm::COUNT_HASH);

  return sizes[algorithm];
}

string hash_impl(HashAlgorithm algorithm, const string& data, bool binary) noexcept {
  string hashed_data{static_cast<string::size_type>(calc_hashed_size(algorithm)), false};
  auto res{k2::hash(algorithm, {data.c_str(), data.size()}, {hashed_data.buffer(), hashed_data.size()})};
  if (!res) [[unlikely]] {
    return string{false};
  }

  return binary ? std::move(hashed_data) : kphp::strings::bin2hex({hashed_data.c_str(), hashed_data.size()});
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

  string hashed_data{static_cast<string::size_type>(calc_hashed_size(*algo)), false};
  auto res{k2::hash_hmac(*algo, {data.c_str(), data.size()}, {key.c_str(), key.size()}, {hashed_data.buffer(), hashed_data.size()})};
  if (!res) [[unlikely]] {
    return string{false};
  }

  return binary ? std::move(hashed_data) : kphp::strings::bin2hex({hashed_data.c_str(), hashed_data.size()});
}

string f$sha1(const string& data, bool binary) noexcept {
  return hash_impl(HashAlgorithm::SHA1, data, binary);
}

int64_t f$crc32(const string& s) noexcept {
  return crc32_partial_generic(static_cast<const void*>(s.c_str()), s.size(), -1) ^ -1;
}
