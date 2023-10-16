// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __VK_NET_CONNECTIONS_H__
#define __VK_NET_CONNECTIONS_H__

#include <assert.h>
#include <stddef.h>
#include <sys/socket.h>

#include "common/macos-ports.h"
#include "common/kprintf.h"

#include "net/net-buffers.h"
#include "net/net-events.h"
#include "net/net-msg.h"
#include "net/net-sockaddr-storage.h"

DECLARE_VERBOSITY(net_connections);

#define MAX_TARGETS 65536
#define PRIME_TARGETS 99961

#define BUFF_SIZE 2048

#define CONN_CUSTOM_DATA_BYTES 128

#define NEED_MORE_BYTES (0x7fffffff)
#define SKIP_ALL_BYTES (0x80000000)

/* for connection flags */
#define C_WANTRD    0x00000001
#define C_WANTWR    0x00000002
#define C_WANTRW    (C_WANTRD | C_WANTWR)
#define C_INCONN    0x00000004
#define C_ERROR     0x00000008
#define C_NORD      0x00000010
#define C_NOWR      0x00000020
#define C_NORW      (C_NORD | C_NOWR)
#define C_INQUERY   0x00000040
#define C_FAILED    0x00000080
#define C_ALARM     0x00000100
#define C_AIO       0x00000200
#define C_INTIMEOUT 0x00000400
#define C_STOPREAD  0x00000800
#define C_REPARSE   0x00001000
#define C_DFLUSH    0x00002000
#define C_IPV6      0x00004000
#define C_FAKE      0x00008000
#define C_SPECIAL   0x00010000
#define C_NOQACK    0x00020000
#define C_RAWMSG    0x00040000
#define C_UNIX      0x00080000
#define C_CRYPTOIN  0x00100000
#define C_CRYPTOOUT 0x00200000

#define C_PERMANENT (C_IPV6 | C_RAWMSG | C_UNIX)

/* for connection status */
enum {
  conn_none,           // closed/uninitialized
  conn_expect_query,   // wait for inbound query, no query bytes read
  conn_reading_query,  // query partially read, wait for more bytes
  conn_running,        // running a query, almost never appears
  conn_wait_aio,       // waiting for aio completion (while running a query)
  conn_wait_net,       // waiting for network (while running a query)
  conn_connecting,     // outbound socket, connecting to the other side
  conn_ready,          // outbound socket ready
  conn_sending_query,  // sending a query to outbound socket, almost never appears
  conn_wait_answer,    // waiting for an answer from an outbound socket
  conn_reading_answer, // answer partially read, waiting for more bytes
  conn_error,          // connection in bad state (it will be probably closed)
  conn_wait_timeout,   // connection was in wait_aio/wait_net states, and it timed out
  conn_listen,         // listening for inbound connections
  conn_write_close,    // write all output buffer, then close; don't read input
  conn_total_states    // total number of connection states
};

/* for connection basic_type */
enum {
  ct_none,     // no connection (closed)
  ct_listen,   // listening socket
  ct_inbound,  // inbound connection
  ct_outbound, // outbound connection
  ct_aio,      // used for aio file operations ( net-aio.h )
  ct_pipe,     // used for pipe reading
  ct_job       // used for async jobs ( net-jobs.h )
};

/* for connection->ready of outbound connections */
enum {
  cr_notyet,  // not ready yet (e.g. logging in)
  cr_ok,      // working
  cr_stopped, // stopped (don't send more queries)
  cr_busy,    // busy (sending queries not allowed by protocol)
  cr_failed   // failed (possibly timed out)
};

struct connection;

/* connection function table */

#define CONN_FUNC_MAGIC 0x11ef55aa

typedef struct conn_functions {
  int magic;
  int flags; /* may contain for example C_RAWMSG; (partially) inherited by inbound/outbound connections */
  const char *title;
  int (*accept)(struct connection *c);         /* invoked for listen/accept connections of this type */
  int (*init_accepted)(struct connection *c);  /* initialize a new accept()'ed connection */
  int (*create_outbound)(const struct sockaddr_storage *endpoint);  /* creates new outbound fd */
  int (*run)(struct connection *c);            /* invoked when an event related to connection of this type occurs */
  int (*reader)(struct connection *c);         /* invoked from run() for reading network data */
  int (*writer)(struct connection *c);         /* invoked from run() for writing data */
  int (*close)(struct connection *c, int who); /* invoked from run() whenever we need to close connection */
  int (*free_buffers)(struct connection *c);   /* invoked from close() to free all buffers */
  int (*parse_execute)(struct connection *c);  /* invoked from reader() for parsing and executing one query */
  int (*init_outbound)(struct connection *c);  /* initializes newly created outbound connection */
  int (*connected)(struct connection *c);      /* invoked from run() when outbound connection is established */
  int (*wakeup)(struct connection *c);         /* invoked from run() when pending_queries == 0 */
  int (*alarm)(struct connection *c);          /* invoked when timer is out */
  int (*ready_to_write)(struct connection *c); /* invoked from server_writer when Out.total_bytes crosses write_low_watermark ("greater or equal" -> "less") */
  int (*check_ready)(struct connection *c);    /* updates conn->ready if necessary and returns it */
  int (*wakeup_aio)(struct connection *c, int r);    /* invoked from net_aio.c::check_aio_completion when aio read operation is complete */
  int (*data_received)(struct connection *c, int r); /* invoked after r>0 bytes are read from socket */
  int (*data_sent)(struct connection *c, int w);     /* invoked after w>0 bytes are written into socket */
  void (*ancillary_data_received)(struct connection *c, const struct cmsghdr* cmsg);
  int (*flush)(struct connection *c);                /* generates necessary padding and writes as much bytes as possible */
  int (*crypto_init)(struct connection *c, void *key_data, int key_data_len); /* < 0 = error */
  int (*crypto_free)(struct connection *c);
  int (*crypto_encrypt_output)(struct connection *c);      /* 0 = all ok, >0 = so much more bytes needed to encrypt last block */
  int (*crypto_decrypt_input)(struct connection *c);       /* 0 = all ok, >0 = so much more bytes needed to decrypt last block */
  int (*crypto_needed_output_bytes)(struct connection *c); /* returns # of bytes needed to complete last output block */
} conn_type_t;

struct conn_target {
  int min_connections;
  int max_connections;
  struct connection *first_conn, *last_conn;
  struct conn_query *first_query, *last_query;
  conn_type_t *type;
  void *extra;
  int generation;
  struct sockaddr_storage endpoint;
  int active_outbound_connections, outbound_connections;
  int ready_outbound_connections;
  double next_reconnect, reconnect_timeout, next_reconnect_timeout;
  int custom_field;
  int refcnt;
  struct conn_target *next_target, *prev_target;
  struct conn_target *hnext;
  void *extra_ptr1, *extra_ptr2;
};
typedef struct conn_target conn_target_t;

#define CQUERY_FUNC_MAGIC 0xDEADBEEF
struct conn_query;

typedef struct conn_query_functions {
  int magic;
  const char *title;
  int (*parse_execute)(struct connection *c);
  int (*close)(struct conn_query *q);
  int (*wakeup)(struct conn_query *q);
  int (*complete)(struct conn_query *q);
} conn_query_type_t;

struct conn_query {
  int custom_type;
  int req_generation;
  struct connection *outbound;
  struct connection *requester;
  struct conn_query *next, *prev;
  conn_query_type_t *cq_type;
  void *extra;
  double start_time;
  event_timer_t timer;
};

struct connection {
  int fd;
  int flags;
  struct connection *next, *prev;
  struct conn_query *first_query, *last_query;
  conn_type_t *type;
  event_t *ev;
  void *extra;
  conn_target_t *target;
  int basic_type;
  int status;
  int error;
  int generation;
  int unread_res_bytes;
  int skip_bytes;
  int pending_queries;
  int queries_ok;
  char custom_data[CONN_CUSTOM_DATA_BYTES];
  struct sockaddr_storage local_endpoint;
  struct sockaddr_storage remote_endpoint;
  double query_start_time;
  double last_query_time;
  double last_query_sent_time;
  double last_response_time;
  double last_query_timeout;
  event_timer_t timer;
  event_timer_t write_timer;
  int limit_per_write, limit_per_sec;
  int last_write_time, written_per_sec;
  int unreliability;
  int ready;
  int parse_state;
  int write_low_watermark;
  void *crypto;
  int listening, listening_generation;
  int window_clamp;
  int eagain_count;
  raw_message_t in_u, in, out, out_p;
  nb_iterator_t Q;
  netbuffer_t *Tmp, In, Out;
  char in_buff[BUFF_SIZE];
  char out_buff[BUFF_SIZE];
  struct ucred credentials;
  bool interrupted;
  bool ignored;
};
typedef struct connection connection_t;

static_assert(offsetof(struct connection, first_query) == offsetof(struct conn_query, next), "checkFirstQuery");
static_assert(offsetof(struct connection, last_query) == offsetof(struct conn_query, prev), "checkLastQuery");
static_assert(offsetof(conn_target_t, first_query) == offsetof(struct conn_query, next), "checkFirstQueryTarget");
static_assert(offsetof(conn_target_t, last_query) == offsetof(struct conn_query, prev), "checkLastQueryTarget");
static_assert(offsetof(conn_target_t, first_conn) == offsetof(struct connection, next), "checkFirstConnTarget");
static_assert(offsetof(conn_target_t, last_conn) == offsetof(struct connection, prev), "checkLastConnTarget");

extern struct connection *Connections;

extern int max_connection;
extern int active_connections;
extern int active_special_connections, max_special_connections;
extern int outbound_connections, active_outbound_connections, ready_outbound_connections;
extern int conn_generation;
extern int ready_targets;
extern long long total_failed_connections, total_connect_failures, unused_connections_closed;
extern const char *unix_socket_directory;

void set_on_active_special_connections_update_callback(void (*callback)(bool on_accept)) noexcept;

int init_listening_connection_mode(int fd, conn_type_t *type, void *extra, int mode);

static inline int init_listening_connection(int fd, conn_type_t *type, void *extra) {
  return init_listening_connection_mode(fd, type, extra, 0);
}

static inline int init_listening_tcpv6_connection(int fd, conn_type_t *type, void *extra, int mode) {
  return init_listening_connection_mode(fd, type, extra, mode);
}

connection_t* lock_fake_connection();
void unlock_fake_connection(connection_t *c);


/* default methods */
int accept_new_connections(struct connection *c);
int server_read_write(struct connection *c);
int server_reader(struct connection *c);
int server_writer(struct connection *c);
int server_noop(struct connection *c);
int server_noop(struct connection *c, int who);
int server_failed(struct connection *c);
int server_failed(struct connection *c, int who);

int server_close_connection(struct connection *c, int who);
int client_close_connection(struct connection *c, int who);
int free_connection_buffers(struct connection *c);
int default_parse_execute(struct connection *c); // DO NOT USE!!!
int client_init_outbound(struct connection *c);
int server_check_ready(struct connection *c);
void ancillary_data_received(struct connection *c, const struct cmsghdr *cmsg);

int conn_timer_wakeup_gateway(event_timer_t *et);
int conn_write_timer_wakeup_gateway(event_timer_t *et);
int server_read_write_gateway(int fd, void *data, event_t *ev);
int check_conn_functions(conn_type_t *type);

/* useful functions */

int fail_connection(struct connection *c, int err);
int flush_connection_output(struct connection *c);
int flush_later(struct connection *c);

int set_connection_timeout(struct connection *c, double timeout);
int clear_connection_timeout(struct connection *c);

void dump_connection_buffers(struct connection *c);

int conn_rerun_later(struct connection *c, int ts_delta);

/* target handling */

extern conn_target_t Targets[MAX_TARGETS];
extern int allocated_targets, active_targets, inactive_targets, free_targets;

/* `was_created` return value (if non-zero pointer passed):
   1 = created new target (with all fields copied from `source`),
   2 = refcnt changed from 0 to 1 (often logically equivalent to case 1),
   0 = existed before with refcnt > 0.
*/
conn_target_t *create_target(const conn_target_t *source, int *was_created);
int destroy_target(conn_target_t *S);
int clean_unused_target(conn_target_t *S);
struct connection *get_default_target_connection(conn_target_t *target);

int create_new_connections(conn_target_t *S);
int create_all_outbound_connections();
void install_client_connection(conn_target_t *S, int fd);

int force_clear_connection(struct connection *c);
void net_reset_after_fork();

/* connection query queues */
int insert_conn_query(struct conn_query *q);
int push_conn_query(struct conn_query *q);
int insert_conn_query_into_list(struct conn_query *q, struct conn_query *h);
int push_conn_query_into_list(struct conn_query *q, struct conn_query *h);
int delete_conn_query(struct conn_query *q);
int delete_conn_query_from_requester(struct conn_query *q);

extern long long netw_queries, netw_update_queries;
int free_tmp_buffers(struct connection *c);

int write_out_chk(struct connection *c, const void *data, int len);
void cond_dump_connection_buffers_stats();
void dump_connection_buffers_stats();

static inline int is_ipv6_localhost(unsigned char ipv6[16]) { return !*(long long *)ipv6 && ((long long *)ipv6)[1] == 1LL << 56; }

double get_recent_idle_percent();

int out_total_processed_bytes(struct connection *c);
int out_total_unprocessed_bytes(struct connection *c);

void init_connection_buffers(struct connection *c);
void clean_connection_buffers(struct connection *c);

#endif
