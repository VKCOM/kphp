// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "server/external-net-drivers/net-drivers-adaptor.h"

class Request;

class Connector : public ManagedThroughDlAllocator {
public:
  virtual int get_fd() const noexcept = 0;
  virtual void handle_read() noexcept = 0;
  virtual void handle_write() noexcept = 0;
  virtual void push_async_request(Request *req) noexcept = 0;

  virtual void close() noexcept = 0;

  bool connected() const noexcept;
  bool connect_async() noexcept;

protected:
  bool is_connected{};
  bool ready_to_read{};
  bool ready_to_write{};

  virtual bool connect_async_impl() noexcept = 0;

  void update_state_ready_to_write() {
    fprintf(stderr, "@@@@@@@ update_state_ready_to_write\n");
    ready_to_write = true;
    ready_to_read = false;
    update_state_in_reactor();
  }

  void update_state_ready_to_read() {
    fprintf(stderr, "@@@@@@@ update_state_ready_to_read\n");
    ready_to_read = true;
    ready_to_write = false;
    update_state_in_reactor();
  }

  void update_state_idle() {
    fprintf(stderr, "@@@@@@@ update_state_idle\n");
    ready_to_write = false;
    ready_to_read = false;
    update_state_in_reactor();
  }

private:
  void update_state_in_reactor() const noexcept {
    int action_flags = 0;
    if (ready_to_read) {
      action_flags |= EVT_READ;
    }
    if (ready_to_write) {
      action_flags |= EVT_WRITE;
    }
    epoll_insert(get_fd(), EVT_SPEC | EVT_LEVEL | action_flags);
  }
};
