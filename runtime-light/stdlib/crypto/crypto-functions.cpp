//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/crypto/crypto-functions.h"

#include <cstddef>

#include "common/tl/constants/common.h"
#include "runtime-common/stdlib/server/url-functions.h"
#include "runtime-light/stdlib/component/component-api.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"

namespace {

constexpr const char *CRYPTO_COMPONENT_NAME = "crypto";

} // namespace

task_t<Optional<string>> f$openssl_random_pseudo_bytes(int64_t length) noexcept {
  if (length <= 0 || length > string::max_size()) {
    co_return false;
  }

  // TODO think about performance when transferring data

  tl::GetCryptosecurePseudorandomBytes request{.size = static_cast<int32_t>(length)};

  tl::TLBuffer buffer;
  request.store(buffer);

  auto query = f$component_client_send_request(string{CRYPTO_COMPONENT_NAME}, string{buffer.data(), static_cast<string::size_type>(buffer.size())});
  string resp = co_await f$component_client_fetch_response(co_await query);

  buffer.clean();
  buffer.store_bytes({resp.c_str(), static_cast<size_t>(resp.size())});

  // Maybe better to do this in some structure, but there's not much work to do with TL here
  std::optional<uint32_t> magic = buffer.fetch_trivial<uint32_t>();
  if (!magic.has_value() || *magic != TL_MAYBE_TRUE) {
    co_return false;
  }
  std::string_view str_view = buffer.fetch_string();
  co_return string(str_view.data(), str_view.size());
}

task_t<Optional<array<mixed>>> f$openssl_x509_parse(const string &data, bool shortnames) noexcept {
  tl::GetPemCertInfo request{.is_short = shortnames, .bytes = data};

  tl::TLBuffer buffer;
  request.store(buffer);

  auto query = f$component_client_send_request(string{CRYPTO_COMPONENT_NAME}, string{buffer.data(), static_cast<string::size_type>(buffer.size())});
  string resp_from_platform = co_await f$component_client_fetch_response(co_await query);

  buffer.clean();
  buffer.store_bytes({resp_from_platform.c_str(), static_cast<size_t>(resp_from_platform.size())});

  tl::GetPemCertInfoResponse response;
  if (!response.fetch(buffer)) {
    co_return false;
  }

  co_return response.data;
}

task_t<bool> f$openssl_sign(const string &data, string &signature, const string &private_key, int64_t algo) noexcept {
  tl::DigestSign request{.data = data, .private_key = private_key, .algorithm = static_cast<tl::DigestAlgorithm>(algo)};

  tl::TLBuffer buffer;
  request.store(buffer);

  auto query = f$component_client_send_request(string{CRYPTO_COMPONENT_NAME}, string(buffer.data(), buffer.size()));
  string resp_from_platform = co_await f$component_client_fetch_response(co_await query);

  buffer.clean();
  buffer.store_bytes({resp_from_platform.c_str(), static_cast<size_t>(resp_from_platform.size())});

  std::optional<uint32_t> magic = buffer.fetch_trivial<uint32_t>();
  if (!magic.has_value() || *magic != TL_MAYBE_TRUE) {
    co_return false;
  }

  std::string_view str_view = buffer.fetch_string();
  signature = string(str_view.data(), str_view.size());

  co_return true;
}

task_t<int64_t> f$openssl_verify(const string &data, const string &signature, const string &pub_key, int64_t algo) noexcept {
  tl::DigestVerify request{.data = data, .public_key = pub_key, .algorithm = static_cast<tl::DigestAlgorithm>(algo), .signature = signature};

  tl::TLBuffer buffer;
  request.store(buffer);

  auto query = f$component_client_send_request(string{CRYPTO_COMPONENT_NAME}, string(buffer.data(), buffer.size()));
  string resp_from_platform = co_await f$component_client_fetch_response(co_await query);

  buffer.clean();
  buffer.store_bytes({resp_from_platform.c_str(), static_cast<size_t>(resp_from_platform.size())});

  // For now returns only 1 or 0, -1 is never returned
  // Because it's currently impossible to distiguish error from negative verification
  std::optional<uint32_t> magic = buffer.fetch_trivial<uint32_t>();
  if (!magic.has_value() || *magic != TL_BOOL_TRUE) {
    co_return 0;
  }

  co_return 1;
}

namespace {

constexpr std::string_view AES_128_CBC = "aes-128-cbc";
constexpr std::string_view AES_256_CBC = "aes-256-cbc";

std::optional<tl::CipherAlgorithm> parse_algorithm(const string &method) noexcept {
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
      php_warning("unexpected cipher algorithm");
      return 0;
    }
  }
}

enum class cipher_opts : int64_t { OPENSSL_RAW_DATA = 1, OPENSSL_ZERO_PADDING = 2, OPENSSL_DONT_ZERO_PAD_KEY = 4 };

Optional<std::pair<string, string>> algorithm_pad_key_iv(tl::CipherAlgorithm algorithm, const string &source_key, const string &source_iv,
                                                         int64_t options) noexcept {
  const size_t iv_required_len = algorithm_iv_len(algorithm);
  const size_t key_required_len = algorithm_key_len(algorithm);
  auto iv = source_iv;
  if (iv.size() < iv_required_len) {
    php_warning("IV passed is only %d bytes long, cipher expects an IV of precisely %zd bytes, padding with \\0", iv.size(), iv_required_len);
    iv.append(static_cast<string::size_type>(iv_required_len - iv.size()), '\0');
  } else if (iv.size() > iv_required_len) {
    php_warning("IV passed is %d bytes long which is longer than the %zd expected by selected cipher, truncating", iv.size(), iv_required_len);
    iv.shrink(static_cast<string::size_type>(iv_required_len));
  }

  auto key = source_key;
  if (key.size() < key_required_len) {
    if (options & static_cast<int64_t>(cipher_opts::OPENSSL_DONT_ZERO_PAD_KEY)) {
      php_warning("Key length should be %ld bytes long:\n", key_required_len);
      return false;
    }
    php_warning("passphrase passed is only %d bytes long, cipher expects an passphrase of precisely %zd bytes, padding with \\0", key.size(), key_required_len);
    key.append(key_required_len - key.size(), '\0');
  } else if (key.size() > key_required_len) {
    php_warning("passphrase passed is %d bytes long which is longer than the %zd expected by selected cipher, truncating", key.size(), key_required_len);
    key.shrink(static_cast<string::size_type>(key_required_len));
  }

  if (options & static_cast<int64_t>(cipher_opts::OPENSSL_ZERO_PADDING)) {
    php_warning("OPENSSL_ZERO_PADDING option is not supported for now:\n");
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

Optional<int64_t> f$openssl_cipher_iv_length(const string &method) noexcept {
  auto algorithm = parse_algorithm(method);
  if (!algorithm.has_value()) {
    php_warning("Unknown cipher algorithm");
    return false;
  }
  return algorithm_iv_len(*algorithm);
}

task_t<Optional<string>> f$openssl_encrypt(const string &data, const string &method, const string &source_key, int64_t options, const string &source_iv,
                                           string &tag, const string &aad, int64_t tag_length __attribute__((unused))) noexcept {
  auto algorithm = parse_algorithm(method);
  if (!algorithm.has_value()) {
    php_warning("Unknown cipher algorithm");
    co_return false;
  }

  if (&tag != &CryptoInstanceState::get().default_tag_dummy) {
    php_warning("The authenticated tag cannot be provided for cipher that doesn not support AEAD");
  }
  if (!aad.empty()) {
    php_warning("The additional authenticated data cannot be provided for cipher that doesn not support AEAD");
  }
  if (source_iv.empty()) {
    php_warning("Using an empty Initialization Vector (iv) is potentially insecure and not recommended");
  }

  auto key_iv = algorithm_pad_key_iv(*algorithm, source_key, source_iv, options);
  if (key_iv.is_null()) {
    co_return false;
  }
  tl::CbcEncrypt request{.algorithm = *algorithm,
                         .padding = tl::BlockPadding::PKCS7,
                         .passphrase = std::move(key_iv.val().first),
                         .iv = std::move(key_iv.val().second),
                         .data = data};

  tl::TLBuffer buffer;
  request.store(buffer);

  auto query = f$component_client_send_request(string{CRYPTO_COMPONENT_NAME}, string{buffer.data(), static_cast<string::size_type>(buffer.size())});
  string resp = co_await f$component_client_fetch_response(co_await query);

  buffer.clean();
  buffer.store_bytes({resp.c_str(), static_cast<size_t>(resp.size())});

  tl::String response{.value = resp};
  // TODO: parse error?
  if (!response.fetch(buffer)) {
    co_return false;
  }
  co_return(options & static_cast<int64_t>(cipher_opts::OPENSSL_RAW_DATA)) ? std::move(response.value) : f$base64_encode(response.value);
}

task_t<Optional<string>> f$openssl_decrypt(string data, const string &method, const string &source_key, int64_t options, const string &source_iv, string tag,
                                           const string &aad) noexcept {
  if (!(options & static_cast<int64_t>(cipher_opts::OPENSSL_RAW_DATA))) {
    Optional<string> decoding_data = f$base64_decode(data, true);
    if (!decoding_data.has_value()) {
      php_warning("Failed to base64 decode the input");
      co_return false;
    }
    data = std::move(decoding_data.val());
  }

  auto algorithm = parse_algorithm(method);
  if (!algorithm.has_value()) {
    php_warning("Unknown cipher algorithm");
    co_return false;
  }

  if (!tag.empty()) {
    php_warning("The authenticated tag cannot be provided for cipher that doesn not support AEAD");
  }
  if (!aad.empty()) {
    php_warning("The additional authenticated data cannot be provided for cipher that doesn not support AEAD");
  }

  auto key_iv = algorithm_pad_key_iv(*algorithm, source_key, source_iv, options);
  if (key_iv.is_null()) {
    co_return false;
  }
  tl::CbcDecrypt request{.algorithm = *algorithm,
                         .padding = tl::BlockPadding::PKCS7,
                         .passphrase = std::move(key_iv.val().first),
                         .iv = std::move(key_iv.val().second),
                         .data = data};

  tl::TLBuffer buffer;
  request.store(buffer);

  auto query = f$component_client_send_request(string{CRYPTO_COMPONENT_NAME}, string{buffer.data(), static_cast<string::size_type>(buffer.size())});
  string resp = co_await f$component_client_fetch_response(co_await query);

  buffer.clean();
  buffer.store_bytes({resp.c_str(), static_cast<size_t>(resp.size())});

  tl::String response{.value = resp};
  // TODO: parse error?
  if (!response.fetch(buffer)) {
    co_return false;
  }
  co_return std::move(response.value);
}
