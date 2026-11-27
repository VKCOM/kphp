// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/curl-async.h"

#include "runtime-common/stdlib/diagnostics/curl-time-stats.h"
#include "runtime/resumable.h"
#include "server/curl-adaptor.h"

namespace curl_async {

class curl_exec_concurrently final : public Resumable {
private:
  using ReturnT = Optional<string>;
  using Timer = decltype(CurlTimeStats::get().write(CurlBuiltin::curl_exec_concurrently));

  const double timeout_s{0.0};
  int resumable_id{0};
  Timer timer;
  const CurlRequest request;
  std::unique_ptr<CurlResponse> response;

public:
  curl_exec_concurrently(const CurlRequest& request, double timeout_s, Timer&& timer) noexcept
      : timeout_s(timeout_s),
        timer(std::move(timer)),
        request(request) {
    this->request.builtin_timer = &this->timer;
  }

  bool run() noexcept final {
    BuiltinTimeNetworkScope network_scope{timer};
    RESUMABLE_BEGIN
    resumable_id = vk::singleton<CurlAdaptor>::get().launch_request_resumable(request);
    response = f$wait<std::unique_ptr<CurlResponse>, false>(resumable_id, timeout_s);
    TRY_WAIT(curl_exec_concurrently_label, response, std::unique_ptr<CurlResponse>);
    request.network_wait.end();
    timer.subtract_elapsed_ns(request.network_wait.elapsed_ns());
    vk::singleton<CurlAdaptor>::get().finish_request(request);
    RETURN(response ? response->response : ReturnT{false});
    RESUMABLE_END
  }
};
} // namespace curl_async

Optional<string> f$curl_exec_concurrently(curl_easy easy_id, double timeout_s) {
  auto timer{CurlTimeStats::get().write(CurlBuiltin::curl_exec_concurrently)};
  BuiltinTimeNetworkScope network_scope{timer};
  try {
    auto request = curl_async::CurlRequest::build(easy_id);
    return start_resumable<Optional<string>>(new curl_async::curl_exec_concurrently(request, timeout_s, std::move(timer)));
  } catch (...) {
    return false;
  }
}
