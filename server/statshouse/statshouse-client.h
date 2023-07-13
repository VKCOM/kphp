// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "server/statshouse/add-metrics-batch.h"

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"

class StatsHouseClient : vk::not_copyable {
public:
  void set_port(int value);
  void set_host(std::string value);
  /**
   * Must be called from master process only
   */
  void master_send_metrics();
  void send_metrics(char* result, int len);
private:
  int port = 0;
  int sock_fd = 0;
  std::string host;

  StatsHouseClient();
  ~StatsHouseClient();

  bool init_connection();

  friend class vk::singleton<StatsHouseClient>;
};
