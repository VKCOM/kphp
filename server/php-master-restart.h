// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "server/http-server-context.h"

// ATTENTION: do NOT change this structures without changing the magic
#define SHARED_DATA_MAGIC 0x3b720002

struct master_data_t {
  int is_alive;

  pid_t pid;
  unsigned long long start_time;

  long long rate;
  long long generation;

  int running_http_workers_n;
  int dying_http_workers_n;

  int to_kill;
  long long to_kill_generation;

  int own_http_fds;
  int http_ports_count;
  int ask_http_fds_generation;
  int sent_http_fds_generation;

  uint32_t instance_cache_elements_cached;

  uint16_t http_ports[HttpServerContext::MAX_HTTP_PORTS];

  int reserved[50 - 1 - HttpServerContext::MAX_HTTP_PORTS / 2];
};

struct shared_data_t {
  int magic;
  int is_inited;
  int init_owner;

  pthread_mutex_t main_mutex;
  pthread_cond_t main_cond;

  int id;
  master_data_t masters[2];
};

static constexpr size_t MASTER_DATA_T_SIZEOF = 272;
#if defined(__APPLE__)
static constexpr size_t SHARED_DATA_T_SIZEOF = 680;
#elif defined(__x86_64__)
static constexpr size_t SHARED_DATA_T_SIZEOF = 656;
#elif defined(__arm64__) || defined(__aarch64__)
static constexpr size_t SHARED_DATA_T_SIZEOF = 664;
#else
#error "Unsupported arch"
#endif

static_assert(sizeof(master_data_t) == MASTER_DATA_T_SIZEOF, "Layout of this struct must be the same in any KPHP version unless shared data magic is used, "
                                                             "otherwise restart won't work");
static_assert(sizeof(shared_data_t) == SHARED_DATA_T_SIZEOF, "Layout of this struct must be the same in any KPHP version unless shared data magic is used, "
                                                             "otherwise restart won't work");

extern shared_data_t *shared_data;
extern master_data_t *me, *other; // these are pointers to shared memory

inline bool in_new_master_on_restart() {
  return other->is_alive && me->rate > other->rate;
}

inline bool in_old_master_on_restart() {
  return other->is_alive && me->rate < other->rate;
}

shared_data_t *get_shared_data(const char *name);
void shared_data_lock(shared_data_t *data);
void shared_data_unlock(shared_data_t *data);
void shared_data_update(shared_data_t *shared_data);
void shared_data_get_masters(shared_data_t *shared_data, master_data_t **me, master_data_t **other);

void master_data_remove_if_dead(master_data_t *master);
void master_init(master_data_t *me, master_data_t *other);
