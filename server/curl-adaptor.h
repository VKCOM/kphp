// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>

#include "common/smart_ptrs/singleton.h"
#include "common/mixin/not_copyable.h"
#include "runtime/signal_safe_hashtable.h"

namespace curl_async {

class CurlRequest;
class CurlResponse;

struct CurlRequestInfo {
  int resumable_id{0};
  std::unique_ptr<CurlResponse> response;
};

class CurlAdaptor : vk::not_copyable {
public:
  int launch_request_resumable(const CurlRequest &request) noexcept;
  void process_request_net_query(const CurlRequest &request) noexcept;
  void finish_request_resumable(std::unique_ptr<CurlResponse> &&response) noexcept;
  void process_response_event(std::unique_ptr<CurlResponse> &&response) noexcept;

  std::unique_ptr<CurlResponse> withdraw_response(int request_id) noexcept;
  void finish_request(const CurlRequest &request) noexcept;
  void reset() noexcept;

private:
  class CurlRequestResumable;
  int finish_request_processing(int request_id, std::unique_ptr<CurlResponse> &&response) noexcept;

  SignalSafeHashtable<int, CurlRequestInfo> processing_requests;
};
} // namespace curl_async
