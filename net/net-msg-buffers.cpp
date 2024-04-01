// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-msg-buffers.h"

#include <stddef.h>

#include "common/allocators/lockfree-slab.h"
#include "common/options.h"
#include "common/parallel/counter.h"
#include "common/parallel/limit-counter.h"
#include "common/parallel/maximum.h"
#include "common/stats/provider.h"
#include "net-msg.h"

#define MSG_MAX_ALLOCATED_BYTES (1L << 40)
#define MSG_DEFAULT_MAX_ALLOCATED_BYTES (1L << 28)

PARALLEL_COUNTER(buffer_slab_alloc_ops);
PARALLEL_COUNTER(total_used_buffers);
PARALLEL_COUNTER(total_used_buffers_size);
PARALLEL_MAXIMUM(max_total_used_buffers_size);
PARALLEL_LIMIT_COUNTER(allocated_buffer_bytes);

static long long allocated_buffer_bytes_limit = MSG_DEFAULT_MAX_ALLOCATED_BYTES;

static int buffer_size_values;

OPTION_PARSER(OPT_NETWORK, "msg-buffers-size", required_argument, "memory for udp/new tcp/binlog buffers (default %lld megabytes)",
              allocated_buffer_bytes_limit >> 20) {
  allocated_buffer_bytes_limit = parse_memory_limit(optarg);
  PARALLEL_LIMIT_COUNTER_INIT(allocated_buffer_bytes, allocated_buffer_bytes_limit, 1 * 1024 * 1024);
  return 0;
}

STATS_PROVIDER(msg_buffers, 1000) {
  stats->add_histogram_stat("allocated_buffer_bytes", PARALLEL_LIMIT_COUNTER_READ(allocated_buffer_bytes));
  stats->add_histogram_stat("buffer_chunk_allocations", PARALLEL_COUNTER_READ(buffer_slab_alloc_ops));
  stats->add_histogram_stat("max_allocated_buffer_bytes", allocated_buffer_bytes_limit);
  stats->add_histogram_stat("max_total_used_buffers_size", PARALLEL_MAXIMUM_READ(max_total_used_buffers_size));
  stats->add_histogram_stat("total_used_buffers", PARALLEL_COUNTER_READ(total_used_buffers));
  stats->add_histogram_stat("total_used_buffers_size", PARALLEL_COUNTER_READ(total_used_buffers_size));
}

#define BUFFER_SIZE_NUM 5
static const int default_buffer_sizes[BUFFER_SIZE_NUM] = {48, 512, 2048, 16384, 262144};
static lockfree_slab_cache_t slab_caches[BUFFER_SIZE_NUM];
static __thread lockfree_slab_cache_tls_t slab_caches_tls[BUFFER_SIZE_NUM];

static void msg_buffers_constructor() __attribute__((constructor));
static void msg_buffers_constructor() {
  for (int i = 0; i < BUFFER_SIZE_NUM; ++i) {
    lockfree_slab_cache_init(&slab_caches[i], default_buffer_sizes[i] + sizeof(struct msg_buffer));
    assert(!i || default_buffer_sizes[i] > default_buffer_sizes[i - 1]);
  }
}

static void init_slab_caches() {
  for (int i = 0; i < BUFFER_SIZE_NUM; ++i) {
    lockfree_slab_cache_register_thread(&slab_caches[i], &slab_caches_tls[i]);
    slab_caches_tls[i].extra = default_buffer_sizes[i];
  }
}

void msg_buffers_register_thread() {
  init_slab_caches();
  parallel_register_thread();
  PARALLEL_MAXIMUM_INIT(max_total_used_buffers_size, 1 * 1024 * 1024);
  PARALLEL_LIMIT_COUNTER_INIT(allocated_buffer_bytes, allocated_buffer_bytes_limit, 1 * 1024 * 1024);
  PARALLEL_COUNTER_REGISTER_THREAD(buffer_slab_alloc_ops);
  PARALLEL_COUNTER_REGISTER_THREAD(total_used_buffers);
  PARALLEL_COUNTER_REGISTER_THREAD(total_used_buffers_size);
  PARALLEL_MAXIMUM_REGISTER_THREAD(max_total_used_buffers_size);
  PARALLEL_LIMIT_COUNTER_REGISTER_THREAD(allocated_buffer_bytes);
  init_msg();
}

void preallocate_msg_buffers() {
  if (!buffer_size_values) {
    msg_buffers_register_thread();
    assert(allocated_buffer_bytes_limit >= 0 && allocated_buffer_bytes_limit <= MSG_MAX_ALLOCATED_BYTES);
    assert(allocated_buffer_bytes_limit >= PARALLEL_LIMIT_COUNTER_READ(allocated_buffer_bytes));
    buffer_size_values = BUFFER_SIZE_NUM;
  }
}

/* allocates buffer of at least given size, -1 = maximal */
msg_buffer_t *alloc_msg_buffer(int size_hint) {
  preallocate_msg_buffers();

  int si = buffer_size_values - 1;
  if (size_hint >= 0) {
    while (si > 0 && default_buffer_sizes[si - 1] >= size_hint) {
      si--;
    }
  }
  lockfree_slab_cache_t *cache = &slab_caches[si];
  lockfree_slab_cache_tls_t *cache_tls = &slab_caches_tls[si];

  if (!PARALLEL_LIMIT_COUNTER_ADD(allocated_buffer_bytes, cache->object_size)) {
    return NULL;
  }

  const unsigned blocks_before = lockfree_slab_cache_count_used_blocks(cache_tls);
  msg_buffer_t *buffer = static_cast<msg_buffer_t *>(lockfree_slab_cache_alloc(cache_tls));
  const unsigned delta = lockfree_slab_cache_count_used_blocks(cache_tls) - blocks_before;
  PARALLEL_COUNTER_ADD(buffer_slab_alloc_ops, delta);

  buffer->cache_tls = cache_tls;
  buffer->refcnt = 1;

  PARALLEL_COUNTER_INC(total_used_buffers);
  PARALLEL_COUNTER_ADD(total_used_buffers_size, cache->object_size);
  PARALLEL_MAXIMUM_ADD(max_total_used_buffers_size, cache->object_size);

  return buffer;
}

void free_msg_buffer(msg_buffer_t *buffer) {
  assert(!buffer->refcnt);

  lockfree_slab_cache_t *cache = buffer->cache_tls->cache;
  lockfree_slab_cache_tls_t *cache_tls = &slab_caches_tls[cache - slab_caches];

  PARALLEL_COUNTER_DEC(total_used_buffers);
  PARALLEL_COUNTER_SUB(total_used_buffers_size, cache->object_size);
  PARALLEL_MAXIMUM_SUB(max_total_used_buffers_size, cache->object_size);
  PARALLEL_LIMIT_COUNTER_SUB(allocated_buffer_bytes, cache->object_size);

  const unsigned blocks_before = lockfree_slab_cache_count_used_blocks(cache_tls);
  lockfree_slab_cache_free(cache_tls, buffer);
  const unsigned delta = blocks_before - lockfree_slab_cache_count_used_blocks(cache_tls);
  PARALLEL_COUNTER_SUB(buffer_slab_alloc_ops, delta);
}
