// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "third-party/statshouse.h"

class StatsHouseClient {
public:
  // Safe to use dummy instance
  StatsHouseClient() : transport({}, {}) {}

  StatsHouseClient(const std::string &ip, int port) : transport(ip, port) {}

  statshouse::TransportUDPBase::MetricBuilder metric(std::string_view name, bool force_tag_host = false);

  void set_environment(const std::string &env) {
    transport.set_default_env(env);
  }

  void set_tag_cluster(std::string_view cluster) {
    tag_cluster = cluster;
  }

  void set_tag_host(std::string_view host) {
    tag_host = host;
  }

  void enable_tag_host() {
    host_enabled = true;
  }

  void disable_tag_host() {
    host_enabled = false;
  }

private:
  statshouse::TransportUDP transport;
  std::string tag_cluster;
  std::string tag_host;
  bool host_enabled{false};
};
