// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/resumable.h"

#include "common/kprintf.h"

#include "runtime/net_events.h"
#include "server/php-queries.h"
#include "runtime/kphp_tracing.h"

DEFINE_VERBOSITY(resumable);

bool resumable_finished;

const char *last_wait_error;

static int64_t runned_resumable_id;

static bool in_main_thread() noexcept {
  return runned_resumable_id == 0;
}

static void debug_print_resumables() noexcept;

Storage *Resumable::input_;
Storage *Resumable::output_;

static Storage *get_storage(int64_t resumable_id);

bool check_started_storage(Storage *s);
bool check_forked_storage(Storage *s);

static inline void update_current_resumable_id(int64_t new_id, bool is_internal);

bool Resumable::resume(int64_t resumable_id, Storage *input) {
  php_assert(!input || check_started_storage(input) || check_forked_storage(input));
  int64_t parent_id = runned_resumable_id;

  input_ = input;
  update_current_resumable_id(resumable_id, is_internal_resumable());

  bool res = run();

  // must not be used
  input_ = nullptr;
  update_current_resumable_id(parent_id, is_internal_resumable());

  return res;
}

void Resumable::update_output() {
  output_ = in_main_thread() ? nullptr : get_storage(runned_resumable_id);
}

int64_t first_forked_resumable_id;

namespace {

struct resumable_info {
  Storage output;
  Resumable *continuation;
  int64_t son;
  const char *name;
};

struct forked_resumable_info : resumable_info {
  // 0 - default
  // 2 * 10^9 > x > 10^8 - waited by x
  // 10^8 > x > 0 - in queue x
  // -1 - if answer received and not in queue
  // x < 0 - (-id) of next finished function in the same queue or -2 if none
  int64_t queue_id;
  double running_time;
};

struct started_resumable_info : resumable_info {
  // x > 0 - has parent x
  // 0 - in main thread
  // -1 - finished
  int64_t parent_id;
  int64_t fork_id;
};

int64_t first_array_forked_resumable_id;
int64_t current_forked_resumable_id = 1123456789;
forked_resumable_info *forked_resumables;
forked_resumable_info gotten_forked_resumable_info;
uint32_t forked_resumables_size;

int64_t first_started_resumable_id;
int64_t current_started_resumable_id = 123456789;
started_resumable_info *started_resumables;
uint32_t started_resumables_size;
int64_t first_free_started_resumable_id;

struct wait_queue {
  int64_t first_finished_function;
  int32_t left_functions;
  // 0 - default
  // x > 0 - id of wait_queue_next resumable
  // x < 0 - next free queue_id
  int64_t resumable_id;
};

wait_queue *wait_queues;
uint32_t wait_queues_size;
int64_t wait_next_queue_id;

int64_t *finished_resumables;
uint32_t finished_resumables_size;
uint32_t finished_resumables_count;

int64_t *yielded_resumables;
uint32_t yielded_resumables_l;
uint32_t yielded_resumables_r;
uint32_t yielded_resumables_size;

inline bool is_forked_resumable_id(int64_t resumable_id) {
  return first_forked_resumable_id <= resumable_id && resumable_id < current_forked_resumable_id;
}

inline forked_resumable_info *get_forked_resumable_info(int64_t resumable_id) {
  php_assert(is_forked_resumable_id(resumable_id));
  if (resumable_id < first_array_forked_resumable_id) {
    return &gotten_forked_resumable_info;
  }
  return &forked_resumables[resumable_id - first_array_forked_resumable_id];
}

inline bool is_started_resumable_id(int64_t resumable_id) {
  return first_started_resumable_id <= resumable_id && resumable_id < current_started_resumable_id;
}

inline started_resumable_info *get_started_resumable_info(int64_t resumable_id) {
  php_assert(is_started_resumable_id(resumable_id));
  return &started_resumables[resumable_id - first_started_resumable_id];
}

inline bool is_wait_queue_id(int64_t queue_id) {
  return 0 < queue_id && queue_id <= wait_next_queue_id && wait_queues[queue_id - 1].resumable_id >= 0;
}

inline wait_queue *get_wait_queue(int64_t queue_id) {
  php_assert(is_wait_queue_id(queue_id));
  return &wait_queues[queue_id - 1];
}

Storage *get_started_storage(int64_t resumable_id) noexcept {
  return &get_started_resumable_info(resumable_id)->output;
}

} // namespace

Storage *get_forked_storage(int64_t resumable_id) {
  return &get_forked_resumable_info(resumable_id)->output;
}

bool check_started_storage(Storage *storage) {
  return static_cast<void *>(started_resumables) <= static_cast<void *>(storage) &&
         static_cast<void *>(storage) < static_cast<void *>(started_resumables + started_resumables_size);
}

bool check_forked_storage(Storage *storage) {
  return static_cast<void *>(forked_resumables) <= static_cast<void *>(storage) &&
         static_cast<void *>(storage) < static_cast<void *>(forked_resumables + forked_resumables_size);
}

static Storage *get_storage(int64_t resumable_id) {
  if (resumable_id > 1000000000) {
    return get_forked_storage(resumable_id);
  } else {
    return get_started_storage(resumable_id);
  }
}

int64_t register_forked_resumable(Resumable *resumable) {
  if (current_forked_resumable_id == first_array_forked_resumable_id + forked_resumables_size) {
    uint32_t first_needed_id = 0;
    while (first_needed_id < forked_resumables_size &&
           forked_resumables[first_needed_id].queue_id == -1 &&
           forked_resumables[first_needed_id].output.tag == 0) {
      first_needed_id++;
    }
    if (first_needed_id > forked_resumables_size / 2) {
      memcpy(forked_resumables, forked_resumables + first_needed_id, sizeof(forked_resumable_info) * (forked_resumables_size - first_needed_id));
      first_array_forked_resumable_id += first_needed_id;
    } else {
      forked_resumables = static_cast<forked_resumable_info *>(dl::reallocate(forked_resumables, sizeof(forked_resumable_info) * 2 * forked_resumables_size, sizeof(forked_resumable_info) * forked_resumables_size));
      forked_resumables_size *= 2;
    }
    Resumable::update_output();
  }
  if (current_forked_resumable_id >= 2000000000) {
    php_critical_error("too many forked resumables");
  }

  int64_t res_id = current_forked_resumable_id++;
  forked_resumable_info *res = get_forked_resumable_info(res_id);

  new(&res->output) Storage();
  res->continuation = resumable;
  res->queue_id = 0;
  res->son = 0;
  res->running_time = 0;
  res->name = resumable ? typeid(*resumable).name() : "(null)";

  return res_id;
}

static inline void update_current_resumable_id(int64_t new_id, bool is_internal) {
  int64_t old_running_fork = f$get_running_fork_id();
  runned_resumable_id = new_id;
  int64_t new_running_fork = f$get_running_fork_id();
  if (new_running_fork != old_running_fork) {
    update_precise_now();
    if (old_running_fork) {
      get_forked_resumable_info(old_running_fork)->running_time += get_precise_now();
    }
    if (new_running_fork) {
      get_forked_resumable_info(new_running_fork)->running_time -= get_precise_now();
    }
    if (!is_internal && kphp_tracing::is_turned_on()) {
      kphp_tracing::on_fork_switch(new_running_fork);
    }
  }
  Resumable::update_output();
}

int64_t f$get_running_fork_id() {
  if (in_main_thread()) {
    return 0;
  }
  if (is_forked_resumable_id(runned_resumable_id)) {
    return runned_resumable_id;
  }
  if (is_started_resumable_id(runned_resumable_id)) {
    return get_started_resumable_info(runned_resumable_id)->fork_id;
  }
  php_assert (false);
  return 0;
}

Optional<array<mixed>> f$get_fork_stat(int64_t fork_id) {
  auto *info = get_forked_resumable_info(fork_id);
  if (!info) {
    return false;
  }
  if (info->queue_id == -1 && info->output.tag == 0) {
    return false;
  }

  array<mixed> result;
  result.set_value(string("id"), fork_id);
  result.set_value(string("name"), string(info->name));
  if (info->queue_id < 0) {
    result.set_value(string("state"), string("finished"));
  } else if (fork_id == f$get_running_fork_id()){
    result.set_value(string("state"), string("running"));
  } else {
    result.set_value(string("state"), string("blocked"));
  }
  double running_time = info->running_time;
  if (fork_id == f$get_running_fork_id()) {
    running_time += get_precise_now();
  }
  result.set_value(string("work_time"), running_time);
  return result;
}

static int64_t register_started_resumable(Resumable *resumable) noexcept {
  int64_t res_id;
  bool is_new = false;
  if (first_free_started_resumable_id) {
    res_id = first_free_started_resumable_id;
    first_free_started_resumable_id = get_started_resumable_info(first_free_started_resumable_id)->parent_id;
  } else {
    if (current_started_resumable_id == first_started_resumable_id + started_resumables_size) {
      started_resumables = static_cast<started_resumable_info *>(dl::reallocate(started_resumables, sizeof(started_resumable_info) * 2 * started_resumables_size, sizeof(started_resumable_info) * started_resumables_size));
      started_resumables_size *= 2;
    }

    res_id = current_started_resumable_id++;
    is_new = true;

    Resumable::update_output();
  }
  if (res_id >= 1000000000) {
    php_critical_error("too many started resumables");
  }

  started_resumable_info *res = get_started_resumable_info(res_id);

  php_assert(is_new || res->output.tag == 0);

  new(&res->output) Storage();
  res->continuation = resumable;
  res->parent_id = runned_resumable_id;
  res->fork_id = f$get_running_fork_id();
  res->son = 0;
  res->name = resumable ? typeid(*resumable).name() : "(null)";

  if (!in_main_thread()) {
    auto *parent = is_started_resumable_id(runned_resumable_id)
                   ? static_cast<resumable_info *>(get_started_resumable_info(runned_resumable_id))
                   : static_cast<resumable_info *>(get_forked_resumable_info(runned_resumable_id));
    if (parent->son != 0) {
      kprintf("Tring to change %" PRIi64 "->son from %" PRIi64 " to %" PRIi64 "\n", runned_resumable_id, parent->son, res_id);
      debug_print_resumables();
      php_assert(parent->son == 0);
    }
    parent->son = res_id;
  }

  return res_id;
}


Resumable *get_forked_resumable(int64_t resumable_id) {
  return get_forked_resumable_info(resumable_id)->continuation;
}

Resumable *get_started_resumable(int64_t resumable_id) {
  return get_started_resumable_info(resumable_id)->continuation;
}


static void add_resumable_to_queue(int64_t resumable_id, forked_resumable_info *resumable) {
  int64_t queue_id = resumable->queue_id;
  wait_queue *q = get_wait_queue(queue_id);
  tvkprintf(resumable, 1, "Push resumable %" PRIi64 " to queue %" PRIi64 "(%" PRIi64 ", %" PRIi64 ", %" PRIi32 ") at %.6lf\n",
            resumable_id, resumable->queue_id, q->resumable_id, q->first_finished_function, q->left_functions, (update_precise_now(), get_precise_now()));

  resumable->queue_id = q->first_finished_function;
  q->first_finished_function = -resumable_id;

  if (q->resumable_id) {
    resumable_run_ready(q->resumable_id);
    q = get_wait_queue(queue_id); // can be reallocated in run_ready
  }

  q->left_functions--;
}

static void free_resumable_continuation(resumable_info *res) noexcept {
  php_assert(res->continuation);
  delete res->continuation;
  res->continuation = nullptr;
}

static void finish_forked_resumable(int64_t resumable_id) noexcept {
  forked_resumable_info *res = get_forked_resumable_info(resumable_id);
  bool is_internal = res->continuation->is_internal_resumable();

  free_resumable_continuation(res);
  if (!is_internal && kphp_tracing::is_turned_on()) {
    kphp_tracing::on_fork_finish(resumable_id);
  }

  if (res->queue_id > 100000000) {
    php_assert(is_started_resumable_id(res->queue_id));
    int64_t wait_resumable_id = res->queue_id;
    res->queue_id = -1;
    resumable_run_ready(wait_resumable_id);
  } else if (res->queue_id > 0) {
    add_resumable_to_queue(resumable_id, res);
  } else {
    res->queue_id = -1;
  }

  php_assert(get_forked_resumable_info(resumable_id)->queue_id < 0);
}

static void finish_started_resumable(int64_t resumable_id) {
  started_resumable_info *res = get_started_resumable_info(resumable_id);
  free_resumable_continuation(res);
}

void unregister_started_resumable_debug_hack(int64_t resumable_id) {
  started_resumable_info *res = get_started_resumable_info(resumable_id);
  php_assert(res->continuation == nullptr);

  res->parent_id = first_free_started_resumable_id;
  first_free_started_resumable_id = resumable_id;
}

static void unregister_started_resumable(int64_t resumable_id) noexcept {
  started_resumable_info *res = get_started_resumable_info(resumable_id);
  if (res->parent_id > 0) {
    auto *parent = is_started_resumable_id(res->parent_id)
                   ? static_cast<resumable_info *>(get_started_resumable_info(res->parent_id))
                   : static_cast<resumable_info *>(get_forked_resumable_info(res->parent_id));
    if (parent->son != resumable_id && parent->son != 0) {
      kprintf("Tring to change %" PRIi64 "->son from %" PRIi64 " to %d\n", res->parent_id, resumable_id, 0);
      debug_print_resumables();
      php_assert(parent->son == resumable_id || parent->son == 0);
    }
    parent->son = 0;
  }
  php_assert(res->continuation == nullptr);

  res->parent_id = first_free_started_resumable_id;
  first_free_started_resumable_id = resumable_id;
}


static bool resumable_has_finished() {
  return finished_resumables_count > 0 || yielded_resumables_l != yielded_resumables_r;
}

static void yielded_resumables_push(int64_t id) {
  yielded_resumables[yielded_resumables_r] = id;
  yielded_resumables_r++;
  if (yielded_resumables_r == yielded_resumables_size) {
    yielded_resumables_r = 0;
  }
  if (yielded_resumables_r == yielded_resumables_l) {
    yielded_resumables = static_cast<int64_t *>(dl::reallocate(yielded_resumables, sizeof(int64_t) * 2 * yielded_resumables_size, sizeof(int64_t) * yielded_resumables_size));
    memcpy(yielded_resumables + yielded_resumables_size, yielded_resumables, sizeof(int64_t) * yielded_resumables_r);
    yielded_resumables_r += yielded_resumables_size;
    yielded_resumables_size *= 2;
  }
}

static int64_t yielded_resumables_pop() {
  php_assert(yielded_resumables_l != yielded_resumables_r);
  int64_t result = yielded_resumables[yielded_resumables_l];
  yielded_resumables_l++;
  if (yielded_resumables_l == yielded_resumables_size) {
    yielded_resumables_l = 0;
  }
  return result;
}


static void resumable_add_finished(int64_t resumable_id) {
  php_assert(is_started_resumable_id(resumable_id));
  if (finished_resumables_count >= finished_resumables_size) {
    php_assert(finished_resumables_count == finished_resumables_size);
    finished_resumables = static_cast<int64_t *>(
      dl::reallocate(finished_resumables,
                     sizeof(int64_t) * 2 * finished_resumables_size,
                     sizeof(int64_t) * finished_resumables_size));
    finished_resumables_size *= 2;
  }

  tvkprintf(resumable, 1, "Resumable %" PRIi64 " put to position %" PRIu32 " of finished list\n", resumable_id, finished_resumables_count);
  finished_resumables[finished_resumables_count++] = resumable_id;
}

static void resumable_get_finished(int64_t *resumable_id, bool *is_yielded) {
  php_assert(resumable_has_finished());
  if (finished_resumables_count) {
    *resumable_id = finished_resumables[--finished_resumables_count];
    *is_yielded = false;
  } else {
    *resumable_id = yielded_resumables_pop();
    *is_yielded = true;
  }
}

static void debug_print_resumables() noexcept {
  fprintf(stderr, "first free resumable id: %" PRIi64 "\n", first_free_started_resumable_id);
  fprintf(stderr, "finished:");
  for (uint32_t i = 0; i < finished_resumables_count; i++) {
    fprintf(stderr, " %" PRIi64, finished_resumables[i]);
  }
  fprintf(stderr, "\n");
  for (int64_t i = first_started_resumable_id; i < current_started_resumable_id; i++) {
    started_resumable_info *info = get_started_resumable_info(i);
    fprintf(stderr, "started id = %" PRIi64 ", parent = %" PRIi64 ", fork = %" PRIi64 ", son = %" PRIi64 ", continuation = %s, name = %s\n", i, info->parent_id, info->fork_id, info->son,
            info->continuation ? typeid(*info->continuation).name() : "(none)", info->name ? info->name : "()");
  }
  for (int64_t i = first_forked_resumable_id; i < current_forked_resumable_id; i++) {
    forked_resumable_info *info = get_forked_resumable_info(i);
    fprintf(stderr, "forked id = %" PRIi64 ", queue_id = %" PRIi64 ", son = %" PRIi64 ", continuation = %s, name = %s\n", i, info->queue_id, info->son,
            info->continuation ? typeid(*info->continuation).name() : "(none)", info->name ? info->name : "()");
  }
}

static bool wait_started_resumable(int64_t resumable_id) noexcept;

Storage *start_resumable_impl(Resumable *resumable) noexcept {
  check_script_timeout();
  int64_t id = register_started_resumable(resumable);

  if (resumable->resume(id, nullptr)) {
    Storage *output = get_started_storage(id);
    finish_started_resumable(id);
    unregister_started_resumable(id);
    resumable_finished = true;
    return output;
  }

  if (in_main_thread()) {
    php_assert (wait_started_resumable(id));
    Storage *output = get_started_storage(id);
    resumable_finished = true;
    unregister_started_resumable(id);
    return output;
  }

  resumable_finished = false;
  return nullptr;
}

int64_t fork_resumable(Resumable *resumable) noexcept {
  int64_t id = register_forked_resumable(resumable);

  if (kphp_tracing::is_turned_on()) {
    kphp_tracing::on_fork_start(id);
    if (unlikely(kphp_tracing::cur_trace_level >= 2)) {
      kphp_tracing::on_fork_provide_name(id, string{typeid(*resumable).name()});
    }
  }

  if (resumable->resume(id, nullptr)) {
    finish_forked_resumable(id);
  }

  return id;
}

static void continue_resumable(resumable_info *res, int64_t resumable_id) noexcept {
  if (unlikely(res->son)) {
    debug_print_resumables();
    php_assert(res->son == 0);
  }
  const bool is_done = res->continuation->resume(resumable_id, nullptr);
  php_assert(is_done);
}

void resumable_run_ready(int64_t resumable_id) {
  tvkprintf(resumable, 1, "Run ready %" PRIi64 "\n", resumable_id);
  if (resumable_id > 1000000000) {
    forked_resumable_info *res = get_forked_resumable_info(resumable_id);
    php_assert(res->queue_id >= 0);
    continue_resumable(res, resumable_id);
    finish_forked_resumable(resumable_id);
  } else {
    started_resumable_info *res = get_started_resumable_info(resumable_id);
    continue_resumable(res, resumable_id);
    finish_started_resumable(resumable_id);
    resumable_add_finished(resumable_id);
  }
}

#if defined(__APPLE__)
void run_scheduler(double timeout);
#else
void run_scheduler(double timeout) __attribute__((section("run_scheduler_section")));
#endif

static int64_t scheduled_resumable_id = 0;

void run_scheduler(double dead_line_time) {
  tvkprintf(resumable, 1, "Run scheduler %" PRIu32 "\n", finished_resumables_count);
  int32_t left_resumables = 1000;
  bool force_run_next = false;
  while (resumable_has_finished() && --left_resumables >= 0) {
    if (force_run_next) {
      force_run_next = false;
    } else if (get_precise_now() > dead_line_time) {
      return;
    }

    int64_t resumable_id;
    bool is_yielded;
    resumable_get_finished(&resumable_id, &is_yielded);
    if (is_yielded) {
      left_resumables = 0;
    }

    started_resumable_info *res = get_started_resumable_info(resumable_id);
    php_assert(res->continuation == nullptr);

    tvkprintf(resumable, 1, "Process %" PRIi64 "(%s) with parent %" PRIi64 " in scheduler\n",
              resumable_id, is_yielded ? "yielded" : "not yielded", res->parent_id);
    int64_t parent_id = res->parent_id;
    if (parent_id == 0) {
      res->parent_id = -1;
      return;
    }

    php_assert(parent_id > 0);
    scheduled_resumable_id = parent_id;
    if (parent_id < 1000000000) {
      started_resumable_info *parent = get_started_resumable_info(parent_id);
      if (parent->continuation == nullptr || parent->son != resumable_id) {
        kprintf("Will fail assert with resumbale_id = %" PRIi64 "\n", resumable_id);
        debug_print_resumables();
      }
      php_assert(parent->son == resumable_id);
      php_assert(parent->continuation != nullptr);
      php_assert(parent->parent_id != -2);
      parent->son = 0;
      if (parent->continuation->resume(parent_id, &res->output)) {
        finish_started_resumable(parent_id);
        resumable_add_finished(parent_id);
        left_resumables++;
        force_run_next = true;
      }
    } else {
      forked_resumable_info *parent = get_forked_resumable_info(parent_id);
      if (parent->continuation == nullptr || parent->son != resumable_id) {
        kprintf("Will fail assert with resumbale_id = %" PRIi64 "\n", resumable_id);
        debug_print_resumables();
      }
      php_assert(parent->son == resumable_id);
      php_assert(parent->continuation != nullptr);
      php_assert(parent->queue_id >= 0);
      parent->son = 0;
      if (parent->continuation->resume(parent_id, &res->output)) {
        finish_forked_resumable(parent_id);
      }
    }
    scheduled_resumable_id = 0;

    unregister_started_resumable_debug_hack(resumable_id);
  }
}


void wait_without_result_synchronously(int64_t resumable_id) {
  forked_resumable_info *resumable = get_forked_resumable_info(resumable_id);

  if (resumable->queue_id < 0) {
    return;
  }

  update_precise_now();
  while (wait_net(MAX_TIMEOUT_MS) && resumable->queue_id >= 0) {
    update_precise_now();
  }
}

bool wait_without_result_synchronously_safe(int64_t resumable_id) {
  last_wait_error = nullptr;
  if (!is_forked_resumable_id(resumable_id)) {
    last_wait_error = "Wrong resumable id";
    return false;
  }

  forked_resumable_info *resumable = get_forked_resumable_info(resumable_id);

  if (resumable->queue_id < 0) {
    return true;
  }

  if (resumable->queue_id > 0) {
    last_wait_error = "Someone already waits for this resumable";
    return false;
  }

  wait_without_result_synchronously(resumable_id);
  return true;
}


static bool wait_forked_resumable(int64_t resumable_id, double dead_line_time) {
  php_assert(dead_line_time > get_precise_now()); // TODO remove asserts
  php_assert(in_main_thread()); // TODO remove asserts
  php_assert(is_forked_resumable_id(resumable_id));

  forked_resumable_info *resumable = get_forked_resumable_info(resumable_id);

  do {
    if (resumable->queue_id < 0) {
      return true;
    }

    run_scheduler(dead_line_time);

    resumable = get_forked_resumable_info(resumable_id);//can change in scheduler
    if (resumable->queue_id < 0) {
      return true;
    }

    if (get_precise_now() > dead_line_time) {
      return false;
    }
    update_precise_now();

    if (resumable_has_finished()) {
      wait_net(0);
      continue;
    }
  } while (resumable_has_finished() || wait_net(timeout_convert_to_ms(dead_line_time - get_precise_now())));

  return false;
}

static bool wait_started_resumable(int64_t resumable_id) noexcept {
  php_assert(in_main_thread()); // TODO remove asserts
  php_assert(is_started_resumable_id(resumable_id));

  started_resumable_info *resumable = get_started_resumable_info(resumable_id);

  do {
    php_assert(resumable->parent_id == 0);
    run_scheduler(get_precise_now() + MAX_TIMEOUT);

    // can change in scheduler
    resumable = get_started_resumable_info(resumable_id);
    if (resumable->parent_id == -1) {
      return true;
    }

    update_precise_now();

    if (resumable_has_finished()) {
      wait_net(0);
      continue;
    }
  } while (resumable_has_finished() || wait_net(MAX_TIMEOUT_MS));

  return false;
}

class ResumableWithTimer : public Resumable {
protected:
  void set_timer(double timeout, int32_t wakeup_callback_id, int64_t wakeup_extra) noexcept {
    php_assert(static_cast<int32_t>(wakeup_extra) == wakeup_extra);
    timer_ = allocate_event_timer(timeout, wakeup_callback_id, static_cast<int32_t>(wakeup_extra));
  }

  void remove_timer() noexcept {
    if (timer_) {
      remove_event_timer(timer_);
      timer_ = nullptr;
    }
  }

private:
  kphp_event_timer *timer_{nullptr};
};

static int32_t wait_timeout_wakeup_id = -1;

class wait_resumable final : public ResumableWithTimer {
public:
  bool is_internal_resumable() const noexcept final {
    return true;
  }

  explicit wait_resumable(int64_t child_id) noexcept:
    child_id_(child_id) {
  }

  void set_timer(double timeout, int64_t wakeup_extra) noexcept {
    ResumableWithTimer::set_timer(timeout, wait_timeout_wakeup_id, wakeup_extra);
  }

private:
  bool run() final {
    remove_timer();

    forked_resumable_info *info = get_forked_resumable_info(child_id_);

    if (info->queue_id < 0) {
      output_->save<bool>(true);
    } else {
      php_assert(input_ == nullptr);
      info->queue_id = 0;
      output_->save<bool>(false);
    }

    child_id_ = -1;
    return true;
  }

  int64_t child_id_;
};

class wait_concurrently_resumable final : public Resumable {
public:
  explicit wait_concurrently_resumable(int64_t child_id) noexcept:
    child_id_(child_id) {
  }

private:
  bool run() final {
    RESUMABLE_BEGIN
      while (true) {
        info_ = get_forked_resumable_info(child_id_);

        if (info_->queue_id < 0) {
          break;
        }
        if (!resumable_has_finished()) {
          wait_net(MAX_TIMEOUT);
        }
        f$sched_yield();
        TRY_WAIT_DROP_RESULT(wait_many_resumable_label1, void);
      }

      output_->save<bool>(true);
      child_id_ = -1;
      return true;
    RESUMABLE_END
  }

  int64_t child_id_;
  forked_resumable_info *info_{nullptr};
};


static void process_wait_timeout(kphp_event_timer *timer) {
  int64_t wait_resumable_id = timer->wakeup_extra;
  php_assert(is_started_resumable_id(wait_resumable_id));

  resumable_run_ready(wait_resumable_id);
}

bool wait_without_result(int64_t resumable_id, double timeout) {
  resumable_finished = true;

  last_wait_error = nullptr;
  if (!is_forked_resumable_id(resumable_id)) {
    last_wait_error = "Wrong resumable id";
    return false;
  }

  bool has_timeout = true;
  if (timeout <= 0 || timeout > MAX_TIMEOUT) {
    has_timeout = false;
    timeout = MAX_TIMEOUT;
  } else {
    update_precise_now();
  }
  if (timeout != 0.0) {
    timeout += get_precise_now();
  } else {
    wait_net(0);
  }

  forked_resumable_info *resumable = get_forked_resumable_info(resumable_id);

  if (resumable->queue_id < 0) {
    return true;
  }

  if (resumable->queue_id > 0) {
    int waiter_id = resumable->queue_id;
    Resumable *waiter_fork = nullptr;
    Resumable *waiter_resumable = nullptr;
    int fork_id = -1;
    if (is_started_resumable_id(waiter_id)) {
      auto *info = get_started_resumable_info(waiter_id);
      waiter_resumable = info->continuation;
      fork_id = info->fork_id;
      if (fork_id != 0) {
        waiter_fork = get_forked_resumable_info(fork_id)->continuation;
      }
    }
    php_warning("Resumable is already waited by other thread: waiter=%d(%s), fork=%d(%s), running_resumable_id=%d",
                waiter_id, waiter_resumable ? typeid(*waiter_resumable).name() : "null", fork_id, waiter_fork ? typeid(*waiter_fork).name() : "null",
                static_cast<int>(runned_resumable_id));
    last_wait_error = "Someone already waits for this resumable";
    return false;
  }

  if (timeout == 0.0) {
    last_wait_error = "Zero timeout in wait";
    return false;
  }

  if (in_main_thread()) {
    if (!wait_forked_resumable(resumable_id, timeout)) {
      last_wait_error = "Timeout in wait";
      return false;
    }

    return true;
  }

  wait_resumable *res = new wait_resumable(resumable_id);
  resumable->queue_id = register_started_resumable(res);
  if (has_timeout) {
    res->set_timer(timeout, resumable->queue_id);
  }

  resumable_finished = false;
  return false;
}

bool f$wait_concurrently(int64_t resumable_id) {
  resumable_finished = true;

  last_wait_error = nullptr;
  if (!is_forked_resumable_id(resumable_id)) {
    last_wait_error = "Wrong resumable id";
    return false;
  }

  wait_net(0);

  forked_resumable_info *resumable = get_forked_resumable_info(resumable_id);

  if (resumable->queue_id < 0) {
    return true;
  }

  if (resumable->queue_id > 0) {
    return start_resumable<bool>(new wait_concurrently_resumable(resumable_id));
  }

  return wait_without_result(resumable_id);
}

static int64_t wait_queue_push(int64_t queue_id, int64_t resumable_id) {
  if (!is_forked_resumable_id(resumable_id)) {
    php_warning("Wrong resumable_id %" PRIi64 " in function wait_queue_push", resumable_id);
    return queue_id;
  }

  forked_resumable_info *resumable = get_forked_resumable_info(resumable_id);
  if (resumable->queue_id != 0 && resumable->queue_id != -1) {
    php_warning("Someone already waits resumable %" PRIi64, resumable_id);
    return queue_id;
  }

  if (resumable->queue_id < 0 && resumable->output.tag == 0) {
    return queue_id;
  }

  if (queue_id == -1) {
    queue_id = f$wait_queue_create();
  } else if (!is_wait_queue_id(queue_id)) {
    php_warning("Wrong queue_id %" PRIi64, queue_id);
    return queue_id;
  }

  wait_queue *q = get_wait_queue(queue_id);
  if (resumable->queue_id == 0) {
    resumable->queue_id = queue_id;

    tvkprintf(resumable, 1, "Link resumable %" PRIi64 " with queue %" PRIi64 " at %.6lf\n",
              resumable_id, queue_id, (update_precise_now(), get_precise_now()));
    q->left_functions++;
  } else {
    resumable->queue_id = q->first_finished_function;
    q->first_finished_function = -resumable_id;
  }

  return queue_id;
}

int64_t wait_queue_push_unsafe(int64_t queue_id, int64_t resumable_id) {
  if (queue_id == -1) {
    queue_id = f$wait_queue_create();
  }

  forked_resumable_info *resumable = get_forked_resumable_info(resumable_id);
  wait_queue *q = get_wait_queue(queue_id);
  if (resumable->queue_id == 0) {
    resumable->queue_id = queue_id;
    q->left_functions++;
  } else {
    resumable->queue_id = q->first_finished_function;
    q->first_finished_function = -resumable_id;
  }

  return queue_id;
}

static int64_t first_free_queue_id;

void unregister_wait_queue(int64_t queue_id) {
  if (queue_id != -1) {
    get_wait_queue(queue_id)->resumable_id = -first_free_queue_id - 1;
    first_free_queue_id = queue_id;
  }
}

int64_t f$wait_queue_create() {
  int64_t res_id;
  if (first_free_queue_id != 0) {
    res_id = first_free_queue_id;
    first_free_queue_id = -wait_queues[res_id - 1].resumable_id - 1;
    php_assert(0 <= first_free_queue_id && first_free_queue_id <= wait_next_queue_id);
  } else {
    if (wait_next_queue_id >= wait_queues_size) {
      php_assert(wait_next_queue_id == wait_queues_size);
      wait_queues = static_cast<wait_queue *>(dl::reallocate(wait_queues, sizeof(wait_queue) * 2 * wait_queues_size, sizeof(wait_queue) * wait_queues_size));
      wait_queues_size *= 2;
    }
    res_id = ++wait_next_queue_id;
  }
  if (wait_next_queue_id >= 99999999) {
    php_critical_error("too many wait queues");
  }

  wait_queue *q = &wait_queues[res_id - 1];
  q->first_finished_function = -2;
  q->left_functions = 0;
  q->resumable_id = 0;

  return res_id;
}

int64_t f$wait_queue_create(const mixed &resumable_ids) {
  return f$wait_queue_push(-1, resumable_ids);
}

int64_t f$wait_queue_push(int64_t queue_id, const mixed &resumable_ids) {
  if (queue_id == -1) {
    queue_id = f$wait_queue_create();
  }

  if (resumable_ids.is_array()) {
    for (auto p : resumable_ids) {
      wait_queue_push(queue_id, f$intval(p.get_value()));
    }
    return queue_id;
  }
  return wait_queue_push(queue_id, f$intval(resumable_ids));
}

int64_t wait_queue_create(const array<int64_t> &resumable_ids) {
  if (resumable_ids.count() == 0) {
    return -1;
  }

  const int64_t queue_id = f$wait_queue_create();
  for (auto p : resumable_ids) {
    wait_queue_push(queue_id, p.get_value());
  }
  return queue_id;
}


static void wait_queue_skip_gotten(wait_queue *q) {
  while (q->first_finished_function != -2) {
    forked_resumable_info *resumable = get_forked_resumable_info(-q->first_finished_function);
    if (resumable->output.tag != 0) {
      break;
    }
    q->first_finished_function = resumable->queue_id;
    resumable->queue_id = -1;
    php_assert(q->first_finished_function != -1);
  }
}

bool f$wait_queue_empty(int64_t queue_id) {
  if (!is_wait_queue_id(queue_id)) {
    if (queue_id != -1) {
      php_warning("Wrong queue_id %" PRIi64 " in function wait_queue_empty", queue_id);
    }
    return true;
  }

  wait_queue *q = get_wait_queue(queue_id);
  wait_queue_skip_gotten(q);
  return q->left_functions == 0 && q->first_finished_function == -2;
}

static void wait_queue_next(int64_t queue_id, double timeout) {
  php_assert(timeout > get_precise_now()); // TODO remove asserts
  php_assert(in_main_thread()); // TODO remove asserts
  php_assert(is_wait_queue_id(queue_id));

  wait_queue *q = get_wait_queue(queue_id);

  do {
    if (q->first_finished_function != -2) {
      return;
    }

    run_scheduler(timeout);
    // can change in scheduler
    q = get_wait_queue(queue_id);
    wait_queue_skip_gotten(q);
    if (q->first_finished_function != -2 || q->left_functions == 0 || get_precise_now() > timeout) {
      return;
    }

    update_precise_now();
    if (resumable_has_finished()) {
      wait_net(0);
      continue;
    }
  } while (resumable_has_finished() || wait_net(timeout_convert_to_ms(timeout - get_precise_now())));
}

Optional<int64_t> wait_queue_next_synchronously(int64_t queue_id) {
  wait_queue *q = get_wait_queue(queue_id);
  wait_queue_skip_gotten(q);

  if (q->first_finished_function != -2) {
    return -q->first_finished_function;
  }

  if (q->left_functions == 0) {
    return 0;
  }

  update_precise_now();
  while (wait_net(MAX_TIMEOUT * 1000) && q->first_finished_function == -2) {
    update_precise_now();
  }

  return q->first_finished_function == -2 ? 0 : -q->first_finished_function;
}

static int32_t wait_queue_timeout_wakeup_id = -1;

class wait_queue_resumable final : public ResumableWithTimer {
public:
  bool is_internal_resumable() const noexcept final {
    return true;
  }

  explicit wait_queue_resumable(int64_t queue_id) noexcept:
    queue_id_(queue_id) {
  }

  void set_timer(double timeout, int64_t wakeup_extra) noexcept {
    ResumableWithTimer::set_timer(timeout, wait_queue_timeout_wakeup_id, wakeup_extra);
  }

private:
  using ReturnT = Optional<int64_t>;

  bool run() final {
    remove_timer();
    wait_queue *q = get_wait_queue(queue_id_);

    php_assert(q->resumable_id > 0);
    q->resumable_id = 0;

    queue_id_ = -1;
    if (q->first_finished_function != -2) {
      RETURN((-q->first_finished_function));
    }
    // timeout
    php_assert(input_ == nullptr);
    RETURN(false);
  }

  int64_t queue_id_;
};

static void process_wait_queue_timeout(kphp_event_timer *timer) {
  int64_t wait_queue_resumable_id = timer->wakeup_extra;
  php_assert(is_started_resumable_id(wait_queue_resumable_id));

  resumable_run_ready(wait_queue_resumable_id);
}

Optional<int64_t> f$wait_queue_next(int64_t queue_id, double timeout) {
  resumable_finished = true;

  tvkprintf(resumable, 1, "Waiting for queue %" PRIi64 "\n", queue_id);
  if (!is_wait_queue_id(queue_id)) {
    if (queue_id != -1) {
      php_warning("Wrong queue_id %" PRIi64 " in function wait_queue_next", queue_id);
    }

    return false;
  }

  wait_queue *q = get_wait_queue(queue_id);
  wait_queue_skip_gotten(q);
  int64_t finished_slot_id = q->first_finished_function;
  if (finished_slot_id != -2) {
    php_assert(finished_slot_id != -1);
    return -finished_slot_id;
  }
  if (q->left_functions == 0) {
    return false;
  }

  if (timeout == 0.0) {
    wait_net(0);

    return q->first_finished_function == -2
           ? Optional<int64_t>{false}
           : Optional<int64_t>{-q->first_finished_function};
  }

  bool has_timeout = true;
  if (timeout < 0 || timeout > MAX_TIMEOUT) {
    has_timeout = false;
    timeout = MAX_TIMEOUT;
  } else {
    update_precise_now();
  }
  timeout += get_precise_now();

  if (in_main_thread()) {
    wait_queue_next(queue_id, timeout);

    q = get_wait_queue(queue_id);//can change in scheduler
    return q->first_finished_function == -2
           ? Optional<int64_t>{false}
           : Optional<int64_t>{-q->first_finished_function};
  }

  wait_queue_resumable *res = new wait_queue_resumable(queue_id);
  q->resumable_id = register_started_resumable(res);
  if (has_timeout) {
    res->set_timer(timeout, q->resumable_id);
  }

  resumable_finished = false;
  return false;
}

Optional<int64_t> f$wait_queue_next_synchronously(int64_t queue_id) {
  if (!is_wait_queue_id(queue_id)) {
    if (queue_id != -1) {
      php_warning("Wrong queue_id %" PRIi64 " in function wait_queue_next_synchronously", queue_id);
    }

    return false;
  }

  return wait_queue_next_synchronously(queue_id);
}

static int32_t yield_wakeup_id;

void f$sched_yield_sleep(double timeout) {
  if (in_main_thread()) {
    resumable_finished = true;
    return;
  }

  resumable_finished = false;
  int64_t id = register_started_resumable(nullptr);
  php_assert(id == static_cast<int32_t>(id));
  allocate_event_timer(timeout + get_precise_now(), yield_wakeup_id, static_cast<int32_t>(id));
}

void yielded_resumable_timeout(kphp_event_timer *timer) {
  int64_t resumable_id = timer->wakeup_extra;
  php_assert(is_started_resumable_id(resumable_id));

  get_started_storage(resumable_id)->save_void();

  yielded_resumables_push(resumable_id);
  remove_event_timer(timer);
}

void f$sched_yield() {
  if (in_main_thread()) {
    resumable_finished = true;
    return;
  }

  resumable_finished = false;
  int64_t id = register_started_resumable(nullptr);
  get_started_storage(id)->save_void();
  yielded_resumables_push(id);
}

void global_init_resumable_lib() {
  php_assert(wait_timeout_wakeup_id == -1);
  php_assert(wait_queue_timeout_wakeup_id == -1);

  wait_timeout_wakeup_id = register_wakeup_callback(&process_wait_timeout);
  wait_queue_timeout_wakeup_id = register_wakeup_callback(&process_wait_queue_timeout);
  yield_wakeup_id = register_wakeup_callback(&yielded_resumable_timeout);
}

void init_resumable_lib() {
  php_assert(wait_timeout_wakeup_id != -1);
  php_assert(wait_queue_timeout_wakeup_id != -1);

  resumable_finished = true;

  runned_resumable_id = 0;
  Resumable::update_output();

  first_forked_resumable_id = current_forked_resumable_id;
  if (first_forked_resumable_id >= 1500000000) {
    first_forked_resumable_id = 1111111111;
  }
  first_array_forked_resumable_id = current_forked_resumable_id = first_forked_resumable_id;

  first_started_resumable_id = current_started_resumable_id;
  if (first_started_resumable_id >= 500000000) {
    first_started_resumable_id = 111111111;
  }
  current_started_resumable_id = first_started_resumable_id;
  first_free_started_resumable_id = 0;

  forked_resumables_size = 170;
  forked_resumables = static_cast<forked_resumable_info *>(dl::allocate(sizeof(forked_resumable_info) * forked_resumables_size));

  started_resumables_size = 170;
  started_resumables = static_cast<started_resumable_info *>(dl::allocate(sizeof(started_resumable_info) * started_resumables_size));

  finished_resumables_size = 170;
  finished_resumables_count = 0;
  finished_resumables = static_cast<int64_t *>(dl::allocate(sizeof(int64_t) * finished_resumables_size));

  yielded_resumables_size = 170;
  yielded_resumables_l = yielded_resumables_r = 0;
  yielded_resumables = static_cast<int64_t *>(dl::allocate(sizeof(int64_t) * yielded_resumables_size));

  wait_queues_size = 101;
  wait_queues = static_cast<wait_queue *>(dl::allocate(sizeof(wait_queue) * wait_queues_size));
  wait_next_queue_id = 0;
  first_free_queue_id = 0;
  new(&gotten_forked_resumable_info.output) Storage;
  gotten_forked_resumable_info.queue_id = -1;
  gotten_forked_resumable_info.continuation = nullptr;
}

int32_t get_resumable_stack(void **buffer, int32_t limit) {
  if (!scheduled_resumable_id) {
    return 0;
  }
  int64_t resumable_id = scheduled_resumable_id;
  if (is_forked_resumable_id(resumable_id)) {
    return 0;
  }
  for (int32_t it = 0; it < limit; it++) {
    started_resumable_info *my = get_started_resumable_info(resumable_id);
    int64_t parent_id = my->parent_id;
    if (is_forked_resumable_id(parent_id)) {
      buffer[it] = get_forked_resumable(parent_id)->get_stack_ptr();
      return it + 1;
    } else if (is_started_resumable_id(parent_id)){
      buffer[it] = get_started_resumable(parent_id)->get_stack_ptr();
      resumable_id = parent_id;
    } else {
      return it;
    }
  }
  return limit;
}

void forcibly_stop_all_running_resumables() {
  resumable_finished = true;
  runned_resumable_id = 0;
  first_free_started_resumable_id = 0;
  Resumable::update_output();
  // Forcibly bind all suspension points to main thread.
  // It will prevent all previously started forks from continuing
  for (int64_t i = first_started_resumable_id; i < current_started_resumable_id; ++i) {
    started_resumable_info *info = get_started_resumable_info(i);
    info->parent_id = 0;
  }
  // Forcibly unbind all forks from their waits.
  // Make it as if nobody waits them anymore
  for (int64_t i = first_forked_resumable_id; i < current_forked_resumable_id; ++i) {
    forked_resumable_info *info = get_forked_resumable_info(i);
    info->queue_id = 0;
  }
}
