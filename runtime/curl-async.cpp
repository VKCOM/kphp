// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/curl-async.h"

#include "runtime/resumable.h"
#include "server/curl-adaptor.h"

class curl_exec_concurrently final : public Resumable {
private:
  using ReturnT = Optional<string>;

  std::unique_ptr<CurlRequest> request;
  double timeout_s{0.0};
  int resumable_id{0};
  std::unique_ptr<CurlResponse> response;

public:
  curl_exec_concurrently(std::unique_ptr<CurlRequest> &&request, double timeout_s) noexcept
    : request(std::move(request))
    , timeout_s(timeout_s) {}

  bool run() noexcept final {
    RESUMABLE_BEGIN
    resumable_id = vk::singleton<CurlAdaptor>::get().launch_request_resumable(std::move(request));
    response = f$wait<std::unique_ptr<CurlResponse>, false>(resumable_id, timeout_s);
    TRY_WAIT(curl_exec_concurrently_label, response, std::unique_ptr<CurlResponse>);
    RETURN(response ? response->response : ReturnT{false});
    RESUMABLE_END
  }
};

Optional<string> f$curl_exec_concurrently(curl_easy easy_id, double timeout_s) {
  auto request = CurlRequest::build(easy_id);
  if (!request) {
    return false;
  }
  return start_resumable<Optional<string>>(new curl_exec_concurrently(std::move(request), timeout_s));
}
