// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/statshouse/statshouse-client.h"

void StatsHouseClient::init_common_tags(std::string_view cluster, std::string_view host) {
  tag_cluster = cluster;
  tag_host = host;
}

statshouse::TransportUDPBase::MetricBuilder StatsHouseClient::metric(std::string_view name) {
  auto builder = transport.metric(name);
  builder.tag(tag_cluster);
  if (host_enabled) {
    builder.tag("host", tag_host);
  }
  return builder;
}
