// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/curl-adaptor.h"

#include "runtime/curl.h"
#include "runtime/resumable.h"
#include "server/php-engine.h"
#include "server/slot-ids-factory.h"
#include "server/php-queries.h"

template<>
int Storage::tagger<std::unique_ptr<CurlResponse>>::get_tag() noexcept {
  return 447403581;
}

class CurlAdaptor::CurlRequestResumable final : public Resumable {
  using ReturnT = std::unique_ptr<CurlResponse>;
  int request_id{0};

public:
  explicit CurlRequestResumable(int request_id) noexcept
    : request_id(request_id) {}

protected:
  bool run() final {
    ReturnT res = vk::singleton<CurlAdaptor>::get().withdraw_response(request_id);
    RETURN(std::move(res));
  }
};

void CurlAdaptor::reset() noexcept {
  processing_requests.clear();
  processing_requests_holder.clear();
}

int CurlAdaptor::launch_request_resumable(std::unique_ptr<CurlRequest> &&request) noexcept {
  slot_id_t request_id = curl_requests_factory.create_slot();
  request->request_id = request_id;

  net_query_t *query = create_net_query();
  query->slot_id = request_id;
  query->data = request.release();
  int resumable_id = register_forked_resumable(new CurlRequestResumable{request_id});
  processing_requests.insert(request_id, CurlRequestInfo{resumable_id});

  return resumable_id;
}

void CurlAdaptor::process_request_net_query(std::unique_ptr<CurlRequest> &&request) noexcept {
  request->send_async();
  int request_id = request->request_id;
  processing_requests_holder.insert(request_id, std::move(request));
}

void CurlAdaptor::finish_request_resumable(std::unique_ptr<CurlResponse> &&response) noexcept {
  int event_status = 0;
  if (curl_requests_factory.is_from_current_script_execution(response->bound_request_id)) {
    net_event_t *event = nullptr;
    event_status = alloc_net_event(response->bound_request_id, &event);
    if (event_status > 0) {
      event->data = response.release();
      event_status = 1;
    }
  }
  on_net_event(event_status);
}

void CurlAdaptor::process_response_event(std::unique_ptr<CurlResponse> &&response) noexcept {
  int resumable_id = finish_request_processing(response->bound_request_id, std::move(response));
  if (resumable_id == 0) {
    return;
  }
  resumable_run_ready(resumable_id);
}

int CurlAdaptor::finish_request_processing(int request_id, std::unique_ptr<CurlResponse> &&response) noexcept {
  auto *req_info = processing_requests.get(request_id);
  if (req_info == nullptr) {
    return 0;
  }
  int resumable_id = req_info->resumable_id;
  processing_requests.insert_or_assign(request_id, CurlRequestInfo{resumable_id, std::move(response)});
  return resumable_id;
}

std::unique_ptr<CurlResponse> CurlAdaptor::withdraw_response(int request_id) noexcept {
  CurlRequestInfo req_info = processing_requests.extract(request_id);
  php_assert(req_info.resumable_id != 0);
  php_assert(req_info.response);
  return std::move(req_info.response);
}
