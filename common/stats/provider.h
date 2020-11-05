// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/stats/buffer.h"

#define AM_GET_MEMORY_USAGE_SELF 1
#define AM_GET_MEMORY_USAGE_OVERALL 2

typedef enum {
  STATS_TYPE_TL,
  STATS_TYPE_STATSD
} stats_type;

typedef struct {
  stats_type type;
  stats_buffer_t sb;
  const char *statsd_prefix;
} stats_t;

char *statsd_normalize_key(const char *key);
void add_general_stat(stats_t *stats, const char *key, const char *value_format, ...) __attribute__ ((format (printf, 3, 4)));
void add_histogram_stat_long(stats_t *stats, const char *key, long long value);
void add_gauge_stat_long(stats_t *stats, const char *key, long long value);
void add_histogram_stat_double(stats_t *stats, const char *key, double value);
char *stat_temp_format(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
int am_get_memory_usage(pid_t pid, long long *a, int m);


typedef struct {
  const char *name;
  int priority; // smaller values mean early in stats
  unsigned int tag; // allows to specify tag for filtering stats
  void (*prepare)(stats_t *stats);
} stats_provider_t;

#define STATS_TAG_DEFAULT 1 << 0
#define STATS_TAG_KPHP_SERVER 1 << 31

#define STATS_TAG_MASK_FULL 0xFFFFFFFF

void register_stats_provider(stats_provider_t provider);
void prepare_common_stats_with_tag_mask(stats_t *stats, unsigned int tag_mask);
void prepare_common_stats(stats_t *stats);

static inline double safe_div(double x, double y) { return y > 0 ? x / y : 0; }

void sb_print_queries(stats_t *stats, const char *desc, long long q);
int get_at_prefix_length(const char *key, int key_len);
int64_t get_vmrss();

#define STATS_PROVIDER_TAGGED(nm, prio, tag_val)                          \
static void prepare_ ## nm ## _stats(stats_t *stats);                     \
static void register_ ## nm ## _stats() __attribute__((constructor));     \
static void register_ ## nm ## _stats() {                                 \
  stats_provider_t provider;                                              \
  provider.name =  #nm ;                                                  \
  provider.priority = (prio);                                             \
  provider.tag = (tag_val);                                               \
  provider.prepare = prepare_ ## nm ## _stats;                            \
  register_stats_provider(provider);                                      \
}                                                                         \
static void prepare_ ## nm ## _stats(stats_t *stats)

#define STATS_PROVIDER(nm, prio) STATS_PROVIDER_TAGGED(nm, prio, STATS_TAG_DEFAULT)

