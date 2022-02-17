// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>

#include "common/mixin/not_copyable.h"
#include "runtime/allocator.h"
#include "server/database-drivers/async-operation-status.h"

namespace database_drivers {

class Request;

class Connector : public ManagedThroughDlAllocator, public vk::not_copyable {
public:
  int connector_id{};

  /**
   * @brief Closes underlying connection, removes fd from epoll and clean up other resources.
   */
  virtual ~Connector() noexcept = 0;

  /**
   * @brief Gets file descriptor number of underlying connection.
   * @return File descriptor number.
   */
  virtual int get_fd() const noexcept = 0;

  /**
   * @brief Attempts to establish underlying connection asynchronously.
   * @return Status of operation: in progress, completed or error.
   */
  virtual AsyncOperationStatus connect_async() noexcept = 0;

  /**
   * @brief Registers outgoing request to send it later when underlying connection will be ready for write.
   * @param request
   *
   * Usually it puts @a request to pending requests queue.
   */
  virtual void push_async_request(std::unique_ptr<Request> &&request) noexcept = 0;

  /**
   * @brief Performs necessary actions when underlying connection is ready for write, @see EPOLLOUT.
   *
   * Usually it sends all pending requests and removes completely sent requests from queue.
   */
  virtual void handle_write() noexcept = 0;

  /**
   * @brief Performs necessary actions when underlying connection is ready for read, @see EPOLLIN.
   *
   * Usually it reads all available responses and finishes corresponding resumables, @see database_drivers::Adaptor::finish_request_resumable().
   */
  virtual void handle_read() noexcept = 0;

  /**
   * @brief Performs necessary actions when special events happened on underlying connection , @see EPOLLRDHUP, EPOLLPRI.
   *
   * For example, special events are: server closed the connection, TCP out-of-band data came on socket
   */
  virtual void handle_special() noexcept = 0;

  bool connected() const noexcept;

protected:
  bool is_connected{};
  bool ready_to_read{};
  bool ready_to_write{};

  void update_state_ready_to_write();

  void update_state_ready_to_read();

  void update_state_idle();

private:
  AsyncOperationStatus connect_async_and_epoll_insert() noexcept;
  void update_state_in_reactor() const noexcept;

  friend class Adaptor;
};

} // namespace database_drivers
