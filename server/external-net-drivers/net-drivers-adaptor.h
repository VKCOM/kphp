// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>
#include <unordered_map>

#include "common/smart_ptrs/singleton.h"
#include "common/mixin/not_copyable.h"
#include "net/net-events.h"
#include "runtime/critical_section.h"
#include "runtime/kphp_core.h"
#include "server/external-net-drivers/connector.h"

struct php_query_connect_answer_t;

class Connector;
class Request;
class Response;

struct RequestInfo {
  int resumable_id{0};
  std::unique_ptr<Response> response;
};

/**
 * This STL container is safe to use in PHP runtime environment.
 * All modifiable operations are guarded with dl::CriticalSectionGuard.
 */
class ConnectorsContainer {
public:
  void put(int id, std::unique_ptr<Connector> &&connector) noexcept {
    dl::CriticalSectionGuard guard;
    connectors[id] = std::move(connector);
  }

  void remove(int id) noexcept {
    dl::CriticalSectionGuard guard;
    connectors.erase(id);
  }

  Connector *get(int id) const noexcept {
    auto it = connectors.find(id);
    if (it == connectors.end()) {
      return nullptr;
    }
    return it->second.get();
  }

  auto cbegin() const noexcept {
    return connectors.cbegin();
  }

  auto cend() const noexcept {
    return connectors.cend();
  }

  void clear () {
    dl::CriticalSectionGuard guard;
    connectors.clear();
  }
private:
  std::unordered_map<int, std::unique_ptr<Connector>> connectors;
};

class NetDriversAdaptor : vk::not_copyable {
public:
  static int epoll_gateway(int fd, void *data, event_t *ev) noexcept;

  void reset() noexcept;

  int register_connector(std::unique_ptr<Connector> &&connector) noexcept;
  void remove_connector(int connector_id) noexcept;

  template <typename T>
  T *get_connector(int connection_id) noexcept {
    auto *ptr = connectors.get(connection_id);
    if (!ptr) {
      return nullptr;
    }
    return dynamic_cast<T *>(ptr);
  }

  void create_outbound_connections() noexcept;
  php_query_connect_answer_t *php_query_connect(std::unique_ptr<Connector> &&connector) noexcept;

  int send_request(int connector_id, std::unique_ptr<Request> &&request) noexcept;
  void process_external_db_request_net_query(int connector_id, std::unique_ptr<Request> &&request) noexcept;

  int create_external_db_response_net_event(std::unique_ptr<Response> &&response) noexcept;
  void process_external_db_response_event(std::unique_ptr<Response> &&response) noexcept;

  void start_request_processing(int request_id, RequestInfo &&request_info) noexcept;
  int finish_request_processing(int request_id, std::unique_ptr<Response> &&response) noexcept;
  Response *withdraw_response(int request_id) noexcept;
private:
//  using RequestMapT = memory_resource::stl::map<int, RequestInfo, memory_resource::unsynchronized_pool_resource>;

  ConnectorsContainer connectors;
  std::unordered_map<int, RequestInfo> processing_requests;

  NetDriversAdaptor() = default;

  friend class vk::singleton<NetDriversAdaptor>;
};
