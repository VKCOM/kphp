// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/dl-utils-lite.h"
#include "common/kprintf.h"
#include "common/macos-ports.h"
#include "common/wrappers/memory-utils.h"
#include "server/php-master-restart.h"

// ATTENTION: the file has .cpp extension due to usage of "pthread_mutexattr_setrobust_np" which is by some strange reason unsupported for .c

shared_data_t *shared_data;
master_data_t *me, *other; // these are pointers to shared memory

void init_mutex(pthread_mutex_t *mutex) {
  pthread_mutexattr_t attr;

  int err;
  err = pthread_mutexattr_init(&attr);
  assert(err == 0 && "failed to init mutexattr");
  err = pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST_NP);
  assert(err == 0 && "failed to setrobust_np for mutex");
  err = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  assert(err == 0 && "failed to setpshared for mutex");

  err = pthread_mutex_init(mutex, &attr);
  assert(err == 0 && "failed to init mutex");
}

void shared_data_init(shared_data_t *data) {
  vkprintf(2, "Init shared data: begin\n");
  init_mutex(&data->main_mutex);

  data->magic = SHARED_DATA_MAGIC;
  data->is_inited = 1;
  vkprintf(2, "Init shared data: end\n");
}

shared_data_t *get_shared_data(const char *name) {
  int ret;
  vkprintf(2, "Get shared data: begin\n");
#if defined(__APPLE__)
  shm_unlink(name);
#endif
  int fid = shm_open(name, O_RDWR, 0777);
  int init_flag = 0;
  if (fid == -1) {
    if (errno == ENOENT) {
      vkprintf(1, "shared memory entry is not exists\n");
      vkprintf(1, "create shared memory\n");
      fid = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0777);
      if (fid == -1) {
        vkprintf(1, "failed to create shared memory\n");
        assert(errno == EEXIST && "failed to created shared memory for unknown reason");
        vkprintf(1, "somebody created it before us! so lets just open it\n");
        fid = shm_open(name, O_RDWR, 0777);
        assert(fid != -1 && "failed to open shared memory");
      } else {
        init_flag = 1;
      }
    }
  }
  assert(fid != -1);

  size_t mem_len = sizeof(shared_data_t);
  ret = ftruncate(fid, mem_len);
  assert(ret == 0 && "failed to ftruncate shared memory");

  auto *data = static_cast<shared_data_t *>(mmap_shared(mem_len, fid));
  if (init_flag) {
    shared_data_init(data);
  } else {
    if (data->is_inited == 0) {
      vkprintf(1, "somebody is trying to init shared data right now. lets wait for a second\n");
      sleep(1);
    }
    assert(data->is_inited == 1 && "shared data is not inited yet");

    // TODO: handle this without assert. Use magic in shared_data_t
    assert(data->magic == SHARED_DATA_MAGIC && "wrong magic in shared data");
  }

  vkprintf(1, "Get shared data: end\n");
  return data;
}

void shared_data_lock(shared_data_t *data) {
  int err = pthread_mutex_lock(&data->main_mutex);
  if (err != 0) {
    if (err == EOWNERDEAD) {
      vkprintf(1, "owner of shared memory mutex is dead. trying to make mutex and memory consitent\n");

      err = pthread_mutex_consistent(&data->main_mutex);
      assert(err == 0 && "failed to make mutex constistent_np");
    } else {
      assert(0 && "unknown mutex lock error");
    }
  }
}

void shared_data_unlock(shared_data_t *data) {
  int err = pthread_mutex_unlock(&data->main_mutex);
  assert(err == 0 && "unknown mutex unlock error");
}

void master_data_remove_if_dead(master_data_t *master) {
  if (master->is_alive) {
    unsigned long long start_time = get_pid_start_time(master->pid);
    if (start_time != master->start_time) {
      master->is_alive = false;
      dl_assert(me == nullptr || master != me, dl_pstr("[start_time = %llu] [master->start_time = %llu]", start_time, master->start_time));
    }
  }
}

void shared_data_update(shared_data_t *shared_data) {
  master_data_remove_if_dead(&shared_data->masters[0]);
  master_data_remove_if_dead(&shared_data->masters[1]);
}

void shared_data_get_masters(shared_data_t *shared_data, master_data_t **me, master_data_t **other) {
  *me = nullptr;

  if (!shared_data->masters[0].is_alive) {
    *me = &shared_data->masters[0];
    *other = &shared_data->masters[1];
  } else if (!shared_data->masters[1].is_alive) {
    *me = &shared_data->masters[1];
    *other = &shared_data->masters[0];
  }
}

void master_init(master_data_t *me, master_data_t *other) {
  assert(me != nullptr);
  memset(me, 0, sizeof(*me));

  if (other->is_alive) {
    me->rate = other->rate + 1;
  } else {
    me->rate = 1;
  }

  me->pid = getpid();
  me->start_time = get_pid_start_time(me->pid);
#if !defined(__APPLE__)
  assert(me->start_time != 0);
#endif

  if (other->is_alive) {
    me->generation = other->generation + 1;
  } else {
    me->generation = 1;
  }

  me->running_http_workers_n = 0;
  me->dying_http_workers_n = 0;
  me->to_kill = 0;

  me->own_http_fds = 0;
  me->http_ports_count = -1;

  me->ask_http_fds_generation = 0;
  me->sent_http_fds_generation = 0;

  me->instance_cache_elements_cached = 0;

  me->is_alive = true; // NB: must be the last operation.
}
