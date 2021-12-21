// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/statshouse/statshouse-client.h"

#include <fcntl.h>
#include <unistd.h>

#include "common/resolver.h"
#include "common/server/stats.h"
#include "common/stats/provider.h"
#include "common/tl/constants/statshouse.h"
#include "common/tl/methods/string.h"
#include "net/net-connections.h"
#include "runtime/critical_section.h"
#include "server/server-log.h"

class statshouse_stats_t : public stats_t {
public:
  statshouse_stats_t(const std::vector<std::pair<std::string, std::string>> &tags)
    : tags(tags) {}

  void add_general_stat(const char *, const char *, va_list) noexcept final {
    // ignore it
  }

  void add_stat(char type [[maybe_unused]], const char *key, const char *value_format [[maybe_unused]], double value) noexcept final {
    auto metric = make_statshouse_value_metric(normalize_key(key, "%s_%s", stats_prefix), value, tags);
    auto len = vk::tl::store_to_buffer(sb.buff + sb.pos, sb.size, metric);
    sb.pos += len;
    ++counter;
  }

  void add_stat(char type, const char *key, const char *value_format, long long value) noexcept final {
    add_stat(type, key, value_format, static_cast<double>(value));
  }

  virtual bool needAggrStats() noexcept final {
    return false;
  }

  int get_counter() const {
    return counter;
  }

private:
  int counter{0};
  const std::vector<std::pair<std::string, std::string>> &tags;
};

namespace {
std::pair<char *, int> prepare_statshouse_stats(statshouse_stats_t &&stats, const char *statsd_prefix, unsigned int tag_mask) {
  stats.stats_prefix = statsd_prefix;

  char *buf = get_engine_default_prepare_stats_buffer();

  sb_init(&stats.sb, buf, STATS_BUFFER_LEN);
  constexpr int offset = 3 * 4; // for magic, fields_mask and vector size
  stats.sb.pos = offset;
  prepare_common_stats_with_tag_mask(&stats, tag_mask);

  auto metrics_batch = StatsHouseAddMetricsBatch{.fields_mask = vk::tl::statshouse::add_metrics_batch_fields_mask::ALL, .metrics_size = stats.get_counter()};
  vk::tl::store_to_buffer(stats.sb.buff, offset, metrics_batch);
  return {buf, stats.sb.pos};
}
} // namespace

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

void StatsHouseClient::send_metrics() {
  if (port == 0 || (sock_fd <= 0 && !init_connection())) {
    return;
  }

  auto [result, len] = prepare_statshouse_stats(statshouse_stats_t{tags}, "kphp", STATS_TAG_KPHP_SERVER);

  ssize_t slen = send(sock_fd, result, len, 0);
  if (slen < 0) {
    log_server_error("Can't send metrics to statshouse (len = %i): %s", len, strerror(errno));
  }
}

StatsHouseClient::StatsHouseClient() {
  char hostname[128];
  int res = gethostname(hostname, 128);
  if (res == -1) {
    log_server_error("Can't gethostname for statshouse metrics: %s", strerror(errno));
  }
  tags = {{"host", std::string(hostname)}};
}

StatsHouseClient::~StatsHouseClient() {
  if (sock_fd > 0) {
    close(sock_fd);
  }
}
