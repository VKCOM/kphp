//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/crypto/crypto-functions.h"

#include "common/tl/constants/common.h"
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

  string request_buf;
  request_buf.append(buffer.data(), buffer.size());

  auto query = f$component_client_send_request(string{CRYPTO_COMPONENT_NAME}, string{buffer.data(), static_cast<string::size_type>(buffer.size())});
  string resp = co_await f$component_client_fetch_response(co_await query);

  buffer.clean();
  buffer.store_bytes(resp.c_str(), resp.size());

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

  string request_buf;
  request_buf.append(buffer.data(), buffer.size());

  auto query = f$component_client_send_request(string{CRYPTO_COMPONENT_NAME}, request_buf);
  string resp_from_platform = co_await f$component_client_fetch_response(co_await query);

  buffer.clean();
  buffer.store_bytes(resp_from_platform.c_str(), resp_from_platform.size());

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
  buffer.store_bytes(resp_from_platform.c_str(), resp_from_platform.size());

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
  buffer.store_bytes(resp_from_platform.c_str(), resp_from_platform.size());

  // For now returns only 1 or 0, -1 is never returned
  // Because it's currently impossible to distiguish error from negative verification
  std::optional<uint32_t> magic = buffer.fetch_trivial<uint32_t>();
  if (!magic.has_value() || *magic != TL_BOOL_TRUE) {
    co_return 0;
  }

  co_return 1;
}
