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
  virtual void push_async_request(int request_id, Request *req) noexcept = 0;

  virtual void close() noexcept = 0;

  bool connected() const noexcept;
  bool connect_async() noexcept;
protected:
  bool is_connected{};

  virtual bool connect_async_impl() noexcept = 0;
};
