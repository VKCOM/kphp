// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/php-queries.h"

#include <array>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <type_traits>

#include "common/precise-time.h"
#include "common/wrappers/overloaded.h"

#include "net/net-buffers.h"
#include "net/net-connections.h"

#include "runtime/allocator.h"
#include "runtime/job-workers/processing-jobs.h"
#include "runtime/tl/tl_magics_decoding.h"
#include "runtime/rpc.h"

#include "server/database-drivers/adaptor.h"
#include "server/job-workers/job-message.h"
#include "server/php-engine-vars.h"
#include "server/php-init-scripts.h"
#include "server/php-queries-stats.h"
#include "server/php-runner.h"
#include "server/php-worker.h"

#define MAX_NET_ERROR_LEN 128

static char last_net_error[MAX_NET_ERROR_LEN + 1];

static void save_last_net_error(const char *s) {
  if (s == nullptr) {
    last_net_error[0] = 0;
    return;
  }

  auto l = strlen(s);
  if (l >= MAX_NET_ERROR_LEN) {
    l = MAX_NET_ERROR_LEN - 1;
  }
  memcpy(last_net_error, s, l);
  last_net_error[l] = 0;
}

#undef MAX_NET_ERROR_LEN

/** create connection query **/
php_query_http_load_post_answer_t *php_query_http_load(char *buf, int min_len, int max_len) {
  assert (PhpScript::in_script_context);

  //DO NOT use query after script is terminated!!!
  php_query_http_load_post_t q;
  q.buf = buf;
  q.min_len = min_len;
  q.max_len = max_len;

  PhpScript::current_script->ask_query(&q);

  return static_cast<php_query_http_load_post_answer_t *>(q.ans);
}

int http_load_long_query(char *buf, int min_len, int max_len) {
  php_query_http_load_post_answer_t *ans = php_query_http_load(buf, min_len, max_len);
  assert (min_len <= ans->loaded_bytes && ans->loaded_bytes <= max_len);
  return ans->loaded_bytes;
}


/***
 QUERY MEMORY ALLOCATOR
 ***/

#include <map>

#include <cassert>

long long qmem_generation = 0;

namespace qmem {
// constants
const int static_pages_n = 2;
const size_t page_size = (1 << 22), max_mem = (1 << 27);
const int max_pages_n = 1000;
char static_memory[static_pages_n][page_size];

//variables
void *pages[max_pages_n];
size_t pages_size[max_pages_n], cur_mem, used_mem;
int pages_n;

enum state_t {
  st_empty,
  st_inited
};
state_t state;

std::multimap<size_t, void *> left;

//private functions
inline void reg(void *ptr, size_t ptr_size) {
  left.insert(std::make_pair(ptr_size, ptr));
}

void *alloc_at_least(size_t n, size_t use) {

  size_t alloc_size = n >= page_size ? n : page_size;

  if (pages_n == max_pages_n || alloc_size > max_mem || alloc_size + cur_mem > max_mem) {
    assert ("NOT ENOUGH MEMORY\n" && false);
    return nullptr;
  }

  void *ptr = pages[pages_n] = malloc(alloc_size);
  assert (ptr != nullptr);

  pages_size[pages_n] = alloc_size;

  if (use < alloc_size) {
    reg((char *)ptr + use, alloc_size - use);
  }

  pages_n++;
  return ptr;
}
} // namespace qmem

//public functions
void qmem_init() {
  using namespace qmem;

  assert (state == st_empty);

  assert (left.empty());

  cur_mem = 0;
  pages_n = static_pages_n;
  for (int i = 0; i < static_pages_n; i++) {
    pages[i] = static_memory[i];
    pages_size[i] = page_size;
    cur_mem += page_size;

    reg(pages[i], pages_size[i]);
  }

  state = st_inited;
}


void *qmem_malloc(size_t n) {
  using namespace qmem;

  std::multimap<size_t, void *>::iterator i = left.lower_bound(n);
  used_mem += n;
  if (i == left.end()) {
    return alloc_at_least(n, n);
  }

  void *ptr = i->second;
  size_t ptr_size = i->first;

  left.erase(i);

  ptr_size -= n;
  if (ptr_size > 0) {
    reg((char *)ptr + n, ptr_size);
  }

  return ptr;
}

void *qmem_malloc0(size_t n) {
  void *res = qmem_malloc(n);
  if (res != nullptr) {
    memset(res, 0, n);
  }
  return res;
}


void qmem_free_ptrs() {
  using namespace qmem;

  if (used_mem + used_mem > cur_mem) {
    left.clear();
    used_mem = 0;
    for (int i = 0; i < pages_n; i++) {
      reg(pages[i], pages_size[i]);
    }
  }

  qmem_generation++;
}

void qmem_clear() {
  using namespace qmem;

  assert (state == st_inited);

  left.clear();
  used_mem = 0;
  for (int i = static_pages_n; i < pages_n; i++) {
    free(pages[i]);
  }

  state = st_empty;

  qmem_generation++;
}

/** qmem_pstr **/
const char *qmem_pstr(char const *msg, ...) {
  const int maxlen = 5000;
  static char s[maxlen];
  va_list args;

  va_start (args, msg);
  int len = vsnprintf(s, maxlen, msg, args);
  va_end (args);

  if (len >= maxlen) {
    len = maxlen - 1;
  }

  char *res = (char *)qmem_malloc(len + 1);
  memcpy(res, s, len);
  res[len] = 0;

  return res;
}

/** str_buffer **/
str_buf_t *str_buf_create() ubsan_supp("alignment");

str_buf_t *str_buf_create() {
  str_buf_t *buf = (str_buf_t *)qmem_malloc(sizeof(str_buf_t));
  assert (buf != nullptr);
  buf->buf_len = 0;
  buf->len = 0;

  return buf;
}

void str_buf_append(str_buf_t *buf, data_reader_t *reader) ubsan_supp("alignment");

void str_buf_append(str_buf_t *buf, data_reader_t *reader) {
  int need = reader->len + buf->len;
  if (need >= buf->buf_len) {
    need = need * 2 + 1;
    char *new_buf = (char *)qmem_malloc(need);
    assert (new_buf != nullptr);
    if (buf->buf) {
      memcpy(new_buf, buf->buf, buf->len);
    }
    buf->buf = new_buf;
    buf->buf_len = need;
  }

  reader->read(reader, buf->buf + buf->len);
  buf->len += reader->len;
}

char *str_buf_cstr(str_buf_t *buf) ubsan_supp("alignment");

char *str_buf_cstr(str_buf_t *buf) {
  buf->buf[buf->len] = 0;
  return buf->buf;
}

int str_buf_len(str_buf_t *buf) ubsan_supp("alignment");

int str_buf_len(str_buf_t *buf) {
  return buf->len;
}

/** chain **/
void chain_conn(chain_t *a, chain_t *b) {
  a->next = b;
  b->prev = a;
}

chain_t *chain_create() {
  chain_t *chain = (chain_t *)qmem_malloc(sizeof(chain_t));
  assert (chain != nullptr);
  chain_conn(chain, chain);
  return chain;
}

void chain_append(chain_t *chain, data_reader_t *reader) {
  chain_t *node = (chain_t *)qmem_malloc(sizeof(chain_t));
  assert (node != nullptr);

  node->buf = (char *)qmem_malloc(reader->len);
  assert (node->buf != nullptr);
  node->len = reader->len;
  reader->read(reader, node->buf);

  chain_conn(chain->prev, node);
  chain_conn(node, chain);
}

/***
  QUERIES
 ***/

/** test x^2 query **/
php_query_x2_answer_t *php_query_x2(int x) {
  assert (PhpScript::in_script_context);

  //DO NOT use query after script is terminated!!!
  php_query_x2_t q;
  q.val = x;

  PhpScript::current_script->ask_query(&q);

  return static_cast<php_query_x2_answer_t *>(q.ans);
}

/** create connection query **/
php_query_connect_answer_t *php_query_connect(const char *host, int port, protocol_type protocol) {
  assert (PhpScript::in_script_context);

  //DO NOT use query after script is terminated!!!
  php_query_connect_t q;
  q.host = host;
  q.port = port;
  q.protocol = protocol;

  PhpScript::current_script->ask_query(&q);

  return static_cast<php_query_connect_answer_t *>(q.ans);
}

int mc_connect_to(const char *host, int port) {
  php_query_connect_answer_t *ans = php_query_connect(host, port, protocol_type::memcached);
  return ans->connection_id;
}

int db_proxy_connect() {
  php_query_connect_answer_t *ans = php_query_connect("unknown", -1, protocol_type::mysqli);
  return ans->connection_id;
}

int rpc_connect_to(const char *host, int port) {
  php_query_connect_answer_t *ans = php_query_connect(host, port, protocol_type::rpc);
  return ans->connection_id;
}

/** net query **/
php_net_query_packet_answer_t *php_net_query_packet(
  int connection_id, const char *data, int data_len,
  double timeout, protocol_type protocol, int extra_type) {
  php_net_query_packet_t q;

  q.connection_id = connection_id;
  q.data = data;
  q.data_len = data_len;
  q.timeout = timeout;
  q.protocol = protocol;
  q.extra_type = extra_type;

  PhpScript::current_script->ask_query(&q);

  return static_cast<php_net_query_packet_answer_t *>(q.ans);
}

void read_str(data_reader_t *reader, void *dest) {
  //reader->readed = 1;
  memcpy(dest, reader->extra, reader->len);
}

/** net answer generator **/

bool net_ansgen_is_alive(net_ansgen_t *base_self) {
  return base_self->qmem_req_generation == qmem_generation;
}

void net_ansgen_timeout(net_ansgen_t *base_self) ubsan_supp("alignment");

void net_ansgen_timeout(net_ansgen_t *base_self) {
//  fprintf (stderr, "mc_ansgen_packet_timeout %p\n", base_self);

  assert (base_self->state == st_ansgen_wait);

  if (net_ansgen_is_alive(base_self)) {
    base_self->ans->state = nq_error;
    base_self->ans->res = "Timeout";
    base_self->qmem_req_generation = -1;
  }
}


void net_ansgen_error(net_ansgen_t *base_self, const char *val) {
//  fprintf (stderr, "mc_ansgen_packet_error %p\n", base_self);

  assert (base_self->state == st_ansgen_wait);

  if (net_ansgen_is_alive(base_self)) {
    base_self->ans->state = nq_error;
    base_self->ans->res = val;
  }

  base_self->state = st_ansgen_error;
}

void net_ansgen_set_desc(net_ansgen_t *base_self, const char *val) ubsan_supp("alignment");

void net_ansgen_set_desc(net_ansgen_t *base_self, const char *val) {
  if (net_ansgen_is_alive(base_self)) {
    base_self->ans->desc = val;
  }
}


/** memcached answer generators **/

static data_reader_t end_reader, stored_reader, notstored_reader;

void init_reader(data_reader_t *reader, const char *s) {
  reader->len = (int)strlen(s);
  reader->extra = (void *)s;
  reader->readed = 0;
  reader->read = read_str;
}

void init_readers() {
  init_reader(&end_reader, "END\r\n");
  init_reader(&stored_reader, "STORED\r\n");
  init_reader(&notstored_reader, "NOT_STORED\r\n");
}

void mc_ansgen_packet_xstored(mc_ansgen_t *mc_self, int is_stored) {
  mc_ansgen_packet_t *self = (mc_ansgen_packet_t *)mc_self;
  net_ansgen_t *base_self = (net_ansgen_t *)mc_self;

  if (self->state == ap_any) {
    self->state = ap_store;
  }
  if (self->state != ap_store) {
    base_self->func->error(base_self, "Unexpected STORED");
  } else {
    if (net_ansgen_is_alive(base_self)) {
      str_buf_append(self->str_buf, is_stored ? &stored_reader : &notstored_reader);
      base_self->ans->state = nq_ok;
      base_self->ans->res = str_buf_cstr(self->str_buf);
      base_self->ans->res_len = str_buf_len(self->str_buf);
    }
    base_self->state = st_ansgen_done;
  }
}


void mc_ansgen_packet_value(mc_ansgen_t *mc_self, data_reader_t *reader) {
//  fprintf (stderr, "mc_ansgen_packet_value\n");
  mc_ansgen_packet_t *self = (mc_ansgen_packet_t *)mc_self;
  net_ansgen_t *base_self = (net_ansgen_t *)mc_self;

  if (self->state == ap_any) {
    self->state = ap_get;
  }
  if (self->state != ap_get) {
    base_self->func->error(base_self, "Unexpected VALUE");
  } else if (net_ansgen_is_alive(base_self)) {
    str_buf_append(self->str_buf, reader);
  }
}

void mc_ansgen_packet_version(mc_ansgen_t *mc_self, data_reader_t *reader) {
  mc_ansgen_packet_t *self = (mc_ansgen_packet_t *)mc_self;
  net_ansgen_t *base_self = (net_ansgen_t *)mc_self;

  if (self->state != ap_version) {
    return;
  }

  if (net_ansgen_is_alive(base_self)) {
    str_buf_append(self->str_buf, reader);
    base_self->ans->state = nq_ok;
    base_self->ans->res = str_buf_cstr(self->str_buf);
    base_self->ans->res_len = str_buf_len(self->str_buf);
  }
  base_self->state = st_ansgen_done;
}

void mc_ansgen_packet_set_query_type(mc_ansgen_t *mc_self, int query_type) {
  mc_ansgen_packet_t *self = (mc_ansgen_packet_t *)mc_self;
  net_ansgen_t *base_self = (net_ansgen_t *)mc_self;

  mc_ansgen_packet_state_t new_state = ap_any;
  if (query_type == 1) {
    new_state = ap_version;
  }
  if (new_state == ap_any) {
    return;
  }
  if (self->state == ap_any) {
    self->state = new_state;
  }
  if (self->state != new_state) {
    base_self->func->error(base_self, "Can't determine query type");
  }
}

void mc_ansgen_packet_other(mc_ansgen_t *mc_self, data_reader_t *reader) {
//  fprintf (stderr, "mc_ansgen_packet_value\n");
  mc_ansgen_packet_t *self = (mc_ansgen_packet_t *)mc_self;
  net_ansgen_t *base_self = (net_ansgen_t *)mc_self;

  if (self->state == ap_any) {
    self->state = ap_other;
  }
  if (self->state != ap_other) {
    base_self->func->error(base_self, "Unexpected \"other\" command");
  } else {
    if (net_ansgen_is_alive(base_self)) {
      str_buf_append(self->str_buf, reader);
      base_self->ans->state = nq_ok;
      base_self->ans->res = str_buf_cstr(self->str_buf);
      base_self->ans->res_len = str_buf_len(self->str_buf);
    }
    base_self->state = st_ansgen_done;
  }
}

void mc_ansgen_packet_end(mc_ansgen_t *mc_self) ubsan_supp("alignment");

void mc_ansgen_packet_end(mc_ansgen_t *mc_self) {
//  fprintf (stderr, "mc_ansgen_packet_end\n");
  mc_ansgen_packet_t *self = (mc_ansgen_packet_t *)mc_self;
  net_ansgen_t *base_self = (net_ansgen_t *)mc_self;

  assert (base_self->state == st_ansgen_wait);
  if (self->state == ap_any) {
    self->state = ap_get;
  }

  if (self->state != ap_get) {
    base_self->func->error(base_self, "Unexpected END");
  } else {
    if (net_ansgen_is_alive(base_self)) {
      str_buf_append(self->str_buf, &end_reader);

      base_self->ans->state = nq_ok;
      base_self->ans->res = str_buf_cstr(self->str_buf);
      base_self->ans->res_len = str_buf_len(self->str_buf);
    }
    base_self->state = st_ansgen_done;
  }

}

void mc_ansgen_packet_free(net_ansgen_t *base_self) {
  mc_ansgen_packet_t *self = (mc_ansgen_packet_t *)base_self;
  free(self);
}

net_ansgen_func_t *get_mc_net_ansgen_functions() {
  static bool inited = false;
  static net_ansgen_func_t f;
  if (!inited) {
    f.error = net_ansgen_error;
    f.timeout = net_ansgen_timeout;
    f.set_desc = net_ansgen_set_desc;
    f.free = mc_ansgen_packet_free;
    inited = true;
  }
  return &f;
}

mc_ansgen_func_t *get_mc_ansgen_functions() {
  static bool inited = false;
  static mc_ansgen_func_t f;
  if (!inited) {
    f.value = mc_ansgen_packet_value;
    f.end = mc_ansgen_packet_end;
    f.xstored = mc_ansgen_packet_xstored;
    f.other = mc_ansgen_packet_other;
    f.version = mc_ansgen_packet_version;
    f.set_query_type = mc_ansgen_packet_set_query_type;
    inited = true;
  }
  return &f;
}

mc_ansgen_t *mc_ansgen_packet_create() {
  mc_ansgen_packet_t *ansgen = (mc_ansgen_packet_t *)malloc(sizeof(mc_ansgen_packet_t));

  ansgen->base.func = get_mc_net_ansgen_functions();
  ansgen->func = get_mc_ansgen_functions();

  ansgen->base.qmem_req_generation = qmem_generation;
  ansgen->base.state = st_ansgen_wait;
  ansgen->base.ans = (php_net_query_packet_answer_t *)qmem_malloc0(sizeof(php_net_query_packet_answer_t));
  assert (ansgen->base.ans != nullptr);

  ansgen->state = ap_any;


  ansgen->str_buf = str_buf_create();

  return (mc_ansgen_t *)ansgen;
}

/*** sql answer generator ***/

void sql_ansgen_packet_set_writer(sql_ansgen_t *sql_self, command_t *writer) {
  sql_ansgen_packet_t *self = (sql_ansgen_packet_t *)sql_self;
  net_ansgen_t *base_self = (net_ansgen_t *)sql_self;

  assert (base_self->state == st_ansgen_wait);
  assert (self->state == sql_ap_init);

  self->writer = writer;
  self->state = sql_ap_wait_conn;
}

void sql_ansgen_packet_ready(sql_ansgen_t *sql_self, void *data) {
  sql_ansgen_packet_t *self = (sql_ansgen_packet_t *)sql_self;
  net_ansgen_t *base_self = (net_ansgen_t *)sql_self;

  assert (base_self->state == st_ansgen_wait);
  assert (self->state == sql_ap_wait_conn);

  if (self->writer != nullptr) {
    self->writer->run(self->writer, data);
  }
  self->state = sql_ap_wait_ans;
}

void sql_ansgen_packet_add_packet(sql_ansgen_t *sql_self, data_reader_t *reader) {
  sql_ansgen_packet_t *self = (sql_ansgen_packet_t *)sql_self;
  net_ansgen_t *base_self = (net_ansgen_t *)sql_self;

  assert (base_self->state == st_ansgen_wait);
  assert (self->state == sql_ap_wait_ans);

  if (net_ansgen_is_alive(base_self)) {
    chain_append(self->chain, reader);
  }
}

void sql_ansgen_packet_done(sql_ansgen_t *sql_self) {
  sql_ansgen_packet_t *self = (sql_ansgen_packet_t *)sql_self;
  net_ansgen_t *base_self = (net_ansgen_t *)sql_self;

  assert (base_self->state == st_ansgen_wait);
  assert (self->state == sql_ap_wait_ans);

  if (net_ansgen_is_alive(base_self)) {
    base_self->ans->state = nq_ok;
    base_self->ans->chain = self->chain;
  }

  base_self->state = st_ansgen_done;
}

void sql_ansgen_packet_free(net_ansgen_t *base_self) {
  sql_ansgen_packet_t *self = (sql_ansgen_packet_t *)base_self;
  if (self->writer != nullptr) {
    self->writer->free(self->writer);
    self->writer = nullptr;
  }
  free(self);
}

net_ansgen_func_t *get_sql_net_ansgen_functions() {
  static bool inited = false;
  static net_ansgen_func_t f;
  if (!inited) {
    f.error = net_ansgen_error;
    f.timeout = net_ansgen_timeout;
    f.set_desc = net_ansgen_set_desc;
    f.free = sql_ansgen_packet_free;
    inited = true;
  }
  return &f;
}

sql_ansgen_func_t *get_sql_ansgen_functions() {
  static bool inited = false;
  static sql_ansgen_func_t f;
  if (!inited) {
    f.set_writer = sql_ansgen_packet_set_writer;
    f.ready = sql_ansgen_packet_ready;
    f.packet = sql_ansgen_packet_add_packet;
    f.done = sql_ansgen_packet_done;
    inited = true;
  }
  return &f;
}

sql_ansgen_t *sql_ansgen_packet_create() {
  sql_ansgen_packet_t *ansgen = (sql_ansgen_packet_t *)malloc(sizeof(sql_ansgen_packet_t));

  ansgen->base.func = get_sql_net_ansgen_functions();
  ansgen->func = get_sql_ansgen_functions();

  ansgen->base.qmem_req_generation = qmem_generation;
  ansgen->base.state = st_ansgen_wait;
  ansgen->base.ans = (php_net_query_packet_answer_t *)qmem_malloc0(sizeof(php_net_query_packet_answer_t));
  assert (ansgen->base.ans != nullptr);

  ansgen->state = sql_ap_init;
  ansgen->writer = nullptr;


  ansgen->chain = chain_create();

  return (sql_ansgen_t *)ansgen;
}

/** new rpc interface **/

template<class DataT, int N>
class StaticQueue {
  static_assert(std::is_trivially_destructible_v<DataT>, "N destructors will be called on server shutdown, and server will most probably hang");
private:
  DataT q[N];
  int begin{};
  int end{};
  int cnt{};
public:
  StaticQueue() :
    begin(0),
    end(0),
    cnt(0) {}

  void clear() {
    begin = 0;
    end = 0;
    cnt = 0;
  }

  bool empty() const noexcept {
    return cnt == 0;
  }

  DataT *create() {
    if (cnt == N) {
      return nullptr;
    }
    DataT *res = &q[end];
    end++;
    cnt++;
    if (end == N) {
      end = 0;
    }
    return res;
  }

  const DataT *top() const noexcept {
    if (empty()) {
      return nullptr;
    }
    return &q[begin];
  }

  void undo_create(DataT *event) {
    int old_end = end - 1;
    if (old_end == -1) {
      old_end = N - 1;
    }
    assert (event == &q[old_end]);
    end = old_end;
    cnt--;
  }

  DataT *pop() {
    if (empty()) {
      return nullptr;
    }
    DataT *res = &q[begin];
    begin++;
    cnt--;
    if (begin == N) {
      begin = 0;
    }
    return res;
  }
};

static StaticQueue<net_event_t, 2000000> net_events;
static StaticQueue<net_query_t, 2000000> net_queries;

void *dl_allocate_safe(size_t size) {
  if (size == 0 || PhpScript::memory_limit_exceeded) {
    return nullptr;
  }

  assert (size <= (1u << 30) - 13);
  void *dest = dl::allocate(size + 13);
  if (dest == nullptr) {
    return nullptr;
  }

  int *dest_int = static_cast <int *> (dest);
  dest_int[0] = static_cast<int>(size);
  dest_int[1] = static_cast<int>(size);
  dest_int[2] = 0;
  (static_cast <char *> (dest))[size + 12] = '\0';

//  fprintf (stderr, "Allocate string of len %d at %p\n", (int)size, (void *)(dest_int + 3));
//  for (int i = 0; i < (int)size + 13; i++) {
//    fprintf (stderr, "%d: %x(%d)\n", i - 12, static_cast <char *> (dest)[i], static_cast <char *> (dest)[i]);
//  }

  return (void *)(dest_int + 3);
}

int alloc_net_event(slot_id_t slot_id, net_event_t **res) {
  net_event_t *event = net_events.create();
  if (event == nullptr) {
    return -2;
  }
  event->slot_id = slot_id;
  *res = event;
  return 1;
}

void unalloc_net_event(net_event_t *event) {
  net_events.undo_create(event);
}

int create_rpc_error_event(slot_id_t slot_id, int error_code, const char *error_message, net_event_t **res) {
  net_event_t *event;
  if (!rpc_ids_factory.is_from_current_script_execution(slot_id)) {
    return 0;
  }
  int status = alloc_net_event(slot_id, &event);
  if (status <= 0) {
    return status;
  }
  event->data = net_events_data::rpc_error{error_code, error_message}; //error_message in static memory
  if (res != nullptr) {
    *res = event;
  }
  return 1;
}

int create_rpc_answer_event(slot_id_t slot_id, int len, net_event_t **res) {
  PhpQueriesStats::get_rpc_queries_stat().register_answer(len);
  net_event_t *event;
  if (!rpc_ids_factory.is_from_current_script_execution(slot_id)) {
    return 0;
  }
  int status = alloc_net_event(slot_id, &event);
  if (status <= 0) {
    return status;
  }
  net_events_data::rpc_answer data;
  if (len != 0) {
    void *buf = dl_allocate_safe(len);
    if (buf == nullptr) {
      unalloc_net_event(event);
      return -1;
    }
    data.result = static_cast <char *> (buf);
  } else {
    data.result = nullptr;
  }
  data.result_len = len;
  event->data = data;
  assert (res != nullptr);
  *res = event;
  return 1;
}

int create_job_worker_answer_event(job_workers::JobSharedMessage *job_result) {
  if (!parallel_job_ids_factory.is_from_current_script_execution(job_result->job_id)) {
    return 0;
  }
  net_event_t *event = nullptr;
  const int status = alloc_net_event(job_result->job_id, &event);
  if (status <= 0) {
    return status;
  }
  event->data = net_events_data::job_worker_answer{ job_workers::copy_finished_job_to_script_memory(job_result) };
  return 1;
}

int net_events_empty() {
  return net_events.empty();
}

net_query_t *create_net_query() {
  net_query_t *query = net_queries.create();
  if (query == nullptr) {
    return nullptr;
  }
  return query;
}

void unalloc_net_query(net_query_t *query) {
  net_queries.undo_create(query);
}

net_query_t *pop_net_query() {
  return net_queries.pop();
}

void free_rpc_send_query(const net_queries_data::rpc_send &query) {
  dl::deallocate(query.request, query.request_size);
}

/*** main functions ***/
void mc_run_query(int host_num, const char *request, int request_len, int timeout_ms, int query_type, void (*callback)(const char *result, int result_len)) {
  PhpQueriesStats::get_mc_queries_stat().register_query(request_len);
  php_net_query_packet_answer_t *res = php_net_query_packet(host_num, request, request_len, timeout_ms * 0.001, protocol_type::memcached, query_type | (PNETF_IMMEDIATE * (callback == nullptr)));
  if (res->state == nq_error) {
    if (callback != nullptr) {
      fprintf(stderr, "mc_run_query error: %s [%s]\n", res->desc ? res->desc : "", res->res);
    }
    save_last_net_error(res->res);
  } else {
    assert (res->res != nullptr);
    PhpQueriesStats::get_mc_queries_stat().register_answer(res->res_len);
    if (callback != nullptr) {
      callback(res->res, res->res_len);
    }
  }
}

void db_run_query(int host_num, const char *request, int request_len, int timeout_ms, void (*callback)(const char *result, int result_len)) {
  PhpQueriesStats::get_sql_queries_stat().register_query(request_len);
  php_net_query_packet_answer_t *res = php_net_query_packet(host_num, request, request_len, timeout_ms * 0.001, protocol_type::mysqli, 0);
  if (res->state == nq_error) {
    fprintf(stderr, "db_run_query error: %s [%s]\n", res->desc ? res->desc : "", res->res);
    save_last_net_error(res->res);
  } else {
    assert (res->chain != nullptr);
    chain_t *cur = res->chain->next;
    while (cur != res->chain) {
      //fprintf (stderr, "sql_callback [len = %d]\n", cur->len);
      PhpQueriesStats::get_sql_queries_stat().register_answer(cur->len);
      callback(cur->buf, cur->len);
      cur = cur->next;
    }
  }
}

void http_send_immediate_response(const char *headers, int headers_len, const char *body, int body_len) {
  php_assert(active_worker != nullptr);
  if (active_worker->mode == http_worker) {
    write_out(&active_worker->conn->Out, headers, headers_len);
    write_out(&active_worker->conn->Out, body, body_len);
    flush_connection_output(active_worker->conn);
  } else {
    php_warning("Immediate HTTP response available only from HTTP worker");
  }
}

slot_id_t rpc_send_query(int host_num, char *request, int request_size, int timeout_ms) {
  net_query_t *query = create_net_query();
  if (query == nullptr) {
    return -1; // memory limit
  }
  query->slot_id = rpc_ids_factory.create_slot();
  if (query->slot_id == -1) {
    unalloc_net_query(query);
    return -1;
  }

  PhpQueriesStats::get_rpc_queries_stat().register_query(request_size);
  query->data = net_queries_data::rpc_send{ host_num, request, request_size, timeout_ms };
  return query->slot_id;
}

void wait_net_events(int timeout_ms) {
  assert (PhpScript::in_script_context);
  php_query_wait_t q;
  q.timeout_ms = timeout_ms;

  PhpScript::current_script->ask_query(&q);
}

net_event_t *pop_net_event() {
  return net_events.pop();
}

const net_event_t *get_last_net_event() {
  return net_events.top();
}

void rpc_answer(const char *res, int res_len) {
  assert (PhpScript::in_script_context);
  php_query_rpc_answer q;
  q.data = res;
  q.data_len = res_len;

  PhpScript::current_script->ask_query(&q);
}

php_net_query_packet_answer_t *php_net_query_get(int connection_id, const char *data, int data_len, int timeout_ms, protocol_type protocol) {
  php_net_query_packet_t q;

  q.connection_id = connection_id;
  q.data = data;
  q.data_len = data_len;
  q.timeout = timeout_ms * 0.001;
  q.protocol = protocol;

  PhpScript::current_script->ask_query(&q);

  return static_cast<php_net_query_packet_answer_t *>(q.ans);
}

void script_error() {
  PhpScript::error("script_error called", script_error_t::unclassified_error);
}

void http_set_result(const char *headers, int headers_len, const char *body, int body_len, int exit_code) {
  script_result res;
  res.exit_code = exit_code;
  res.headers = headers;
  res.headers_len = headers_len;
  res.body = body;
  res.body_len = body_len;

  PhpScript::current_script->set_script_result(&res);
}

void rpc_set_result(const char *body, int body_len, int exit_code) {
  script_result res;
  res.exit_code = exit_code;
  res.headers = nullptr;
  res.headers_len = 0;
  res.body = body;
  res.body_len = body_len;

  PhpScript::current_script->set_script_result(&res);
}

void job_set_result(int exit_code) {
  script_result res;
  res.exit_code = exit_code;
  res.headers = nullptr;
  res.headers_len = 0;
  res.body = nullptr;
  res.body_len = 0;

  PhpScript::current_script->set_script_result(&res);
}

void finish_script(int exit_code __attribute__((unused))) {
  //TODO
  assert (0);
}

void check_script_timeout() {
  PhpScript::current_script->try_run_shutdown_functions_on_timeout();
}

void reset_script_timeout() {
  PhpScript::current_script->reset_script_timeout();
}

double get_net_time() {
  return PhpScript::current_script->get_net_time();
}

double get_script_time() {
  return PhpScript::current_script->get_script_time();
}

int get_net_queries_count() {
  return PhpScript::current_script->get_net_queries_count();
}


int get_engine_uptime() {
  return get_uptime();
}

const char *get_engine_version() {
  return get_version_string();
}


int query_x2(int x) {
  php_query_x2_answer_t *ans = php_query_x2(x);
  return ans->x2;
}

void init_drivers() {
  init_readers();
}

void php_queries_start() {
  qmem_init();
}

void php_queries_finish() {
  qmem_clear();

  net_events.clear();
  net_queries.clear();
}

const char *net_event_t::get_description() const noexcept {
  static std::array<char, 10000> BUF;
  std::visit(overloaded{
    [this](const net_events_data::rpc_answer &event) {
      auto *r = get_rpc_request(slot_id);
      snprintf(BUF.data(), BUF.size(), "RPC_RESPONSE: actor_id=%d, tl_function=%s(0x%08x), response_magic=0x%08x, bytes_length=%d",
                           r->actor_or_port, tl_magic_convert_to_name(r->function_magic), r->function_magic,
                           event.result_len >= 4 ? *reinterpret_cast<unsigned int *>(event.result) : 0, event.result_len);
    },
    [this](const net_events_data::rpc_error &event) {
      auto *r = get_rpc_request(slot_id);
      snprintf(BUF.data(), BUF.size(), "RPC_ERROR: actor_id=%d, tl_function_magic=0x%08x, error_code=%d, error_message=%s",
                           r->actor_or_port, r->function_magic, event.error_code, event.error_message);
    },
    [](const net_events_data::job_worker_answer &event) {
      if (event.job_result) {
        snprintf(BUF.data(), BUF.size(), "JOB_RESPONSE: class_name=%s", event.job_result->response.get_class());
      } else {
        snprintf(BUF.data(), BUF.size(), "JOB_ERROR");
      }
    },
    [](const database_drivers::Response *) {
      snprintf(BUF.data(), BUF.size(), "EXTERNAL_DB_ANSWER");
    },
    [](const curl_async::CurlResponse *) {
      snprintf(BUF.data(), BUF.size(), "CURL_ASYNC_RESPONSE");
    },
  }, data);
  return BUF.data();
}
