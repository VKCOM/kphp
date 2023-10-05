// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/statshouse/statshouse-client.h"

statshouse::TransportUDPBase::MetricBuilder StatsHouseClient::metric(std::string_view name, bool force_tag_host) {
  auto builder = transport.metric(name);
  builder.tag(tag_cluster);
  if (host_enabled || force_tag_host) {
    builder.tag("host", tag_host);
  }
  return builder;
}
