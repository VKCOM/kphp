// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-dc.h"

#include <assert.h>

#include "common/kprintf.h"
#include "common/options.h"

#include "net/net-connections.h"
#include "net/net-ifnet.h"
#include "net/net-sockaddr-storage.h"

static bool force_net_encryption;
FLAG_OPTION_PARSER(OPT_NETWORK, "force-net-encryption", force_net_encryption, "forces encryption for client network activity (even on localhost and same data-center)");


struct dc_ip_and_mask {
  uint32_t ip;
  uint32_t mask;
  int dc_index;

  dc_ip_and_mask() = default;

  int init_from_string(const char *index_ipv4_subnet) {   // "8=1.2.3.4/12"
    char *p_end;
    dc_index = static_cast<int>(strtol(index_ipv4_subnet, &p_end, 10));
    if (p_end == nullptr || *p_end != '=') {
      kprintf("Failed to parse dc_ip_and_mask, expected format '8=1.2.3.4/12', got %s\n", index_ipv4_subnet);
      return -1;
    }
    p_end++;  // move after '=', wait for "ipv4/subnet" now
    return parse_ipv4(p_end, &ip, &mask);
  }
};

static constexpr int MAX_DC_CONFIGS = 24;

static dc_ip_and_mask dc_config[MAX_DC_CONFIGS];
static int dc_config_count = 0;

bool add_dc_by_ipv4_config(const char *index_ipv4_subnet) {
  if (dc_config_count >= MAX_DC_CONFIGS - 1) {
    kprintf("Maximum size of dc configs reached, skipping");
    return false;
  }
  return 0 == dc_config[dc_config_count++].init_from_string(index_ipv4_subnet);
}

__attribute__((constructor))
static void add_dc_127001_no_aes() {
  add_dc_by_ipv4_config("99=127.0.0.0/8");
}

static bool check(uint32_t address, const dc_ip_and_mask &ip) {
  return ((address ^ ip.ip) & ip.mask) == 0;
}

static inline int get_dc_by_ipv4(uint32_t address) {
  for (int i = 0; i < dc_config_count; ++i) {
    if (check(address, dc_config[i])) {
      return dc_config[i].dc_index;
    }
  }

  return -1;
}

int is_same_data_center(struct connection *c, int is_client) {
  if (is_client && force_net_encryption) {
    return 0;
  }

  char buffer_local[SOCKADDR_STORAGE_BUFFER_SIZE], buffer_remote[SOCKADDR_STORAGE_BUFFER_SIZE];
  vkprintf(4, "check_same_data_center(%d): %s -> %s\n", c->fd, sockaddr_storage_to_buffer(&c->local_endpoint, buffer_local),
           sockaddr_storage_to_buffer(&c->remote_endpoint, buffer_remote));
  if (c->flags & C_IPV6) { // we don't use it now
    assert(c->local_endpoint.ss_family == AF_INET6);
    return 0;
  } else if (c->flags & C_UNIX) {
    assert(c->local_endpoint.ss_family == AF_UNIX);
    return 1;
  } else {
    assert(c->local_endpoint.ss_family == AF_INET);

    const uint32_t remote_ip = inet_sockaddr_address(&c->remote_endpoint);
    const uint32_t local_ip = inet_sockaddr_address(&c->local_endpoint);

    const int remote_dc = get_dc_by_ipv4(remote_ip);
    const int local_dc = get_dc_by_ipv4(local_ip);

    return remote_dc != -1 && local_dc != -1 && remote_dc == local_dc;
  }
}
