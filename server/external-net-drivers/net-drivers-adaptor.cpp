// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/external-net-drivers/net-drivers-adaptor.h"
#include "server/external-net-drivers/connector.h"
#include "server/external-net-drivers/request.h"
#include "server/external-net-drivers/response.h"
#include "server/php-queries-types.h"
#include "server/php-runner.h"
#include "server/php-queries.h"
#include "runtime/critical_section.h"
#include "runtime/net_events.h"
#include "runtime/resumable.h"

namespace {

/**
 * Actually it never interrupts, but it's needed for emulating `fork` call on every net request. (like `rpc_resumable` or `job_resumable`)
 */
class RequestResumable final : public Resumable {
  using ReturnT = Response *;
public:
  explicit RequestResumable(int request_id)
    : request_id(request_id) {}

protected:
  bool run() noexcept final {
    auto &adaptor = vk::singleton<NetDriversAdaptor>::get();
    Response *res = adaptor.withdraw_response(request_id);
    RETURN(res);
  }
private:
  int request_id;
};

} // namespace

template<>
int Storage::tagger<Response *>::get_tag() noexcept {
  return 1266376770;
}

int NetDriversAdaptor::register_connector(std::unique_ptr<Connector> &&connector) noexcept {
  connector->connect_async(); // try to connect here beforehand, why not

  // Connection id must be bound to current script run // TODO: because we need to be able to identify if this connection from another scipr
  int connection_id = external_db_requests_factory.create_slot();
  connector->connector_id = connection_id;
  connectors.put(connection_id, std::move(connector));
  return connection_id;
}

void NetDriversAdaptor::create_outbound_connections() noexcept {
  // use const iterator to prevent unexpected mutate() and allocations
  for (auto it = connectors.cbegin(); it != connectors.cend(); ++it) {
    const auto &connector = it->second;
    if (!connector->connected()) {
      connector->connect_async();
    }
  }
}

php_query_connect_answer_t *NetDriversAdaptor::php_query_connect(std::unique_ptr<Connector> &&connector) noexcept {
  assert (PHPScriptBase::is_running);

  //DO NOT use query after script is terminated!!!
  external_driver_connect q{std::move(connector)};

  PHPScriptBase::current_script->ask_query(&q);

  return static_cast<php_query_connect_answer_t *>(q.ans);
}

void NetDriversAdaptor::reset() noexcept {
  connectors.clear();
  hard_reset_var(processing_requests);
}

int NetDriversAdaptor::epoll_gateway(int fd, void *data, event_t *ev) noexcept {
  auto *connector = reinterpret_cast<Connector *>(data);
  assert(connector);
  assert(connector->get_fd() == fd);

  if (ev->ready & EVT_SPEC) {
    fprintf(stderr, "@@@@@@@ EVT_SPEC\n"); // TODO: handle_spec ?
  }
  if (ev->ready & EVT_WRITE) {
    connector->handle_write();
  }
  if (ev->ready & EVT_READ) {
    connector->handle_read();
  }
  return 0; // TODO: EVA_REMOVE DESTROY??
}

int NetDriversAdaptor::send_request(int connector_id, std::unique_ptr<Request> &&request) noexcept {
  slot_id_t request_id = external_db_requests_factory.create_slot();
  request->request_id = request_id;

  net_query_t *query = create_net_query();
  query->slot_id = request_id;
  query->data = net_queries_data::external_db_send{connector_id, std::move(request)};
  int resumable_id = register_forked_resumable(new RequestResumable{request_id});

  start_request_processing(request_id, RequestInfo{resumable_id});

  update_precise_now();
  wait_net(0);
  update_precise_now();

  return resumable_id;
}

void NetDriversAdaptor::process_external_db_request_net_query(int connector_id, std::unique_ptr<Request> &&request) noexcept {
  auto *connector = get_connector<Connector>(connector_id);
  if (connector == nullptr) {
    php_warning("Connector with id = %d doesn't exist. Probably the connection is closed already", connector_id);
    return;
  }
  while (!connector->connect_async()) {}
  connector->push_async_request(std::move(request));
}

int NetDriversAdaptor::create_external_db_response_net_event(std::unique_ptr<Response> &&response) noexcept {
  if (!external_db_requests_factory.is_valid_slot(response->bound_request_id)) {
    return 0;
  }
  net_event_t *event = nullptr;
  int status = alloc_net_event(response->bound_request_id, &event);
  if (status <= 0) {
    return status;
  }
  event->data = std::move(response);
  return 1;
}

void NetDriversAdaptor::process_external_db_response_event(std::unique_ptr<Response> &&response) noexcept {
  int resumable_id = finish_request_processing(response->bound_request_id, std::move(response));
  if (resumable_id == 0) {
    return;
  }
  resumable_run_ready(resumable_id);
}

void NetDriversAdaptor::start_request_processing(int request_id, RequestInfo &&request_info) noexcept {
  dl::CriticalSectionGuard guard;
  processing_requests[request_id] = std::move(request_info);
}

int NetDriversAdaptor::finish_request_processing(int request_id, std::unique_ptr<Response> &&response) noexcept {
  dl::CriticalSectionGuard guard;
  if (!processing_requests.count(request_id)) {
    return 0;
  }
  auto &req = processing_requests[request_id];
  req.response = std::move(response);
  return req.resumable_id;
}

Response *NetDriversAdaptor::withdraw_response(int request_id) noexcept {
  dl::CriticalSectionGuard guard;
  auto req_info_it = processing_requests.find(request_id);
  php_assert(req_info_it->second.resumable_id != 0);
  php_assert(req_info_it->second.response);
  processing_requests.erase(req_info_it); // TODO: fix
  return req_info_it->second.response;
}

void NetDriversAdaptor::remove_connector(int connector_id) noexcept {
  connectors.remove(connector_id);
}
