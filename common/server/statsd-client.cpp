#include "common/server/statsd-client.h"

#include "common/binlog/kdb-binlog-common.h"
#include "common/kfs/kfs.h"
#include "net/net-connections.h"
#include "net/net-crypto-aes.h"

#include "common/options.h"
#include "common/resolver.h"
#include "common/server/engine-settings.h"
#include "common/server/stats.h"

#define STATSD_PORTS_CNT_DEFAULT (2)
#define STATSD_PORTS_CNT_MAX (3)
static int statsd_ports[STATSD_PORTS_CNT_MAX] = {8125, 14880, -1};
static int statsd_ports_cnt = STATSD_PORTS_CNT_DEFAULT;
static conn_target_t *statsd_targets[STATSD_PORTS_CNT_MAX];
static int connected_to_targets[STATSD_PORTS_CNT_MAX];
static int disable_statsd;
static const char *statsd_host;

SAVE_STRING_OPTION_PARSER(OPT_GENERIC, "statsd-host", statsd_host, "engine will connect to statsd daemon at this host and send stats to it");

OPTION_PARSER(OPT_GENERIC, "statsd-port", required_argument, "engine will connect to statsd daemon at this port and send stats to it") {
  assert(statsd_ports_cnt < STATSD_PORTS_CNT_MAX);
  if (sscanf(optarg, "%d", &statsd_ports[statsd_ports_cnt]) != 1) {
    return -1;
  }
  statsd_ports_cnt++;
  return 0;
}

FLAG_OPTION_PARSER(OPT_GENERIC, "disable-statsd", disable_statsd, "disable sending stats to default statsd ports");

static int statsd_parse_execute(struct connection *c) {
  netbuffer_t *in = &c->In;
  int len;
  while ((len = get_ready_bytes(in)) > 0) {
    advance_skip_read_ptr(in, len);
  }
  return 0;
}

conn_type_t ct_statsd_client = [] {
  auto result = conn_type_t();
  result.magic = CONN_FUNC_MAGIC;
  result.title = "statsd_client";
  result.accept = server_failed;
  result.init_accepted = server_failed;
  result.run = server_read_write;
  result.reader = server_reader;
  result.writer = server_writer;
  result.parse_execute = statsd_parse_execute;
  result.close = client_close_connection;
  result.free_buffers = free_connection_buffers;
  result.init_outbound = server_noop;
  result.connected = server_noop;
  result.wakeup = server_noop;
  result.check_ready = server_check_ready;
  result.flush = server_noop;
  result.crypto_init = aes_crypto_init;
  result.crypto_free = aes_crypto_free;
  result.crypto_encrypt_output = aes_crypto_encrypt_output;
  result.crypto_decrypt_input = aes_crypto_decrypt_input;
  result.crypto_needed_output_bytes = aes_crypto_needed_output_bytes;
  return result;
}();

static void init_statsd_targets() {
  static int inited = 0;
  if (inited) {
    return;
  }
  inited = 1;

  for (int i = 0; i < statsd_ports_cnt; i++) {
    int port = statsd_ports[i];
    struct hostent *h;
    const char *hostname = statsd_host ?: "localhost";
    if (!(h = kdb_gethostbyname(hostname)) || h->h_addrtype != AF_INET || h->h_length != 4 || !h->h_addr_list || !h->h_addr) {
      kprintf("cannot resolve %s\n", hostname);
      exit(2);
    }
    static conn_target_t statsd_ct;
    statsd_ct.min_connections = 1;
    statsd_ct.max_connections = 1;
    statsd_ct.type = &ct_statsd_client;
    statsd_ct.extra = NULL;
    statsd_ct.reconnect_timeout = 17;
    statsd_ct.endpoint = make_inet_sockaddr_storage(ntohl(*(uint32_t *)(h->h_addr)), (uint16_t)port);
    statsd_targets[i] = create_target(&statsd_ct, 0);
  }
}

void send_data_to_statsd_with_prefix(const char *stats_prefix, unsigned int tag_mask) {
  if (disable_statsd) {
    vkprintf(2, "Not sending stats to statsd, because it is disabled\n");
    return;
  }
  vkprintf(3, "Send data to statsd\n");
  init_statsd_targets();

  for (int i = statsd_ports_cnt - 1; i >= 0; i--) {
    struct connection *c = get_default_target_connection(statsd_targets[i]);
    if (c == NULL) {
      if (connected_to_targets[i] == 1) {
        kprintf("Can't create connection to statsd at %s\n", sockaddr_storage_to_string(&statsd_targets[i]->endpoint));
        connected_to_targets[i] = 0;
      }
      continue;
    } else if (connected_to_targets[i] == 0) {
      kprintf("Connected to statsd at %s\n", sockaddr_storage_to_string(&statsd_targets[i]->endpoint));
      connected_to_targets[i] = 1;
    }

    char *result = engine_default_prepare_stats_with_tag_mask(STATS_TYPE_STATSD, NULL, stats_prefix, tag_mask);
    write_out(&c->Out, result, (int)strlen(result));
    flush_later(c);
    vkprintf(4, "statsd flush!\n");
    break;
  }
}

