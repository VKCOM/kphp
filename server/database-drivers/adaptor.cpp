// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/database-drivers/adaptor.h"

#include "runtime/critical_section.h"
#include "runtime/net_events.h"
#include "runtime/resumable.h"
#include "server/database-drivers/connector.h"
#include "server/database-drivers/request.h"
#include "server/database-drivers/response.h"
#include "server/php-engine.h"
#include "server/php-queries-types.h"
#include "server/php-runner.h"
#include "server/php-queries.h"

template<>
int Storage::tagger<std::unique_ptr<database_drivers::Response>>::get_tag() noexcept {
  return 1463831301;
}

namespace database_drivers {

/**
 * Actually it never interrupts, but it's needed for emulating `fork` call on every net request. (like `rpc_resumable` or `job_resumable`)
 */
class Adaptor::RequestResumable final : public Resumable {
  using ReturnT = std::unique_ptr<Response>;

public:
  explicit RequestResumable(int request_id)
    : request_id(request_id) {}

  bool is_internal_resumable() const noexcept final {
    return true;
  }

protected:
  bool run() noexcept final {
    auto &adaptor = vk::singleton<database_drivers::Adaptor>::get();
    std::unique_ptr<Response> res = adaptor.withdraw_response(request_id);
    RETURN(std::move(res));
  }

private:
  int request_id;
};

int Adaptor::register_connector(std::unique_ptr<Connector> &&connector) noexcept {
  int connection_id = external_db_requests_factory.create_slot();
  connector->connector_id = connection_id;
  connector->connect_async_and_epoll_insert(); // try to connect here beforehand, why not
  connectors.insert(connection_id, std::move(connector));
  return connection_id;
}

void Adaptor::create_outbound_connections() noexcept {
  for (const auto &item : connectors) {
    const auto &connector = item.second;
    if (!connector->connected()) {
      connector->connect_async_and_epoll_insert();
    }
  }
}

int Adaptor::initiate_connect(std::unique_ptr<Connector> &&connector) noexcept {
  assert(PhpScript::in_script_context);

  // DO NOT use query after script is terminated!!!
  ::external_driver_connect q{std::move(connector)};

  PhpScript::current_script->ask_query(&q);

  return static_cast<php_query_connect_answer_t *>(q.ans)->connection_id;
}

void Adaptor::reset() noexcept {
  connectors.clear();
  processing_requests.clear();
}

int Adaptor::epoll_gateway(int fd, void *data, event_t *ev) noexcept {
  dl::CriticalSectionGuard guard;

  auto connector_id = static_cast<int32_t>(reinterpret_cast<int64_t>(data));
  auto *connector = vk::singleton<database_drivers::Adaptor>::get().get_connector<Connector>(connector_id);
  if (connector == nullptr) {
    return 0;
  }
  assert(connector->get_fd() == fd);

  if (ev->ready & EVT_SPEC) {
    connector->handle_special();
  }
  if (ev->ready & EVT_WRITE) {
    connector->handle_write();
  }
  if (ev->ready & EVT_READ) {
    connector->handle_read();
  }
  return 0; // TODO: EVA_REMOVE DESTROY??
}

int Adaptor::launch_request_resumable(std::unique_ptr<Request> &&request) noexcept {
  slot_id_t request_id = external_db_requests_factory.create_slot();
  request->request_id = request_id;

  ::net_query_t *query = create_net_query();
  query->slot_id = request_id;
  query->data = request.release();
  int resumable_id = register_forked_resumable(new RequestResumable{request_id});

  start_request_processing(request_id, RequestInfo{resumable_id});

  update_precise_now();
  wait_net(0);
  update_precise_now();

  return resumable_id;
}

void Adaptor::process_external_db_request_net_query(std::unique_ptr<Request> &&request) noexcept {
  auto *connector = get_connector<Connector>(request->connector_id);
  if (connector == nullptr) {
    return;
  }
  AsyncOperationStatus status;
  while ((status = connector->connect_async_and_epoll_insert()) == AsyncOperationStatus::IN_PROGRESS) {
  }
  if (status == AsyncOperationStatus::COMPLETED) {
    connector->push_async_request(std::move(request));
  } else {
    remove_connector(request->connector_id);
  }
}

void Adaptor::finish_request_resumable(std::unique_ptr<Response> &&response) noexcept {
  int event_status = 0;
  if (external_db_requests_factory.is_from_current_script_execution(response->bound_request_id)) {
    ::net_event_t *event = nullptr;
    event_status = ::alloc_net_event(response->bound_request_id, &event);
    if (event_status > 0) {
      event->data = response.release();
      event_status = 1;
    }
  }
  on_net_event(event_status); // wakeup php worker to make it process new net event and continue corresponding resumable
}

std::unique_ptr<Response> Adaptor::wait_request_resumable(int resumable_id, double timeout) const noexcept {
  return f$wait<std::unique_ptr<Response>, false>(resumable_id, timeout);
}

void Adaptor::process_external_db_response_event(std::unique_ptr<Response> &&response) noexcept {
  int resumable_id = finish_request_processing(response->bound_request_id, std::move(response));
  if (resumable_id == 0) {
    return;
  }
  resumable_run_ready(resumable_id);
}

void Adaptor::start_request_processing(int request_id, RequestInfo &&request_info) noexcept {
  processing_requests.insert(request_id, std::move(request_info));
}

int Adaptor::finish_request_processing(int request_id, std::unique_ptr<Response> &&response) noexcept {
  auto *req_info = processing_requests.get(request_id);
  if (req_info == nullptr) {
    return 0;
  }
  int resumable_id = req_info->resumable_id;
  processing_requests.insert_or_assign(request_id, RequestInfo{resumable_id, std::move(response)});
  return resumable_id;
}

std::unique_ptr<Response> Adaptor::withdraw_response(int request_id) noexcept {
  RequestInfo req_info = processing_requests.extract(request_id);
  php_assert(req_info.resumable_id != 0);
  php_assert(req_info.response);
  return std::move(req_info.response);
}

void Adaptor::remove_connector(int connector_id) noexcept {
  connectors.erase(connector_id);
}

} // namespace database_drivers
