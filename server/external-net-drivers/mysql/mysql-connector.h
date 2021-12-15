// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <mysql/mysql.h>

#include "runtime/kphp_core.h"
#include "server/external-net-drivers/connector.h"

class Request;

class MysqlConnector : public Connector {
public:
  MYSQL *ctx{};

  MysqlConnector(MYSQL *ctx, string host, string user, string password, string db_name, int port)
    : Connector()
    , ctx(ctx)
    , host(std::move(host))
    , user(std::move(user))
    , password(std::move(password))
    , db_name(std::move(db_name))
    , port(port) {}

  void close() noexcept final;
  int get_fd() const noexcept final;

  void push_async_request(Request *req) noexcept final;

private:
  string host{};
  string user{};
  string password{};
  string db_name{};
  int port{};

  Request *pending_request{};
  Response *pending_response{};

  bool connect_async_impl() noexcept final;
  void handle_read() noexcept override;
  void handle_write() noexcept override;
};
