// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/curl-async.h"

#include "runtime/resumable.h"
#include "server/curl-adaptor.h"

namespace curl_async {

class curl_exec_concurrently final : public Resumable {
private:
  using ReturnT = Optional<string>;

  const double timeout_s{0.0};
  int resumable_id{0};
  const CurlRequest request;
  std::unique_ptr<CurlResponse> response;

public:
  curl_exec_concurrently(const CurlRequest &request, double timeout_s) noexcept
    : timeout_s(timeout_s)
    , request(request) {}

  bool run() noexcept final {
    RESUMABLE_BEGIN
    resumable_id = vk::singleton<CurlAdaptor>::get().launch_request_resumable(request);
    response = f$wait<std::unique_ptr<CurlResponse>, false>(resumable_id, timeout_s);
    TRY_WAIT(curl_exec_concurrently_label, response, std::unique_ptr<CurlResponse>);
    vk::singleton<CurlAdaptor>::get().finish_request(request);
    RETURN(response ? response->response : ReturnT{false});
    RESUMABLE_END
  }
};
} // namespace curl_async

Optional<string> f$curl_exec_concurrently(curl_easy easy_id, double timeout_s) {
  try {
    auto request = curl_async::CurlRequest::build(easy_id);
    return start_resumable<Optional<string>>(new curl_async::curl_exec_concurrently(request, timeout_s));
  } catch (...) {
    return false;
  }
}
