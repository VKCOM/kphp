// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>

#include "common/mixin/not_copyable.h"
#include "runtime/allocator.h"

class Request;

class Connector : public ManagedThroughDlAllocator, public vk::not_copyable {
public:
  int connector_id{};

  virtual ~Connector() noexcept = 0;

  virtual int get_fd() const noexcept = 0;
  virtual void handle_read() noexcept = 0;
  virtual void handle_write() noexcept = 0;
  virtual void push_async_request(std::unique_ptr<Request> &&req) noexcept = 0;

  bool connected() const noexcept;
  bool connect_async() noexcept;
protected:
  bool is_connected{};
  bool ready_to_read{};
  bool ready_to_write{};

  virtual bool connect_async_impl() noexcept = 0;

  void update_state_ready_to_write();

  void update_state_ready_to_read();

  void update_state_idle();

private:
  void update_state_in_reactor() const noexcept;
};
