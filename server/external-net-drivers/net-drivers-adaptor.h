// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/smart_ptrs/singleton.h"
#include "common/mixin/not_copyable.h"
#include "net/net-events.h"
#include "runtime/kphp_core.h"

struct php_query_connect_answer_t;

class Connector;
class Request;
class Response;

struct RequestInfo {
  int resumable_id{0};
  Response *response{nullptr};
};

class NetDriversAdaptor : vk::not_copyable {
public:
  array<Connector *> connectors; // invariant: this array is empty <=> script allocator is disabled

  static int epoll_gateway(int fd, void *data, event_t *ev);

  int register_connector(Connector *connector);

  template <typename T>
  T *get_connector(int connection_id) {
    return dynamic_cast<T *>(connectors.template get_value(connection_id));
  }

  void create_outbound_connections();
  php_query_connect_answer_t *php_query_connect(Connector *connector);
  void reset();

  int send_request(Connector *connector, Request *request);
  void process_external_db_request_net_query(Connector *connector, Request *request);

  int create_external_db_response_net_event(Response *response);
  void process_external_db_response_event(Response *response);

  void start_request_processing(int request_id, const RequestInfo &request_info);
  int finish_request_processing(int request_id, Response *response);
  Response *withdraw_response(int request_id);
private:
  array<RequestInfo> processing_requests;

  NetDriversAdaptor() = default;

  friend class vk::singleton<NetDriversAdaptor>;
};
