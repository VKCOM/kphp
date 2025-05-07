//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/crypto/crypto-functions.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <optional>
#include <ranges>
#include <string_view>

#include "common/crc32_generic.h"
#include "common/tl/constants/common.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/server/url-functions.h"
#include "runtime-common/stdlib/string/string-functions.h"
#include "runtime-light/stdlib/component/component-api.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"
#include "runtime-light/utils/logs.h"

namespace {

constexpr std::string_view CRYPTO_COMPONENT_NAME = "crypto";

} // namespace

kphp::coro::task<Optional<string>> f$openssl_random_pseudo_bytes(int64_t length) noexcept {
  if (length <= 0 || length > string::max_size()) {
    co_return false;
  }

  // TODO think about performance when transferring data

  tl::TLBuffer tlb;
  tl::GetCryptosecurePseudorandomBytes{.size = {.value = static_cast<int32_t>(length)}}.store(tlb);

  auto query{f$component_client_send_request(string{CRYPTO_COMPONENT_NAME.data(), static_cast<string::size_type>(CRYPTO_COMPONENT_NAME.size())},
                                             string{tlb.data(), static_cast<string::size_type>(tlb.size())})};
  string response{co_await f$component_client_fetch_response(co_await query)};

  tlb.clean();
  tlb.store_bytes({response.c_str(), static_cast<size_t>(response.size())});

  // Maybe better to do this in some structure, but there's not much work to do with TL here
  if (tl::details::magic magic{}; !magic.fetch(tlb) || !magic.expect(TL_MAYBE_TRUE)) {
    co_return false;
  }

  tl::string str{};
  kphp::log::assertion(str.fetch(tlb));
  co_return string{str.value.data(), static_cast<string::size_type>(str.value.size())};
}

kphp::coro::task<Optional<array<mixed>>> f$openssl_x509_parse(string data, bool shortnames) noexcept {

  tl::TLBuffer tlb;
  tl::GetPemCertInfo{.is_short = shortnames, .bytes = {.value = {data.c_str(), data.size()}}}.store(tlb);

  auto query{f$component_client_send_request(string{CRYPTO_COMPONENT_NAME.data(), static_cast<string::size_type>(CRYPTO_COMPONENT_NAME.size())},
                                             string{tlb.data(), static_cast<string::size_type>(tlb.size())})};
  string resp{co_await f$component_client_fetch_response(co_await query)};

  tlb.clean();
  tlb.store_bytes({resp.c_str(), static_cast<size_t>(resp.size())});

  tl::Maybe<tl::Dictionary<tl::CertInfoItem>> cert_items;
  if (!cert_items.fetch(tlb)) {
    co_return false;
  }
  if (!cert_items.opt_value.has_value()) {
    co_return false;
  }

  array<mixed> response;
  response.reserve(cert_items.opt_value->size(), false);

  auto item_to_mixed{tl::CertInfoItem::MakeVisitor{[](tl::i64 val) -> mixed { return val.value; },
                                                   [](tl::string val) -> mixed { return string(val.value.data(), val.value.size()); },
                                                   [](const tl::dictionary<tl::string>& sub_dict) -> mixed {
                                                     array<mixed> resp;
                                                     resp.reserve(sub_dict.size(), false);
                                                     for (auto sub_item : sub_dict) {
                                                       auto key = string(sub_item.key.value.data(), sub_item.key.value.size());
                                                       auto value = string(sub_item.value.value.data(), sub_item.value.value.size());
                                                       resp[key] = value;
                                                     }

                                                     return resp;
                                                   }}};

  for (auto cert_kv : std::move(*cert_items.opt_value)) {
    auto key{string{cert_kv.key.value.data(), static_cast<string::size_type>(cert_kv.key.value.size())}};
    tl::CertInfoItem val{std::move(cert_kv.value)};
    response[string{cert_kv.key.value.data(), static_cast<string::size_type>(cert_kv.key.value.size())}] = std::visit(item_to_mixed, val.data);
  }

  co_return response;
}

// FIXME it isn't safe to accept signature by reference
kphp::coro::task<bool> f$openssl_sign(string data, string& signature, string private_key, int64_t algo) noexcept {

  tl::TLBuffer tlb;
  tl::DigestSign{.data = {.value = {data.c_str(), data.size()}},
                 .private_key = {.value = {private_key.c_str(), private_key.size()}},
                 .algorithm = static_cast<tl::HashAlgorithm>(algo)}
      .store(tlb);

  auto query{f$component_client_send_request(string{CRYPTO_COMPONENT_NAME.data(), static_cast<string::size_type>(CRYPTO_COMPONENT_NAME.size())},
                                             string{tlb.data(), static_cast<string::size_type>(tlb.size())})};
  string response{co_await f$component_client_fetch_response(co_await query)};

  tlb.clean();
  tlb.store_bytes({response.c_str(), static_cast<size_t>(response.size())});

  if (tl::details::magic magic{}; !magic.fetch(tlb) || !magic.expect(TL_MAYBE_TRUE)) {
    co_return false;
  }

  tl::string str{};
  kphp::log::assertion(str.fetch(tlb));
  signature = string{str.value.data(), static_cast<string::size_type>(str.value.size())};
  co_return true;
}

kphp::coro::task<int64_t> f$openssl_verify(string data, string signature, string pub_key, int64_t algo) noexcept {
  tl::TLBuffer tlb;
  tl::DigestVerify{.data = {.value = {data.c_str(), data.size()}},
                   .public_key = {.value = {pub_key.c_str(), pub_key.size()}},
                   .algorithm = static_cast<tl::HashAlgorithm>(algo),
                   .signature = {.value = {signature.c_str(), signature.size()}}}
      .store(tlb);

  auto query{f$component_client_send_request(string{CRYPTO_COMPONENT_NAME.data(), static_cast<string::size_type>(CRYPTO_COMPONENT_NAME.size())},
                                             string{tlb.data(), static_cast<string::size_type>(tlb.size())})};
  string response{co_await f$component_client_fetch_response(co_await query)};

  tlb.clean();
  tlb.store_bytes({response.c_str(), static_cast<size_t>(response.size())});

  // For now returns only 1 or 0, -1 is never returned
  // Because it's currently impossible to distiguish error from negative verification
  if (tl::details::magic magic{}; !magic.fetch(tlb) || !magic.expect(TL_BOOL_TRUE)) {
    co_return 0;
  }
  co_return 1;
}

namespace {

constexpr std::string_view AES_128_CBC = "aes-128-cbc";
constexpr std::string_view AES_256_CBC = "aes-256-cbc";

std::optional<tl::CipherAlgorithm> parse_cipher_algorithm(const string& method) noexcept {
  using namespace std::string_view_literals;
  std::string_view method_sv{method.c_str(), method.size()};

  const auto ichar_equals = [](char a, char b) { return std::tolower(a) == std::tolower(b); };

  if (std::ranges::equal(method_sv, AES_128_CBC, ichar_equals)) {
    return tl::CipherAlgorithm::AES128;
  } else if (std::ranges::equal(method_sv, AES_256_CBC, ichar_equals)) {
    return tl::CipherAlgorithm::AES256;
  }
  return {};
}

constexpr size_t AES_BLOCK_LEN = 16;
constexpr size_t AES_128_KEY_LEN = 16;
constexpr size_t AES_256_KEY_LEN = 32;

int64_t algorithm_iv_len([[maybe_unused]] tl::CipherAlgorithm algorithm) noexcept {
  /* since only aes-128/256-cbc supported for now */
  return AES_BLOCK_LEN;
}

int64_t algorithm_key_len(tl::CipherAlgorithm algorithm) noexcept {
  switch (algorithm) {
  case tl::CipherAlgorithm::AES128: {
    return AES_128_KEY_LEN;
  }
  case tl::CipherAlgorithm::AES256: {
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
  if (iv.size() < iv_required_len) {
    kphp::log::warning("IV passed is only {} bytes long, cipher expects an IV of precisely {} bytes, padding with \\0", iv.size(), iv_required_len);
    iv.append(static_cast<string::size_type>(iv_required_len - iv.size()), '\0');
  } else if (iv.size() > iv_required_len) {
    kphp::log::warning("IV passed is {} bytes long which is longer than the {} expected by selected cipher, truncating", iv.size(), iv_required_len);
    iv.shrink(static_cast<string::size_type>(iv_required_len));
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

  if (options & static_cast<int64_t>(cipher_opts::OPENSSL_ZERO_PADDING)) {
    kphp::log::warning("OPENSSL_ZERO_PADDING option is not supported for now\n");
    return false;
  }
  return std::make_pair(key, iv);
}

} // namespace

array<string> f$openssl_get_cipher_methods([[maybe_unused]] bool aliases) noexcept {
  array<string> return_value{
      {std::make_pair(0, string{AES_128_CBC.data(), AES_128_CBC.size()}), std::make_pair(1, string{AES_256_CBC.data(), AES_256_CBC.size()})}};
  return return_value;
}

Optional<int64_t> f$openssl_cipher_iv_length(const string& method) noexcept {
  auto algorithm = parse_cipher_algorithm(method);
  if (!algorithm.has_value()) {
    kphp::log::warning("Unknown cipher algorithm");
    return false;
  }
  return algorithm_iv_len(*algorithm);
}

kphp::coro::task<Optional<string>> f$openssl_encrypt(string data, string method, string source_key, int64_t options, string source_iv, string& tag, string aad,
                                                     [[maybe_unused]] int64_t tag_length) noexcept {
  auto algorithm{parse_cipher_algorithm(method)};
  if (!algorithm.has_value()) {
    kphp::log::warning("Unknown cipher algorithm");
    co_return false;
  }

  if (&tag != &CryptoInstanceState::get().default_tag_dummy) {
    kphp::log::warning("The authenticated tag cannot be provided for cipher that doesn not support AEAD");
  }
  if (!aad.empty()) {
    kphp::log::warning("The additional authenticated data cannot be provided for cipher that doesn not support AEAD");
  }
  if (source_iv.empty()) {
    kphp::log::warning("Using an empty Initialization Vector (iv) is potentially insecure and not recommended");
  }

  auto key_iv{algorithm_pad_key_iv(*algorithm, source_key, source_iv, options)};
  if (key_iv.is_null()) {
    co_return false;
  }

  tl::TLBuffer tlb;
  tl::CbcEncrypt{.algorithm = *algorithm,
                 .padding = tl::BlockPadding::PKCS7,
                 .passphrase = {.value = {key_iv.val().first.c_str(), key_iv.val().first.size()}},
                 .iv = {.value = {key_iv.val().second.c_str(), key_iv.val().second.size()}},
                 .data = {.value = {data.c_str(), data.size()}}}
      .store(tlb);

  auto query{f$component_client_send_request(string{CRYPTO_COMPONENT_NAME.data(), static_cast<string::size_type>(CRYPTO_COMPONENT_NAME.size())},
                                             string{tlb.data(), static_cast<string::size_type>(tlb.size())})};
  string resp{co_await f$component_client_fetch_response(co_await query)};

  tlb.clean();
  tlb.store_bytes({resp.c_str(), static_cast<size_t>(resp.size())});

  tl::String response{};
  kphp::log::assertion(response.fetch(tlb));
  string result{response.inner.value.data(), static_cast<string::size_type>(response.inner.value.size())};
  co_return (options & static_cast<int64_t>(cipher_opts::OPENSSL_RAW_DATA)) ? std::move(result) : f$base64_encode(result);
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
    kphp::log::warning("Unknown cipher algorithm");
    co_return false;
  }

  if (!tag.empty()) {
    kphp::log::warning("The authenticated tag cannot be provided for cipher that doesn not support AEAD");
  }
  if (!aad.empty()) {
    kphp::log::warning("The additional authenticated data cannot be provided for cipher that doesn not support AEAD");
  }

  auto key_iv{algorithm_pad_key_iv(*algorithm, source_key, source_iv, options)};
  if (key_iv.is_null()) {
    co_return false;
  }
  tl::TLBuffer tlb;
  tl::CbcDecrypt{.algorithm = *algorithm,
                 .padding = tl::BlockPadding::PKCS7,
                 .passphrase = {.value = {key_iv.val().first.c_str(), key_iv.val().first.size()}},
                 .iv = {.value = {key_iv.val().second.c_str(), key_iv.val().second.size()}},
                 .data = {.value = {data.c_str(), data.size()}}}
      .store(tlb);

  auto query{f$component_client_send_request(string{CRYPTO_COMPONENT_NAME.data(), static_cast<string::size_type>(CRYPTO_COMPONENT_NAME.size())},
                                             string{tlb.data(), static_cast<string::size_type>(tlb.size())})};
  string resp{co_await f$component_client_fetch_response(co_await query)};

  tlb.clean();
  tlb.store_bytes({resp.c_str(), static_cast<size_t>(resp.size())});

  tl::String response{};
  kphp::log::assertion(response.fetch(tlb));
  co_return string{response.inner.value.data(), static_cast<string::size_type>(response.inner.value.size())};
}

namespace {

constexpr std::array<std::pair<std::string_view, tl::HashAlgorithm>, 6> HASH_ALGOS = {{{"md5", tl::HashAlgorithm::MD5},
                                                                                       {"sha1", tl::HashAlgorithm::SHA1},
                                                                                       {"sha224", tl::HashAlgorithm::SHA224},
                                                                                       {"sha256", tl::HashAlgorithm::SHA256},
                                                                                       {"sha384", tl::HashAlgorithm::SHA384},
                                                                                       {"sha512", tl::HashAlgorithm::SHA512}}};

std::optional<tl::HashAlgorithm> parse_hash_algorithm(std::string_view user_algo) noexcept {
  const auto* it{std::ranges::find_if(
      HASH_ALGOS,
      [user_algo = std::ranges::transform_view(user_algo, [](auto c) { return std::tolower(c); })](auto hash_algo) noexcept {
        return std::ranges::equal(user_algo, hash_algo);
      },
      [](const auto& hash_algo) noexcept { return hash_algo.first; })};

  return it != nullptr && it != HASH_ALGOS.end() ? std::optional{it->second} : std::nullopt;
}

kphp::coro::task<string> send_and_get_string(tl::TLBuffer tlb, bool raw_output) noexcept {
  auto query{f$component_client_send_request(string{CRYPTO_COMPONENT_NAME.data(), static_cast<string::size_type>(CRYPTO_COMPONENT_NAME.size())},
                                             string{tlb.data(), static_cast<string::size_type>(tlb.size())})};
  string resp{co_await f$component_client_fetch_response(co_await query)};

  tlb.clean();
  tlb.store_bytes({resp.c_str(), static_cast<size_t>(resp.size())});

  tl::String response{};
  kphp::log::assertion(response.fetch(tlb));

  if (!raw_output) {
    co_return kphp::strings::bin2hex(response.inner.value);
  }
  // Important to pass size because response.inner.value is binary so
  // it may contain zero in any position, not only in the end
  co_return string{response.inner.value.data(), static_cast<string::size_type>(response.inner.value.size())};
}

kphp::coro::task<string> hash_impl(tl::HashAlgorithm algo, string s, bool raw_output) noexcept {
  tl::TLBuffer tlb;
  tl::Hash{.algorithm = algo, .data = {.value = {s.c_str(), s.size()}}}.store(tlb);
  co_return co_await send_and_get_string(std::move(tlb), raw_output);
}

} // namespace

array<string> f$hash_algos() noexcept {
  array<string> response{array_size{HASH_ALGOS.size(), true}};
  for (auto [algo_name, _] : HASH_ALGOS) {
    response.push_back(string{algo_name.data(), static_cast<string::size_type>(algo_name.size())});
  }
  return response;
}

array<string> f$hash_hmac_algos() noexcept {
  return f$hash_algos();
}

kphp::coro::task<string> f$hash(string algo_str, string s, bool raw_output) noexcept {
  const auto algo{parse_hash_algorithm({algo_str.c_str(), algo_str.size()})};
  if (!algo.has_value()) [[unlikely]] {
    kphp::log::error("algo {} not supported in function hash", algo_str.c_str());
  }
  co_return co_await hash_impl(*algo, s, raw_output);
}

kphp::coro::task<string> f$hash_hmac(string algo_str, string s, string key, bool raw_output) noexcept {
  const auto algo{parse_hash_algorithm({algo_str.c_str(), algo_str.size()})};
  if (!algo.has_value()) [[unlikely]] {
    kphp::log::error("algo {} not supported in function hash", algo_str.c_str());
  }

  tl::TLBuffer tlb;
  tl::HashHmac{.algorithm = *algo, .data = {.value = {s.c_str(), s.size()}}, .secret_key = {.value = {key.c_str(), key.size()}}}.store(tlb);
  co_return co_await send_and_get_string(std::move(tlb), raw_output);
}

kphp::coro::task<string> f$sha1(string s, bool raw_output) noexcept {
  co_return co_await hash_impl(tl::HashAlgorithm::SHA1, s, raw_output);
}

int64_t f$crc32(const string& s) noexcept {
  return crc32_partial_generic(static_cast<const void*>(s.c_str()), s.size(), -1) ^ -1;
}
