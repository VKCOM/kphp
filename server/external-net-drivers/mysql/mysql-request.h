// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"
#include "server/external-net-drivers/request.h"


class MysqlConnector;

class MysqlRequest : public Request {
public:
  MysqlRequest(MysqlConnector *connector, const string &request);

  bool send_async() noexcept final;
private:
  MysqlConnector *connector;
  string request;
};
