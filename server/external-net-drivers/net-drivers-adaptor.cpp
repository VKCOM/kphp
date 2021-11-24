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
#include "runtime/net_events.h"
#include "runtime/resumable.h"

namespace {

class request_resumable final : public Resumable {
  using ReturnT = Response *;
public:
  explicit request_resumable(int request_id)
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
int Storage::tagger<Response *>::get_tag()  noexcept {
  return 1266376770;
}

int NetDriversAdaptor::register_connector(Connector *connector) {
  int id = static_cast<int>(connectors.count());
  connectors.push_back(connector);
  return id;
}

void NetDriversAdaptor::create_outbound_connections() {
  // use const iterator to prevent unexpected mutate() and allocations
  for (auto it = connectors.cbegin(); it != connectors.cend(); ++it) {
    Connector *connector = it.get_value();
    if (!connector->connected()) {
      connector->connect_async();
    }
  }
}

php_query_connect_answer_t *NetDriversAdaptor::php_query_connect(Connector *connector) {
  assert (PHPScriptBase::is_running);

  //DO NOT use query after script is terminated!!!
  external_driver_connect q{connector};

  PHPScriptBase::current_script->ask_query(&q);

  return static_cast<php_query_connect_answer_t *>(q.ans);
}

void NetDriversAdaptor::reset() {
  hard_reset_var(connectors);
  hard_reset_var(processing_requests);
}

int NetDriversAdaptor::epoll_gateway(int fd, void *data, event_t *ev) {
  (void) fd;
  auto *connector = reinterpret_cast<Connector *>(data);
  assert(connector);

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

int NetDriversAdaptor::send_request(Connector *connector, Request *request) {
  slot_id_t request_id = external_db_requests_factory.create_slot();
  net_query_t *query = create_net_query(net_query_type_t::external_db_request);
  query->slot_id = request_id;
  query->connector = connector;
  query->external_db_request = request;
  int resumable_id = register_forked_resumable(new request_resumable{request_id}); // TODO: resumable class?

  start_request_processing(request_id, RequestInfo{resumable_id});

  update_precise_now();
  wait_net(0);
  update_precise_now();

  return resumable_id;
}

void NetDriversAdaptor::process_external_db_request_net_query(int request_id, Connector *connector, Request *request) {
  while (!connector->connect_async()) {}
  connector->push_async_request(request_id, request);
}

int NetDriversAdaptor::create_external_db_response_net_event(int request_id, Response *response) {
  if (!external_db_requests_factory.is_valid_slot(request_id)) {
    return 0;
  }
  net_event_t *event = nullptr;
  int status = alloc_net_event(request_id, net_event_type_t::external_db_answer, &event);
  if (status <= 0) {
    return status;
  }
  event->response = response;
  return 1;
}

void NetDriversAdaptor::process_external_db_response_event(int request_id, Response *response) {
  int resumable_id = finish_request_processing(request_id, response);
  if (resumable_id == 0) {
    return;
  }
  resumable_run_ready(resumable_id);
}

void NetDriversAdaptor::start_request_processing(int request_id, const RequestInfo &request_info) {
  processing_requests[request_id] = request_info;
}

int NetDriversAdaptor::finish_request_processing(int request_id, Response *response) {
  if (!processing_requests.has_key(request_id)) {
    return 0;
  }
  auto &req = processing_requests[request_id];
  req.response = response;
  return req.resumable_id;
}

Response *NetDriversAdaptor::withdraw_response(int request_id) {
  auto req_info = processing_requests.get_value(request_id);
  php_assert(req_info.resumable_id != 0);
  php_assert(req_info.response);
  processing_requests.unset(request_id);
  return req_info.response;
}

//template <typename ReturnT>
//class request_resumable : public Resumable {
//public:
//  explicit request_resumable(int job_id)
//    : job_id(job_id) {}
//
//protected:
//  bool run() final {
//    const class_instance<C$KphpJobWorkerResponse> &res = vk::singleton<job_workers::ProcessingJobs>::get().withdraw(job_id);
//    RETURN(res);
//  }
//
//private:
//  int job_id;
//};
//
//class fetch_response_resumable final : public Resumable {
//private:
//  Optional < int64_t > v$ans;
//  future< int64_t > v$fork;
//public:
//  using ReturnT = Optional < bool >;
//  fetch_response_resumable() noexcept  {
//  }
//  bool run() noexcept  final {
//    RESUMABLE_BEGIN
//    //59:     $fork = fork(foo(1));
//    v$fork = f$fork$foo(1_i64);
//    //60:     $ans = wait($fork);
//    v$ans = f$wait< Optional < int64_t > >(v$fork);
//    TRY_WAIT(resumable_label$uec5d7b577a180cb5_0, v$ans, Optional < int64_t >);
//    CHECK_EXCEPTION(RETURN ({}));
//    //61:     return null;
//    RETURN (Optional<bool>{});
//
//    RESUMABLE_END
//  }
//};
