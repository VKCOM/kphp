// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>
#include <unordered_map>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "runtime-common/core/runtime-core.h"
#include "net/net-events.h"
#include "runtime/critical_section.h"
#include "runtime/signal_safe_hashtable.h"

#include "server/database-drivers/connector.h"
#include "server/database-drivers/request.h"
#include "server/database-drivers/response.h"
#include "server/workers-control.h"

// region database_drivers::Adaptor friends related declarations
class PhpWorker;
struct net_event_t;
struct external_driver_connect;
void generic_event_loop(WorkerType, bool) noexcept;
void php_worker_run_net_queue(PhpWorker *);
bool process_net_event(net_event_t *);
// endregion

namespace database_drivers {

class Connector;
class Request;
class Response;

struct RequestInfo {
  int resumable_id{0};
  std::unique_ptr<Response> response;
};

class Adaptor : vk::not_copyable {
public:
  /**
   * @brief Registers @a connector and embeds it into event loop.
   * @param connector
   * @return ID of registered connector.
   */
  int initiate_connect(std::unique_ptr<Connector> &&connector) noexcept;

  /**
   * @brief Launches @a request resumable (coroutine), that will send @a request asynchronously.
   * @param request
   * @return ID of launched resumable (coroutine), which can be awaited with wait_request_resumable().
   *
   * Creates forked resumable as if we call `fork(launch_request_resumable())`, @see fork_resumable().
   * Moves @a request to Connector::push_async_request().
   */
  int launch_request_resumable(std::unique_ptr<Request> &&request) noexcept;

  /**
   * @brief Finishes request resumable (coroutine), with @a response.
   * @param response
   *
   * Moves @a response through event loop related storages to return value of corresponding resumable (coroutine).
   * Wakes up worker process to make it handle new net response event and continue corresponding resumable, @see resumable_run_ready().
   */
  void finish_request_resumable(std::unique_ptr<Response> &&response) noexcept;

  /**
   * @brief Waits request resumable with @a resumable_id. Waits no longer than @a timeout in seconds.
   * @param resumable_id
   * @param timeout
   * @return Response for Request associated with @a resumable_id.
   */
  std::unique_ptr<Response> wait_request_resumable(int resumable_id, double timeout = -1.0) const noexcept;

  /**
   * @brief Gets connector of given @a ConnectorType by @a connector_id.
   * @tparam ConnectorType
   * @param connection_id
   * @return Pointer to wanted connector.
   */
  template<typename ConnectorType>
  ConnectorType *get_connector(int connector_id) noexcept;

  /**
   * @brief Removes and closes connector by @a connector_id.
   * @param connector_id
   */
  void remove_connector(int connector_id) noexcept;

  void reset() noexcept;

private:
  SignalSafeHashtable<int, std::unique_ptr<Connector>> connectors;
  SignalSafeHashtable<int, RequestInfo> processing_requests;

  static int epoll_gateway(int fd, void *data, event_t *ev) noexcept;

  Adaptor() = default;

  int register_connector(std::unique_ptr<Connector> &&connector) noexcept;
  friend struct ::external_driver_connect;

  void create_outbound_connections() noexcept;
  friend void ::generic_event_loop(WorkerType, bool) noexcept;

  void process_external_db_request_net_query(std::unique_ptr<Request> &&request) noexcept;
  friend void ::php_worker_run_net_queue(PhpWorker *);

  void process_external_db_response_event(std::unique_ptr<Response> &&response) noexcept;
  friend bool ::process_net_event(net_event_t *);

  void start_request_processing(int request_id, RequestInfo &&request_info) noexcept;
  int finish_request_processing(int request_id, std::unique_ptr<Response> &&response) noexcept;
  std::unique_ptr<Response> withdraw_response(int request_id) noexcept;

  friend class ::vk::singleton<database_drivers::Adaptor>;
  friend class Connector;
  class RequestResumable;
};

template<typename ConnectorType>
ConnectorType *Adaptor::get_connector(int connector_id) noexcept {
  auto *ptr = connectors.get(connector_id);
  if (ptr == nullptr) {
    php_warning("Can't find connector with id %d. Probably the connection is closed already", connector_id);
    return nullptr;
  }
  return dynamic_cast<ConnectorType *>(ptr->get());
}

} // namespace database_drivers
