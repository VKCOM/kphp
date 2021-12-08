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
  void send_metrics(const std::vector<StatsHouseMetric> &metrics);

private:
  StatsHouseClient() = default;
  ~StatsHouseClient();
  friend class vk::singleton<StatsHouseClient>;

  bool init_connection();

  int port = 0;
  std::string host;
  int sock_fd = 0;
  constexpr static int BUFFER_LEN = 16000;
  char buffer[BUFFER_LEN];
};
