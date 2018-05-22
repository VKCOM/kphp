#include "compiler/bicycle.h"

#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>

#include "common/container_of.h"

__thread int bicycle_thread_id;

/*** Malloc hooks ***/
#define BICYCLE_MALLOC
#ifdef BICYCLE_MALLOC
extern "C" {
extern __typeof (malloc) __libc_malloc;
extern __typeof (free) __libc_free;
extern __typeof (calloc) __libc_calloc;
extern __typeof (realloc) __libc_realloc;

typedef struct block_t_tmp block_t;
struct block_t_tmp {
  union {
    size_t size;
    block_t *next;
  };
  char data[0];
};

#define MAX_BLOCK_SIZE 16384

class ZAllocatorRaw {
  private:
    char *buf;
    size_t left;
    block_t *free_blocks[MAX_BLOCK_SIZE >> 3];
  public:
    block_t *to_block (void *ptr) {
      return container_of(ptr, block_t, data);
    }
    void *from_block (block_t *block) {
      return &block->data;
    }

    block_t *block_alloc (size_t size) {
      size_t block_size = size + offsetof (block_t, data);
      block_t *res;
      if (size >= MAX_BLOCK_SIZE) {
        res = (block_t *)__libc_malloc (block_size);
      } else {
        size_t bucket_id = size >> 3;
        if (free_blocks[bucket_id] != NULL) {
          res = free_blocks[bucket_id];
          free_blocks[bucket_id] = res->next;
        } else {
          if (unlikely (left < block_size)) {
            assert (malloc != __libc_malloc);
            buf = (char *)__libc_malloc (1 << 26);
            left = 1 << 26;
          }
          res = (block_t *)buf;
          buf += block_size;
          left -= block_size;
        }
      }
      res->size = size;
      return res;
    }

    void block_free (block_t *block) {
      if (block->size >= MAX_BLOCK_SIZE) {
        __libc_free (block);
      } else {
        size_t bucket_id = block->size >> 3;
        block->next = free_blocks[bucket_id];
        free_blocks[bucket_id] = block;
      }
    }

    inline void *alloc (size_t size) {
      size = (size + 7) & -8;

      block_t *block = block_alloc (size);
      return from_block (block);
    }

    inline void *realloc (void *ptr, size_t new_size) {
      if (ptr == NULL) {
        return alloc (new_size);
      }
      if (new_size == 0) {
        free (ptr);
        return NULL;
      }
      block_t *block = to_block (ptr);
      size_t old_size = block->size;
      void *res = alloc (new_size);
      memcpy (res, ptr, min(old_size, new_size));
      block_free (block);
      return res;
    }

    inline void *calloc (size_t size_a, size_t size_b) {
      size_t size = size_a * size_b;
      void *res = alloc (size);
      memset (res, 0, size);
      return res;
    }

    inline void free (void *ptr) {
      if (ptr == NULL) {
        return;
      }
      block_t *block = to_block (ptr);
      block_free (block);
    }
};

TLS <ZAllocatorRaw> zallocator;

void *malloc (size_t size) {
  assert (malloc != __libc_malloc);
  return zallocator->alloc (size);
}
void *calloc (size_t size_a, size_t size_b) {
  return zallocator->calloc (size_a, size_b);
}
void *realloc (void *ptr, size_t size) {
  return zallocator->realloc (ptr, size);
}
__attribute__((error("memalign function is assert(0), don't use it")))
void *memalign (size_t aligment __attribute__((unused)), size_t size __attribute__((unused))) {
  assert (0);
  return NULL;
}
void free (void *ptr) {
  return zallocator->free (ptr);
}
}
#endif

int get_thread_id() {
  return bicycle_thread_id;
}
void set_thread_id (int new_thread_id) {
  bicycle_thread_id = new_thread_id;
}

SchedulerBase::SchedulerBase() {
  set_scheduler (this);
}
SchedulerBase::~SchedulerBase() {
  unset_scheduler (this);
}

OneThreadScheduler::OneThreadScheduler() {
  task_pull = new TaskPull();
}

void OneThreadScheduler::add_node (Node *node) {
  nodes.push_back (node);
}

void OneThreadScheduler::add_task (Task *task) {
  assert (task_pull != NULL);
  task_pull->add_task (task);
}

void OneThreadScheduler::add_sync_node (Node *node) {
  sync_nodes.push (node);
}

void OneThreadScheduler::set_threads_count (int new_threads_count __attribute__((unused))) {
  fprintf (stderr, "Using scheduler whithout threads, thread count is ignored");
}
void OneThreadScheduler::execute() {
  task_pull->add_to_scheduler (this);
  bool run_flag = true;
  while (run_flag) {
    run_flag = false;
    for (int i = 0; i < (int)nodes.size(); i++) {
      Task *task;
      while ((task = nodes[i]->get_task()) != NULL) {
        run_flag = true;
        task->execute();
        delete task;
      }
    }
    if (!run_flag && !sync_nodes.empty()) {
      sync_nodes.front()->on_finish();
      sync_nodes.pop();
      run_flag = true;
    }
  }
}

/*** ThreadLocalStorage ***/
void *thread_execute (void *arg) {
  ThreadLocalStorage *tls = (ThreadLocalStorage *) arg;
  tls->scheduler->thread_execute (tls);
  return NULL;
}

Scheduler::Scheduler()
  : threads_count(-1)
{
  task_pull = new TaskPull();
}

void Scheduler::add_node (Node *node) {
  if (node->is_parallel()) {
    nodes.push_back (node);
  } else {
    one_thread_nodes.push_back (node);
  }
}

void Scheduler::add_task (Task *task) {
  assert (task_pull != NULL);
  task_pull->add_task (task);
}

void Scheduler::add_sync_node (Node *node) {
  sync_nodes.push (node);
}

void Scheduler::execute() {
  task_pull->add_to_scheduler (this);
  set_thread_id (0);
  vector <ThreadLocalStorage> threads (threads_count + 1);

  assert ((int)one_thread_nodes.size() < threads_count);
  for (int i = 1; i <= threads_count; i++) {
    threads[i].thread_id = i;
    threads[i].scheduler = this;
    threads[i].run_flag = true;
    if (i <= (int)one_thread_nodes.size()) {
      threads[i].node = one_thread_nodes[i - 1];
    }
    pthread_create (&threads[i].pthread_id, NULL, ::thread_execute, &threads[i]);
  }

  while (true) {
    if (bicycle_counter > 0) {
      usleep (250);
      continue;
    }
    if (sync_nodes.empty()) {
      break;
    }
    sync_nodes.front()->on_finish();
    sync_nodes.pop();
  }

  for (int i = 1; i <= threads_count; i++) {
    threads[i].run_flag = false;
    __sync_synchronize();
    pthread_join (threads[i].pthread_id, NULL);
  }
}
void Scheduler::set_threads_count (int new_threads_count) {
  assert (1 <= new_threads_count && new_threads_count <= MAX_THREADS_COUNT);
  threads_count = new_threads_count;
}
bool Scheduler::thread_process_node (ThreadLocalStorage *tls, Node *node) {
  Task *task = node->get_task();
  if (task == NULL) {
    return false;
  }
  double st = dl_time();
  execute_task (task);
  double en = dl_time();
  tls->worked += en - st;
  return true;
}
void Scheduler::thread_execute (ThreadLocalStorage *tls) {
  tls->worked = 0;
  tls->started = dl_time();
  set_thread_id (tls->thread_id);

  while (tls->run_flag) {
    if (tls->node != NULL) {
      while (thread_process_node (tls, tls->node)) {
      }
    } else {
      for (int i = 0; i < (int)nodes.size(); i++) {
        Node *node = nodes[i];
        while (thread_process_node (tls, node)) {
        }
      }
    }
    usleep (250);
  }

  tls->finished = dl_time();
  //fprintf (stderr, "%lf\n", tls->worked / (tls->finished - tls->started));
}
static SchedulerBase *scheduler;

void set_scheduler (SchedulerBase *new_scheduler) {
  assert (scheduler == NULL);
  scheduler = new_scheduler;
}

void unset_scheduler (SchedulerBase *old_scheduler) {
  assert (scheduler == old_scheduler);
  scheduler = NULL;
}

SchedulerBase *get_scheduler() {
  assert (scheduler != NULL);
  return scheduler;
}

volatile int bicycle_counter;

TLS <Profiler> profiler;


typedef char pstr_buff_t[5000];
TLS <pstr_buff_t> pstr_buff;
char* bicycle_dl_pstr (char const *msg, ...) {
  pstr_buff_t &s = *pstr_buff;
  va_list args;

  va_start (args, msg);
  vsnprintf (s, 5000, msg, args);
  va_end (args);

  return s;
}
