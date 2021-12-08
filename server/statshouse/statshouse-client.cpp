// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/statshouse/statshouse-client.h"

#include <fcntl.h>
#include <unistd.h>

#include "common/resolver.h"
#include "common/tl/constants/statshouse.h"
#include "common/tl/methods/string.h"
#include "net/net-connections.h"
#include "runtime/critical_section.h"
#include "server/server-log.h"

void StatsHouseClient::set_port(int value) {
  this->port = value;
}

void StatsHouseClient::set_host(std::string value) {
  this->host = std::move(value);
}

bool StatsHouseClient::init_connection() {
  if (sock_fd <= 0) {
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
      log_server_error("Can't create statshouse socket");
      return false;
    }
  }
  fcntl(sock_fd, F_SETFL, O_NONBLOCK);

  hostent *h;
  std::string hostname = host.empty() ? "localhost" : host;
  if (!(h = gethostbyname(hostname.c_str())) || h->h_addrtype != AF_INET || h->h_length != 4 || !h->h_addr_list || !h->h_addr) {
    log_server_error("Can't resolve statshouse host: %s", host.c_str());
    return false;
  }
  struct sockaddr_in addr {};
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = (*reinterpret_cast<uint32_t *>(h->h_addr));
  if (connect(sock_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
    log_server_error("Can't connect to statshouse host: %s", hostname.c_str());
    return false;
  }
  return true;
}

void StatsHouseClient::send_metrics(const std::vector<StatsHouseMetric> &metrics) {
  if (port == 0 || (sock_fd <= 0 && !init_connection())) {
    return;
  }
  StatsHouseAddMetricsBatch metrics_batch = {.fields_mask = vk::tl::statshouse::add_metrics_batch_fields_mask::ALL, .metrics = metrics};
  int stored_size = vk::tl::store_to_buffer(buffer, BUFFER_LEN, metrics_batch);
  assert(stored_size < BUFFER_LEN);

  ssize_t len = send(sock_fd, buffer, stored_size, 0);
  if (len < 0) {
    log_server_error("Can't send metrics to statshouse: %s", strerror(errno));
  }
}

StatsHouseClient::~StatsHouseClient() {
  if (sock_fd > 0) {
    close(sock_fd);
  }
}
