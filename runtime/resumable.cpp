#include "runtime/resumable.h"

#include <typeinfo>

#include "common/kprintf.h"

#include "runtime/net_events.h"

bool resumable_finished;

const char *last_wait_error;

static int64_t runned_resumable_id;

void debug_print_resumables();

Storage *Resumable::input_;
Storage *Resumable::output_;

Resumable::Resumable() :
  pos__(nullptr) {
}

Resumable::~Resumable() {
}

Storage *get_storage(int64_t resumable_id) {
  if (resumable_id > 1000000000) {
    return get_forked_storage(resumable_id);
  } else {
    return get_started_storage(resumable_id);
  }
}

bool check_started_storage(Storage *s);
bool check_forked_storage(Storage *s);

static inline void update_current_resumable_id(int64_t new_id);

bool Resumable::resume(int64_t resumable_id, Storage *input) {
  if (input) {
    php_assert(check_started_storage(input) || check_forked_storage(input));
  }
  int64_t parent_id = runned_resumable_id;

  input_ = input;
  update_current_resumable_id(resumable_id);

  bool res = run();

  input_ = nullptr;//must not be used
  update_current_resumable_id(parent_id);

  return res;
}

void Resumable::update_output() {
  if (runned_resumable_id) {
    output_ = get_storage(runned_resumable_id);
  } else {
    output_ = nullptr;
  }
}

struct forked_resumable_info {
  Storage output;
  Resumable *continuation;
  int64_t queue_id;// == 0 - default, 2 * 10^9 > x > 10^8 - waited by x, 10^8 > x > 0 - in queue x, -1 if answer received and not in queue, x < 0 - (-id) of next finished function in the same queue or -2 if none
  int64_t son;
  const char *name;
  double running_time;
};

struct started_resumable_info {
  Storage output;
  Resumable *continuation;
  int64_t parent_id;// x > 0 - has parent x, == 0 - in main thread, == -1 - finished
  int64_t fork_id;
  int64_t son;
  const char *name;
};

static int64_t first_forked_resumable_id;
static int64_t first_array_forked_resumable_id;
static int64_t current_forked_resumable_id = 1123456789;
static forked_resumable_info *forked_resumables;
static forked_resumable_info gotten_forked_resumable_info;
static uint32_t forked_resumables_size;

static int64_t first_started_resumable_id;
static int64_t current_started_resumable_id = 123456789;
static started_resumable_info *started_resumables;
static uint32_t started_resumables_size;
static int64_t first_free_started_resumable_id;


struct wait_queue {
  int64_t first_finished_function;
  int32_t left_functions;
  int64_t resumable_id;//0 - default, x > 0 - id of wait_queue_next resumable, x < 0 - next free queue_id
};

static wait_queue *wait_queues;
static uint32_t wait_queues_size;
static int64_t wait_next_queue_id;


static int64_t *finished_resumables;
static uint32_t finished_resumables_size;
static uint32_t finished_resumables_count;

static int64_t *yielded_resumables;
static uint32_t yielded_resumables_l;
static uint32_t yielded_resumables_r;
static uint32_t yielded_resumables_size;

bool in_main_thread() {
  return runned_resumable_id == 0;
}

static inline bool is_forked_resumable_id(int64_t resumable_id) {
  return first_forked_resumable_id <= resumable_id && resumable_id < current_forked_resumable_id;
}

static inline forked_resumable_info *get_forked_resumable_info(int64_t resumable_id) {
  php_assert (is_forked_resumable_id(resumable_id));
  if (resumable_id < first_array_forked_resumable_id) {
    return &gotten_forked_resumable_info;
  }
  return &forked_resumables[resumable_id - first_array_forked_resumable_id];
}

static inline bool is_started_resumable_id(int64_t resumable_id) {
  return first_started_resumable_id <= resumable_id && resumable_id < current_started_resumable_id;
}

static inline started_resumable_info *get_started_resumable_info(int64_t resumable_id) {
  php_assert (is_started_resumable_id(resumable_id));
  return &started_resumables[resumable_id - first_started_resumable_id];
}

static inline bool is_wait_queue_id(int64_t queue_id) {
  return 0 < queue_id && queue_id <= wait_next_queue_id && wait_queues[queue_id - 1].resumable_id >= 0;
}

static inline wait_queue *get_wait_queue(int64_t queue_id) {
  php_assert (is_wait_queue_id(queue_id));
  return &wait_queues[queue_id - 1];
}

Storage *get_started_storage(int64_t resumable_id) {
  return &get_started_resumable_info(resumable_id)->output;
}

Storage *get_forked_storage(int64_t resumable_id) {
  return &get_forked_resumable_info(resumable_id)->output;
}

bool check_started_storage(Storage *storage) {
  return ((void *)started_resumables <= (void *)storage && (void *)storage < (void *)(started_resumables + started_resumables_size));
}

bool check_forked_storage(Storage *storage) {
  return ((void *)forked_resumables <= (void *)storage && (void *)storage < (void *)(forked_resumables + forked_resumables_size));
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
      forked_resumables = static_cast <forked_resumable_info *> (dl::reallocate(forked_resumables, sizeof(forked_resumable_info) * 2 * forked_resumables_size, sizeof(forked_resumable_info) * forked_resumables_size));
      forked_resumables_size *= 2;
    }
    Resumable::update_output();
  }
  if (current_forked_resumable_id >= 2000000000) {
    php_critical_error ("too many forked resumables");
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

static inline void update_current_resumable_id(int64_t new_id) {
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
  }
  Resumable::update_output();
}

int64_t f$get_running_fork_id() {
  if (runned_resumable_id == 0) {
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
  auto info = get_forked_resumable_info(fork_id);
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

int64_t register_started_resumable(Resumable *resumable) {
  int64_t res_id;
  bool is_new = false;
  if (first_free_started_resumable_id) {
    res_id = first_free_started_resumable_id;
    first_free_started_resumable_id = get_started_resumable_info(first_free_started_resumable_id)->parent_id;
  } else {
    if (current_started_resumable_id == first_started_resumable_id + started_resumables_size) {
      started_resumables = static_cast <started_resumable_info *> (dl::reallocate(started_resumables, sizeof(started_resumable_info) * 2 * started_resumables_size, sizeof(started_resumable_info) * started_resumables_size));
      started_resumables_size *= 2;
    }

    res_id = current_started_resumable_id++;
    is_new = true;

    Resumable::update_output();
  }
  if (res_id >= 1000000000) {
    php_critical_error ("too many started resumables");
  }

  started_resumable_info *res = get_started_resumable_info(res_id);

  php_assert (is_new || res->output.tag == 0);

  new(&res->output) Storage();
  res->continuation = resumable;
  res->parent_id = runned_resumable_id;
  res->fork_id = f$get_running_fork_id();
  res->son = 0;
  res->name = resumable ? typeid(*resumable).name() : "(null)";

  if (runned_resumable_id) {
    if (is_started_resumable_id(runned_resumable_id)) {
      started_resumable_info *parent = get_started_resumable_info(runned_resumable_id);
      if (parent->son != 0) {
        kprintf("Tring to change %ld->son from %ld to %ld\n", runned_resumable_id, parent->son, res_id);
        debug_print_resumables();
        php_assert(parent->son == 0);
      }
      parent->son = res_id;
    } else {
      forked_resumable_info *parent = get_forked_resumable_info(runned_resumable_id);
      if (parent->son != 0) {
        kprintf("Tring to change %ld->son from %ld to %ld\n", runned_resumable_id, parent->son, res_id);
        debug_print_resumables();
        php_assert(parent->son == 0);
      }
      parent->son = res_id;
    }
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
//  fprintf (stderr, "Push resumable %d to queue %d(%d,%d,%d) at %.6lf\n", resumable_id, resumable->queue_id, q->resumable_id, q->first_finished_function, q->left_functions, (update_precise_now(), get_precise_now()));
  resumable->queue_id = q->first_finished_function;
  q->first_finished_function = -resumable_id;

  if (q->resumable_id) {
    resumable_run_ready(q->resumable_id);
    q = get_wait_queue(queue_id); // can be reallocated in run_ready
  }

  q->left_functions--;
}

void finish_forked_resumable(int64_t resumable_id) {
  forked_resumable_info *res = get_forked_resumable_info(resumable_id);
  php_assert (res->continuation != nullptr);

  delete res->continuation;
  res->continuation = nullptr;

  if (res->queue_id > 100000000) {
    php_assert (is_started_resumable_id(res->queue_id));
    int64_t wait_resumable_id = res->queue_id;
    res->queue_id = -1;
    resumable_run_ready(wait_resumable_id);
  } else if (res->queue_id > 0) {
    add_resumable_to_queue(resumable_id, res);
  } else {
    res->queue_id = -1;
  }

  php_assert (get_forked_resumable_info(resumable_id)->queue_id < 0);
}

void finish_started_resumable(int64_t resumable_id) {
  started_resumable_info *res = get_started_resumable_info(resumable_id);
  php_assert (res->continuation != nullptr);

  delete res->continuation;
  res->continuation = nullptr;
}

void unregister_started_resumable_debug_hack(int64_t resumable_id) {
  started_resumable_info *res = get_started_resumable_info(resumable_id);
  php_assert (res->continuation == nullptr);

  res->parent_id = first_free_started_resumable_id;
  first_free_started_resumable_id = resumable_id;
}

void unregister_started_resumable(int64_t resumable_id) {
  started_resumable_info *res = get_started_resumable_info(resumable_id);
  if (res->parent_id > 0) {
    if (is_started_resumable_id(res->parent_id)) {
      started_resumable_info *parent = get_started_resumable_info(res->parent_id);
      if (parent->son != resumable_id && parent->son != 0) {
        kprintf("Tring to change %ld->son from %ld to %d\n", res->parent_id, resumable_id, 0);
        debug_print_resumables();
        php_assert(parent->son == resumable_id || parent->son == 0);
      }
      parent->son = 0;
    } else if (is_forked_resumable_id(res->fork_id)) {
      forked_resumable_info *parent = get_forked_resumable_info(res->parent_id);
      if (parent->son != resumable_id && parent->son != 0) {
        kprintf("Tring to change %ld->son from %ld to %d\n", res->parent_id, resumable_id, 0);
        debug_print_resumables();
        php_assert(parent->son == resumable_id || parent->son == 0);
      }
      parent->son = 0;
    } else {
      php_assert(0);
    }
  }
  php_assert (res->continuation == nullptr);

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
  php_assert (yielded_resumables_l != yielded_resumables_r);
  int64_t result = yielded_resumables[yielded_resumables_l];
  yielded_resumables_l++;
  if (yielded_resumables_l == yielded_resumables_size) {
    yielded_resumables_l = 0;
  }
  return result;
}


static void resumable_add_finished(int64_t resumable_id) {
  php_assert (is_started_resumable_id(resumable_id));
  if (finished_resumables_count >= finished_resumables_size) {
    php_assert (finished_resumables_count == finished_resumables_size);
    finished_resumables = static_cast<int64_t *>(
      dl::reallocate(finished_resumables,
                     sizeof(int64_t) * 2 * finished_resumables_size,
                     sizeof(int64_t) * finished_resumables_size));
    finished_resumables_size *= 2;
  }

//    fprintf (stderr, "!!! process %d(%d) with parent %d in sheduller\n", resumable_id, is_yielded, res->parent_id);
//  fprintf(stderr, "Resumbale %d put to position %d of finised list\n", resumable_id, finished_resumables_count);
  finished_resumables[finished_resumables_count++] = resumable_id;
}

static void resumable_get_finished(int64_t *resumable_id, bool *is_yielded) {
  php_assert (resumable_has_finished());
  if (finished_resumables_count) {
    *resumable_id = finished_resumables[--finished_resumables_count];
    *is_yielded = false;
  } else {
    *resumable_id = yielded_resumables_pop();
    *is_yielded = true;
  }
}

void debug_print_resumables() {
  fprintf(stderr, "first free resumable id: %ld\n", first_free_started_resumable_id);
  fprintf(stderr, "finished:");
  for (uint32_t i = 0; i < finished_resumables_count; i++) {
    fprintf(stderr, " %ld", finished_resumables[i]);
  }
  fprintf(stderr, "\n");
  for (int64_t i = first_started_resumable_id; i < current_started_resumable_id; i++) {
    started_resumable_info *info = get_started_resumable_info(i);
    fprintf(stderr, "started id = %ld, parent = %ld, fork = %ld, son = %ld, continuation = %s, name = %s\n", i, info->parent_id, info->fork_id, info->son,
            info->continuation ? typeid(*info->continuation).name() : "(none)", info->name ? info->name : "()");
  }
  for (int64_t i = first_forked_resumable_id; i < current_forked_resumable_id; i++) {
    forked_resumable_info *info = get_forked_resumable_info(i);
    fprintf(stderr, "forked id = %ld, queue_id = %ld, son = %ld, continuation = %s, name = %s\n", i, info->queue_id, info->son,
            info->continuation ? typeid(*info->continuation).name() : "(none)", info->name ? info->name : "()");
  }
}


void resumable_run_ready(int64_t resumable_id) {
//  fprintf (stderr, "run ready %d\n", resumable_id);
  if (resumable_id > 1000000000) {
    forked_resumable_info *res = get_forked_resumable_info(resumable_id);
    php_assert (res->queue_id >= 0);
    if (res->son) {
      debug_print_resumables();
      php_assert (res->son == 0);
    }
    php_assert (res->continuation->resume(resumable_id, nullptr));
    finish_forked_resumable(resumable_id);
  } else {
    started_resumable_info *res = get_started_resumable_info(resumable_id);
    if (res->son) {
      debug_print_resumables();
      php_assert (res->son == 0);
    }
    php_assert (res->continuation->resume(resumable_id, nullptr));
    finish_started_resumable(resumable_id);
    resumable_add_finished(resumable_id);
  }
}

void run_scheduler(double timeout) __attribute__((section("run_scheduler_section")));

static int64_t scheduled_resumable_id = 0;

void run_scheduler(double timeout) {
//  fprintf (stderr, "!!! run scheduler %d\n", finished_resumables_count);
  int32_t left_resumables = 1000;
  bool force_run_next = false;
  while (resumable_has_finished() && --left_resumables >= 0) {
    if (force_run_next) {
      force_run_next = false;
    } else if (get_precise_now() > timeout) {
      return;
    }

    int64_t resumable_id;
    bool is_yielded;
    resumable_get_finished(&resumable_id, &is_yielded);
    if (is_yielded) {
      left_resumables = 0;
    }

    started_resumable_info *res = get_started_resumable_info(resumable_id);
    php_assert (res->continuation == nullptr);

//    fprintf (stderr, "!!! process %d(%d) with parent %d in scheduler\n", resumable_id, is_long, res->parent_id);
    int64_t parent_id = res->parent_id;
    if (parent_id == 0) {
      res->parent_id = -1;
      return;
    }

    php_assert (parent_id > 0);
    scheduled_resumable_id = parent_id;
    if (parent_id < 1000000000) {
      started_resumable_info *parent = get_started_resumable_info(parent_id);
      if (parent->continuation == nullptr || parent->son != resumable_id) {
        kprintf("Will fail assert with resumbale_id = %ld\n", resumable_id);
        debug_print_resumables();
      }
      php_assert (parent->son == resumable_id);
      php_assert (parent->continuation != nullptr);
      php_assert (parent->parent_id != -2);
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
        kprintf("Will fail assert with resumbale_id = %ld\n", resumable_id);
        debug_print_resumables();
      }
      php_assert (parent->son == resumable_id);
      php_assert (parent->continuation != nullptr);
      php_assert (parent->queue_id >= 0);
      parent->son = 0;
      if (parent->continuation->resume(parent_id, &res->output)) {
        finish_forked_resumable(parent_id);
      }
    }
    scheduled_resumable_id = 0;

    unregister_started_resumable_debug_hack(resumable_id);
  }
}


void wait_synchronously(int64_t resumable_id) {
  forked_resumable_info *resumable = get_forked_resumable_info(resumable_id);

  if (resumable->queue_id < 0) {
    return;
  }

  update_precise_now();
  while (wait_net(MAX_TIMEOUT * 1000) && resumable->queue_id >= 0) {
    update_precise_now();
  }
}

bool f$wait_synchronously(int64_t resumable_id) {
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

  wait_synchronously(resumable_id);
  return true;
}


static bool wait_forked_resumable(int64_t resumable_id, double timeout) {
  php_assert (timeout > get_precise_now());//TODO remove asserts
  php_assert (in_main_thread());//TODO remove asserts
  php_assert (is_forked_resumable_id(resumable_id));

  forked_resumable_info *resumable = get_forked_resumable_info(resumable_id);

  do {
    if (resumable->queue_id < 0) {
      return true;
    }

    run_scheduler(timeout);

    resumable = get_forked_resumable_info(resumable_id);//can change in scheduler
    if (resumable->queue_id < 0) {
      return true;
    }

    if (get_precise_now() > timeout) {
      return false;
    }
    update_precise_now();

    if (resumable_has_finished()) {
      wait_net(0);
      continue;
    }
  } while (resumable_has_finished() || wait_net(timeout_convert_to_ms(timeout - get_precise_now())));

  return false;
}

bool wait_started_resumable(int64_t resumable_id) {
  php_assert (in_main_thread());//TODO remove asserts
  php_assert (is_started_resumable_id(resumable_id));

  started_resumable_info *resumable = get_started_resumable_info(resumable_id);

  do {
    php_assert (resumable->parent_id == 0);

    run_scheduler(get_precise_now() + MAX_TIMEOUT);

    resumable = get_started_resumable_info(resumable_id);//can change in scheduler
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


static int32_t wait_timeout_wakeup_id = -1;

class wait_resumable : public Resumable {
  int64_t child_id;
  event_timer *timer;
protected:
  bool run() {
    if (timer != nullptr) {
      remove_event_timer(timer);
      timer = nullptr;
    }

    forked_resumable_info *info = get_forked_resumable_info(child_id);

    if (info->queue_id < 0) {
      output_->save<bool>(true);
    } else {
      php_assert (input_ == nullptr);
      info->queue_id = 0;
      output_->save<bool>(false);
    }

    child_id = -1;
    return true;
  }

public:
  wait_resumable(int64_t child_id) :
    child_id(child_id),
    timer(nullptr) {
  }

  void set_timer(double timeout, int64_t wakeup_extra) {
    php_assert(static_cast<int32_t>(wakeup_extra) == wakeup_extra);
    timer = allocate_event_timer(timeout, wait_timeout_wakeup_id, static_cast<int32_t>(wakeup_extra));
  }
};

class wait_multiple_resumable : public Resumable {
  int64_t child_id;
  forked_resumable_info *info;
protected:
  bool run() {
    RESUMABLE_BEGIN
      while (true) {
        info = get_forked_resumable_info(child_id);

        if (info->queue_id < 0) {
          break;
        } else {
          if (!resumable_has_finished()) {
            wait_net(MAX_TIMEOUT);
          }
          f$sched_yield();
          TRY_WAIT_DROP_RESULT (wait_many_resumable_label1, void);
        }
      }

      output_->save<bool>(true);
      child_id = -1;
      return true;
    RESUMABLE_END
  }

public:
  explicit wait_multiple_resumable(int64_t child_id) :
    child_id(child_id),
    info(nullptr) {}
};


void process_wait_timeout(event_timer *timer) {
  int64_t wait_resumable_id = timer->wakeup_extra;
  php_assert (is_started_resumable_id(wait_resumable_id));

  resumable_run_ready(wait_resumable_id);
}

bool f$wait(int64_t resumable_id, double timeout) {
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
    php_warning("Resumable is already waited by other thread");
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

bool f$wait_multiple(int64_t resumable_id) {
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
    return start_resumable<bool>(new wait_multiple_resumable(resumable_id));
  }

  return f$wait(resumable_id);
}

int64_t wait_queue_push(int64_t queue_id, int64_t resumable_id) {
  if (!is_forked_resumable_id(resumable_id)) {
    php_warning("Wrong resumable_id %ld in function wait_queue_push", resumable_id);

    return queue_id;
  }
  forked_resumable_info *resumable = get_forked_resumable_info(resumable_id);
  if (resumable->queue_id != 0 && resumable->queue_id != -1) {
    php_warning("Someone already waits resumable %ld", resumable_id);

    return queue_id;
  }
  if (resumable->queue_id < 0 && resumable->output.tag == 0) {
    return queue_id;
  }

  if (queue_id == -1) {
    queue_id = f$wait_queue_create();
  } else {
    if (!is_wait_queue_id(queue_id)) {
      php_warning("Wrong queue_id %ld", queue_id);

      return queue_id;
    }
  }
  wait_queue *q = get_wait_queue(queue_id);

  if (resumable->queue_id == 0) {
    resumable->queue_id = queue_id;

//    fprintf (stderr, "Link resumable %d with queue %d at %.6lf\n", resumable_id, queue_id, (update_precise_now(), get_precise_now()));
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

int64_t first_free_queue_id;

void unregister_wait_queue(int64_t queue_id) {
  if (queue_id == -1) {
    return;
  }
  get_wait_queue(queue_id)->resumable_id = -first_free_queue_id - 1;
  first_free_queue_id = queue_id;
}

int64_t f$wait_queue_create() {
  int64_t res_id;
  if (first_free_queue_id != 0) {
    res_id = first_free_queue_id;
    first_free_queue_id = -wait_queues[res_id - 1].resumable_id - 1;
    php_assert (0 <= first_free_queue_id && first_free_queue_id <= wait_next_queue_id);
  } else {
    if (wait_next_queue_id >= wait_queues_size) {
      php_assert (wait_next_queue_id == wait_queues_size);
      wait_queues = static_cast <wait_queue *> (dl::reallocate(wait_queues, sizeof(wait_queue) * 2 * wait_queues_size, sizeof(wait_queue) * wait_queues_size));
      wait_queues_size *= 2;
    }
    res_id = ++wait_next_queue_id;
  }
  if (wait_next_queue_id >= 99999999) {
    php_critical_error ("too many wait queues");
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
    for (array<mixed>::const_iterator p = resumable_ids.begin(), p_end = resumable_ids.end(); p != p_end; ++p) {
      wait_queue_push(queue_id, f$intval(p.get_value()));
    }
    return queue_id;
  } else {
    return wait_queue_push(queue_id, f$intval(resumable_ids));
  }
}

int64_t wait_queue_create(const array<int64_t> &resumable_ids) {
  if (resumable_ids.count() == 0) {
    return -1;
  }

  const int64_t queue_id = f$wait_queue_create();

  const auto p_end = resumable_ids.end();
  for (auto p = resumable_ids.begin(); p != p_end; ++p) {
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
    php_assert (q->first_finished_function != -1);
  }
}

bool f$wait_queue_empty(int64_t queue_id) {
  if (!is_wait_queue_id(queue_id)) {
    if (queue_id != -1) {
      php_warning("Wrong queue_id %ld in function wait_queue_empty", queue_id);
    }
    return true;
  }

  wait_queue *q = get_wait_queue(queue_id);
  wait_queue_skip_gotten(q);
  return q->left_functions == 0 && q->first_finished_function == -2;
}

static void wait_queue_next(int64_t queue_id, double timeout) {
  php_assert (timeout > get_precise_now());//TODO remove asserts
  php_assert (in_main_thread());//TODO remove asserts
  php_assert (is_wait_queue_id(queue_id));

  wait_queue *q = get_wait_queue(queue_id);

  do {
    if (q->first_finished_function != -2) {
      return;
    }

    run_scheduler(timeout);

    q = get_wait_queue(queue_id);//can change in scheduler
    wait_queue_skip_gotten(q);
    if (q->first_finished_function != -2 || q->left_functions == 0) {
      return;
    }

    if (get_precise_now() > timeout) {
      return;
    }
    update_precise_now();

    if (resumable_has_finished()) {
      wait_net(0);
      continue;
    }
  } while (resumable_has_finished() || wait_net(timeout_convert_to_ms(timeout - get_precise_now())));

  return;
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

class wait_queue_resumable : public Resumable {
  using ReturnT = Optional<int64_t>;
  int64_t queue_id;
  event_timer *timer;
protected:
  bool run() {
    if (timer != nullptr) {
      remove_event_timer(timer);
      timer = nullptr;
    }

    wait_queue *q = get_wait_queue(queue_id);

    php_assert (q->resumable_id > 0);
    q->resumable_id = 0;

    queue_id = -1;
    if (q->first_finished_function != -2) {
      RETURN((-q->first_finished_function));
    } else {
      php_assert (input_ == nullptr);//timeout
      RETURN(false);
    }
  }

public:
  explicit wait_queue_resumable(int64_t queue_id) :
    queue_id(queue_id),
    timer(nullptr) {
  }

  void set_timer(double timeout, int64_t wakeup_extra) {
    php_assert(static_cast<int32_t>(wakeup_extra) == wakeup_extra);
    timer = allocate_event_timer(timeout, wait_queue_timeout_wakeup_id, static_cast<int32_t>(wakeup_extra));
  }
};

void process_wait_queue_timeout(event_timer *timer) {
  int64_t wait_queue_resumable_id = timer->wakeup_extra;
  php_assert (is_started_resumable_id(wait_queue_resumable_id));

  resumable_run_ready(wait_queue_resumable_id);
}

Optional<int64_t> f$wait_queue_next(int64_t queue_id, double timeout) {
  resumable_finished = true;

//  fprintf (stderr, "Waiting for queue %d\n", queue_id);
  if (!is_wait_queue_id(queue_id)) {
    if (queue_id != -1) {
      php_warning("Wrong queue_id %ld in function wait_queue_next", queue_id);
    }

    return false;
  }

  wait_queue *q = get_wait_queue(queue_id);
  wait_queue_skip_gotten(q);
  int64_t finished_slot_id = q->first_finished_function;
  if (finished_slot_id != -2) {
    php_assert (finished_slot_id != -1);
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
      php_warning("Wrong queue_id %ld in function wait_queue_next_synchronously", queue_id);
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

void yielded_resumable_timeout(event_timer *timer) {
  int64_t resumable_id = timer->wakeup_extra;
  php_assert (is_started_resumable_id(resumable_id));

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
  php_assert (wait_timeout_wakeup_id == -1);
  php_assert (wait_queue_timeout_wakeup_id == -1);

  wait_timeout_wakeup_id = register_wakeup_callback(&process_wait_timeout);
  wait_queue_timeout_wakeup_id = register_wakeup_callback(&process_wait_queue_timeout);
  yield_wakeup_id = register_wakeup_callback(&yielded_resumable_timeout);
}

void init_resumable_lib() {
  php_assert (wait_timeout_wakeup_id != -1);
  php_assert (wait_queue_timeout_wakeup_id != -1);

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
