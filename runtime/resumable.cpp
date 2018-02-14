#include "runtime/resumable.h"

#include <typeinfo>

#include "common/kprintf.h"

#include "runtime/net_events.h"

bool resumable_finished;

const char *last_wait_error;

static int runned_resumable_id;

void debug_print_resumables();

Storage::Storage():
  getter_ (NULL) {
  memset (storage_, 0, sizeof (var));
}

var Storage::load_void (char *storage __attribute__((unused))) {
  // result of void function should not be loaded
  php_assert (0);
  return var();
}

var Storage::load_exception (char *storage) {
  php_assert (CurException == NULL);
  CurException = *reinterpret_cast <Exception **> (storage);
  return var();
}

var Storage::load_as_var() {
  if (getter_ == NULL) {
    last_wait_error = "Result already was gotten";
    return false;
  }

  Getter getter = getter_;
  getter_ = NULL;
  return getter (storage_);
}

void Storage::save_void (void) {
  if (CurException) {
    save_exception();
  } else {
    getter_ = load_void;
  }
}

void Storage::save_exception (void) {
  php_assert (CurException != NULL);
  php_assert (sizeof (Exception *) <= sizeof (var));
  *reinterpret_cast <Exception **> (storage_) = CurException;
  CurException = NULL;
  getter_ = load_exception;
}

Storage *Resumable::input_;
Storage *Resumable::output_;

void *Resumable::operator new (size_t size) {
  return dl::allocate (size);
}

void Resumable::operator delete (void *ptr, size_t size) {
  dl::deallocate (ptr, size);
}

Resumable::Resumable():
  pos__(NULL) {
}

Resumable::~Resumable() {
}

Storage *get_storage (int resumable_id) {
  if (resumable_id > 1000000000) {
    return get_forked_storage (resumable_id);
  } else {
    return get_started_storage (resumable_id);
  }
}

bool check_started_storage(Storage* s);
bool check_forked_storage(Storage* s);

bool Resumable::resume (int resumable_id, Storage *input) {
  if (input) {
    php_assert(check_started_storage(input) || check_forked_storage(input));
  }
  int parent_id = runned_resumable_id;

  input_ = input;
  runned_resumable_id = resumable_id;
  update_output();

  bool res = run();

  input_ = NULL;//must not be used
  runned_resumable_id = parent_id;
  update_output();

  return res;
}

void Resumable::update_output (void) {
  if (runned_resumable_id) {
    output_ = get_storage (runned_resumable_id);
  } else {
    output_ = NULL;
  }
}

struct forked_resumable_info {
  Storage output;
  Resumable *continuation;
  int queue_id;// == 0 - default, 2 * 10^9 > x > 10^8 - waited by x, 10^8 > x > 0 - in queue x, -1 if answer received and not in queue, x < 0 - (-id) of next finished function in the same queue or -2 if none
  int son;
  const char *name;
};

struct started_resumable_info {
  Storage output;
  Resumable *continuation;
  int parent_id;// x > 0 - has parent x, == 0 - in main thread, == -1 - finished
  int fork_id;
  int son;
  const char *name;
};

static int first_forked_resumable_id;
static int first_array_forked_resumable_id;
static int current_forked_resumable_id = 1123456789;
static forked_resumable_info *forked_resumables;
static forked_resumable_info gotten_forked_resumable_info;
static int forked_resumables_size;

static int first_started_resumable_id;
static int current_started_resumable_id = 123456789;
static started_resumable_info *started_resumables;
static int started_resumables_size;
static int first_free_started_resumable_id;


struct wait_queue {
  int first_finished_function;
  int left_functions;
  int resumable_id;//0 - default, x > 0 - id of wait_queue_next resumable, x < 0 - next free queue_id
};

static wait_queue *wait_queues;
static int wait_queues_size;
static int wait_next_queue_id;


static int *finished_resumables;
static int finished_resumables_size;
static int finished_resumables_count;

static int *yielded_resumables;
static int yielded_resumables_l;
static int yielded_resumables_r;
static int yielded_resumables_size;

bool in_main_thread (void) {
  return runned_resumable_id == 0;
}

static inline bool is_forked_resumable_id(int resumable_id) {
  return first_forked_resumable_id <= resumable_id && resumable_id < current_forked_resumable_id;
}

static inline forked_resumable_info* get_forked_resumable_info (int resumable_id) {
  php_assert (is_forked_resumable_id (resumable_id));
  if (resumable_id < first_array_forked_resumable_id) return &gotten_forked_resumable_info;
  return &forked_resumables[resumable_id - first_array_forked_resumable_id];
}

static inline bool is_started_resumable_id(int resumable_id) {
  return first_started_resumable_id <= resumable_id && resumable_id < current_started_resumable_id;
}

static inline started_resumable_info* get_started_resumable_info (int resumable_id) {
  php_assert (is_started_resumable_id (resumable_id));
  return &started_resumables[resumable_id - first_started_resumable_id];
}

static inline bool is_wait_queue_id (int queue_id) {
  return 0 < queue_id && queue_id <= wait_next_queue_id && wait_queues[queue_id - 1].resumable_id >= 0;
}

static inline wait_queue* get_wait_queue(int queue_id) {
  php_assert (is_wait_queue_id (queue_id));
  return &wait_queues[queue_id - 1];
}

Storage *get_started_storage (int resumable_id) {
  return &get_started_resumable_info(resumable_id)->output;
}

Storage *get_forked_storage (int resumable_id) {
  return &get_forked_resumable_info(resumable_id)->output;
}

bool check_started_storage(Storage* storage) {
  return ((void*)started_resumables <= (void*)storage && (void*)storage < (void*)(started_resumables + started_resumables_size));
}

bool check_forked_storage(Storage* storage) {
  return ((void*)forked_resumables <= (void*)storage && (void*)storage < (void*)(forked_resumables + forked_resumables_size));
}


int register_forked_resumable (Resumable *resumable) {
  if (current_forked_resumable_id == first_array_forked_resumable_id + forked_resumables_size) {
    int first_needed_id = 0;
    while (first_needed_id < forked_resumables_size && forked_resumables[first_needed_id].queue_id == -1 && forked_resumables[first_needed_id].output.getter_ == NULL) {
      first_needed_id++;
    }
    if (first_needed_id > forked_resumables_size / 2) {
      memcpy(forked_resumables, forked_resumables + first_needed_id, sizeof(forked_resumable_info) * (forked_resumables_size - first_needed_id));
      first_array_forked_resumable_id += first_needed_id;
    } else {
      forked_resumables = static_cast <forked_resumable_info *> (dl::reallocate (forked_resumables, sizeof (forked_resumable_info) * 2 * forked_resumables_size, sizeof (forked_resumable_info) * forked_resumables_size));
      forked_resumables_size *= 2;
    }
    Resumable::update_output();
  }
  if (current_forked_resumable_id >= 2000000000) {
    php_critical_error ("too many forked resumables");
  }

  int res_id = current_forked_resumable_id++;
  forked_resumable_info *res = get_forked_resumable_info (res_id);

  new (&res->output) Storage();
  res->continuation = resumable;
  res->queue_id = 0;
  res->son = 0;
  res->name = resumable ? typeid(*resumable).name() : "(null)";

  return res_id;
}

int f$get_running_fork_id() {
  if (runned_resumable_id == 0) {
    return 0;
  }
  if (is_forked_resumable_id(runned_resumable_id)) {
    return runned_resumable_id;
  }
  if (is_started_resumable_id(runned_resumable_id)) {
    return get_started_resumable_info (runned_resumable_id)->fork_id;
  }
  php_assert (false);
  return 0;
}

int register_started_resumable (Resumable *resumable) {
  int res_id;
  bool is_new = false;
  if (first_free_started_resumable_id) {
    res_id = first_free_started_resumable_id;
    first_free_started_resumable_id = get_started_resumable_info (first_free_started_resumable_id)->parent_id;
  } else {
    if (current_started_resumable_id == first_started_resumable_id + started_resumables_size) {
      started_resumables = static_cast <started_resumable_info *> (dl::reallocate (started_resumables, sizeof (started_resumable_info) * 2 * started_resumables_size, sizeof (started_resumable_info) * started_resumables_size));
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

  php_assert (is_new || res->output.getter_ == NULL);

  new (&res->output) Storage();
  res->continuation = resumable;
  res->parent_id = runned_resumable_id;
  res->fork_id = f$get_running_fork_id();
  res->son = 0;
  res->name = resumable ? typeid(*resumable).name() : "(null)";

  if (runned_resumable_id) {
    if (is_started_resumable_id(runned_resumable_id)) {
      started_resumable_info *parent = get_started_resumable_info(runned_resumable_id);
      if (parent->son != 0) {
        kprintf("Tring to change %d->son from %d to %d\n", runned_resumable_id, 0, res_id);
        debug_print_resumables();
        php_assert(parent->son == 0);
      }
      parent->son = res_id;
    } else {
      forked_resumable_info *parent = get_forked_resumable_info(runned_resumable_id);
      if (parent->son != 0) {
        kprintf("Tring to change %d->son from %d to %d\n", runned_resumable_id, 0, res_id);
        debug_print_resumables();
        php_assert(parent->son == 0);
      }
      parent->son = res_id;
    }
  }

  return res_id;
}


Resumable *get_forked_resumable (int resumable_id) {
  return get_forked_resumable_info (resumable_id)->continuation;
}

static void add_resumable_to_queue (int resumable_id, forked_resumable_info *resumable) {
  int queue_id = resumable->queue_id;
  wait_queue *q = get_wait_queue (queue_id);
//  fprintf (stderr, "Push resumable %d to queue %d(%d,%d,%d) at %.6lf\n", resumable_id, resumable->queue_id, q->resumable_id, q->first_finished_function, q->left_functions, (update_precise_now(), get_precise_now()));
  resumable->queue_id = q->first_finished_function;
  q->first_finished_function = -resumable_id;

  if (q->resumable_id) {
    resumable_run_ready (q->resumable_id);
    q = get_wait_queue (queue_id); // can be reallocated in run_ready
  }

  q->left_functions--;
}

void finish_forked_resumable (int resumable_id) {
  forked_resumable_info *res = get_forked_resumable_info (resumable_id);
  php_assert (res->continuation != NULL);

  delete res->continuation;
  res->continuation = NULL;

  if (res->queue_id > 100000000) {
    php_assert (is_started_resumable_id (res->queue_id));
    int wait_resumable_id = res->queue_id;
    res->queue_id = -1;
    resumable_run_ready (wait_resumable_id);
  } else if (res->queue_id > 0) {
    add_resumable_to_queue (resumable_id, res);
  } else {
    res->queue_id = -1;
  }

  php_assert (get_forked_resumable_info (resumable_id)->queue_id < 0);
}

void finish_started_resumable (int resumable_id) {
  started_resumable_info *res = get_started_resumable_info (resumable_id);
  php_assert (res->continuation != NULL);

  delete res->continuation;
  res->continuation = NULL;
}

void unregister_started_resumable_debug_hack (int resumable_id) {
  started_resumable_info *res = get_started_resumable_info (resumable_id);
  php_assert (res->continuation == NULL);

  res->parent_id = first_free_started_resumable_id;
  first_free_started_resumable_id = resumable_id;
}

void unregister_started_resumable (int resumable_id) {
  started_resumable_info *res = get_started_resumable_info (resumable_id);
  if (res->parent_id > 0) {
    if (is_started_resumable_id(res->parent_id)) {
      started_resumable_info *parent = get_started_resumable_info(res->parent_id);
      if (parent->son != resumable_id && parent->son != 0) {
        kprintf("Tring to change %d->son from %d to %d\n", res->parent_id, resumable_id, 0);
        debug_print_resumables();
        php_assert(parent->son == resumable_id || parent->son == 0);
      }
      parent->son = 0;
    } else if (is_forked_resumable_id(res->fork_id)) {
      forked_resumable_info *parent = get_forked_resumable_info(res->parent_id);
      if (parent->son != resumable_id && parent->son != 0) {
        kprintf("Tring to change %d->son from %d to %d\n", res->parent_id, resumable_id, 0);
        debug_print_resumables();
        php_assert(parent->son == resumable_id || parent->son == 0);
      }
      parent->son = 0;
    } else {
      php_assert(0);
    }
  }
  php_assert (res->continuation == NULL);

  res->parent_id = first_free_started_resumable_id;
  first_free_started_resumable_id = resumable_id;
}



static bool resumable_has_finished (void) {
  return finished_resumables_count > 0 || yielded_resumables_l != yielded_resumables_r;
}

static void yielded_resumables_push (int id) {
  yielded_resumables[yielded_resumables_r] = id;
  yielded_resumables_r++;
  if (yielded_resumables_r == yielded_resumables_size) {
    yielded_resumables_r = 0;
  }
  if (yielded_resumables_r == yielded_resumables_l) {
    yielded_resumables = static_cast <int *> (dl::reallocate (yielded_resumables, sizeof (int) * 2 * yielded_resumables_size, sizeof (int) * yielded_resumables_size));
    memcpy(yielded_resumables + yielded_resumables_size, yielded_resumables, sizeof(int) * yielded_resumables_r);
    yielded_resumables_r += yielded_resumables_size;
    yielded_resumables_size *= 2;
  }
}

static int yielded_resumables_pop (void) {
  php_assert (yielded_resumables_l != yielded_resumables_r);
  int result = yielded_resumables[yielded_resumables_l];
  yielded_resumables_l++;
  if (yielded_resumables_l == yielded_resumables_size) {
    yielded_resumables_l = 0;
  }
  return result;
}


static void resumable_add_finished (int resumable_id) {
  php_assert (is_started_resumable_id (resumable_id));
  if (finished_resumables_count >= finished_resumables_size) {
    php_assert (finished_resumables_count == finished_resumables_size);
    finished_resumables = static_cast <int *> (dl::reallocate (finished_resumables, sizeof (int) * 2 * finished_resumables_size, sizeof (int) * finished_resumables_size));
    finished_resumables_size *= 2;
  }

//    fprintf (stderr, "!!! process %d(%d) with parent %d in sheduller\n", resumable_id, is_yielded, res->parent_id);
//  fprintf(stderr, "Resumbale %d put to position %d of finised list\n", resumable_id, finished_resumables_count);
  finished_resumables[finished_resumables_count++] = resumable_id;
}

static void resumable_get_finished (int *resumable_id, bool *is_yielded) {
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
  fprintf(stderr, "first free resumable id: %d\n", first_free_started_resumable_id);
  fprintf(stderr, "finished:");
  for (int i = 0; i < finished_resumables_count; i++) {
    fprintf(stderr, " %d", finished_resumables[i]);
  }
  fprintf(stderr, "\n");
  for (int i = first_started_resumable_id; i < current_started_resumable_id; i++) {
    started_resumable_info* info = get_started_resumable_info(i);
    fprintf(stderr, "started id = %d, parent = %d, fork = %d, son = %d, continuation = %s, name = %s\n", i, info->parent_id, info->fork_id, info->son,
            info->continuation ? typeid(*info->continuation).name() : "(none)", info->name ? info->name : "()");
  }
  for (int i = first_forked_resumable_id; i < current_forked_resumable_id; i++) {
    forked_resumable_info* info = get_forked_resumable_info(i);
    fprintf(stderr, "forked id = %d, queue_id = %d, son = %d, continuation = %s, name = %s\n", i, info->queue_id, info->son,
            info->continuation ? typeid(*info->continuation).name()  : "(none)", info->name ? info->name : "()");
  }
}


void resumable_run_ready (int resumable_id) {
//  fprintf (stderr, "run ready %d\n", resumable_id);
  if (resumable_id > 1000000000) {
    forked_resumable_info *res = get_forked_resumable_info (resumable_id);
    php_assert (res->queue_id >= 0);
    if (res->son) {
      debug_print_resumables();
      php_assert (res->son == 0);
    }
    php_assert (res->continuation->resume (resumable_id, NULL));
    finish_forked_resumable (resumable_id);
  } else {
    started_resumable_info *res = get_started_resumable_info (resumable_id);
    if (res->son) {
      debug_print_resumables();
      php_assert (res->son == 0);
    }
    php_assert (res->continuation->resume (resumable_id, NULL));
    finish_started_resumable (resumable_id);
    resumable_add_finished (resumable_id);
  }
}


void run_scheduller (double timeout) {
//  fprintf (stderr, "!!! run scheduller %d\n", finished_resumables_count);
  int left_resumables = 1000;
  int force_run_next = false;
  while (resumable_has_finished() && --left_resumables >= 0) {
    if (force_run_next) {
      force_run_next = false;
    } else if (get_precise_now() > timeout) {
      return;
    }

    int resumable_id;
    bool is_yielded;
    resumable_get_finished (&resumable_id, &is_yielded);
    if (is_yielded) {
      left_resumables = 0;
    }

    started_resumable_info *res = get_started_resumable_info (resumable_id);
    php_assert (res->continuation == NULL);

//    fprintf (stderr, "!!! process %d(%d) with parent %d in scheduller\n", resumable_id, is_long, res->parent_id);
    int parent_id = res->parent_id;
    if (parent_id == 0) {
      res->parent_id = -1;
      return;
    }

    php_assert (parent_id > 0);
    if (parent_id < 1000000000) {
      started_resumable_info *parent = get_started_resumable_info (parent_id);
      if (parent->continuation == NULL || parent->son != resumable_id) {
        kprintf("Will fail assert with resumbale_id = %d\n", resumable_id);
        debug_print_resumables();
      }
      php_assert (parent->son == resumable_id);
      php_assert (parent->continuation != NULL);
      php_assert (parent->parent_id != -2);
      parent->son = 0;
      if (parent->continuation->resume (parent_id, &res->output)) {
        finish_started_resumable (parent_id);
        resumable_add_finished (parent_id);
        left_resumables++;
        force_run_next = true;
      }
    } else {
      forked_resumable_info *parent = get_forked_resumable_info (parent_id);
      if (parent->continuation == NULL || parent->son != resumable_id) {
        kprintf("Will fail assert with resumbale_id = %d\n", resumable_id);
        debug_print_resumables();
      }
      php_assert (parent->son == resumable_id);
      php_assert (parent->continuation != NULL);
      php_assert (parent->queue_id >= 0);
      parent->son = 0;
      if (parent->continuation->resume (parent_id, &res->output)) {
        finish_forked_resumable (parent_id);
      }
    }

    unregister_started_resumable_debug_hack (resumable_id);
  }
}


void wait_synchronously (int resumable_id) {
  forked_resumable_info *resumable = get_forked_resumable_info (resumable_id);

  if (resumable->queue_id < 0) {
    return;
  }

  update_precise_now();
  while (wait_net (MAX_TIMEOUT * 1000) && resumable->queue_id >= 0) {
    update_precise_now();
  }
}

bool f$wait_synchronously (int resumable_id) {
  last_wait_error = NULL;
  if (!is_forked_resumable_id(resumable_id)) {
    last_wait_error = "Wrong resumable id";
    return false;
  }

  forked_resumable_info *resumable = get_forked_resumable_info (resumable_id);

  if (resumable->queue_id < 0) {
    return true;
  }

  if (resumable->queue_id > 0) {
    last_wait_error = "Someone already waits for this resumable";
    return false;
  }

  wait_synchronously (resumable_id);
  return true;
}


static bool wait_forked_resumable (int resumable_id, double timeout) {
  php_assert (timeout > get_precise_now());//TODO remove asserts
  php_assert (in_main_thread());//TODO remove asserts
  php_assert (is_forked_resumable_id(resumable_id));

  forked_resumable_info *resumable = get_forked_resumable_info (resumable_id);

  do {
    if (resumable->queue_id < 0) {
      return true;
    }

    run_scheduller (timeout);

    resumable = get_forked_resumable_info (resumable_id);//can change in scheduller
    if (resumable->queue_id < 0) {
      return true;
    }

    if (get_precise_now() > timeout) {
      return false;
    }
    update_precise_now();
    
    if (resumable_has_finished()) {
      wait_net (0);
      continue;
    }
  } while (resumable_has_finished() || wait_net (timeout_convert_to_ms (timeout - get_precise_now())));

  return false;
}

bool wait_started_resumable (int resumable_id) {
  php_assert (in_main_thread());//TODO remove asserts
  php_assert (is_started_resumable_id (resumable_id));

  started_resumable_info *resumable = get_started_resumable_info (resumable_id);

  do {
    php_assert (resumable->parent_id == 0);

    run_scheduller (get_precise_now() + MAX_TIMEOUT);

    resumable = get_started_resumable_info (resumable_id);//can change in scheduller
    if (resumable->parent_id == -1) {
      return true;
    }

    update_precise_now();
    
    if (resumable_has_finished()) {
      wait_net (0);
      continue;
    }
  } while (resumable_has_finished() || wait_net (MAX_TIMEOUT_MS));

  return false;
}


static int wait_timeout_wakeup_id = -1;

class wait_resumable: public Resumable {
  int child_id;
  event_timer *timer;
protected:
  bool run (void) {
    if (timer != NULL) {
      remove_event_timer (timer);
      timer = NULL;
    }

    forked_resumable_info *info = get_forked_resumable_info (child_id);

    if (info->queue_id < 0) {
      output_->save <bool> (true);
    } else {
      php_assert (input_ == NULL);
      info->queue_id = 0;
      output_->save <bool> (false);
    }

    child_id = -1;
    return true;
  }

public:
  wait_resumable (int child_id): child_id (child_id), timer (NULL) {
  }

  void set_timer (double timeout, int wakeup_extra) {
    timer = allocate_event_timer (timeout, wait_timeout_wakeup_id, wakeup_extra);
  }
};

class wait_multiple_resumable : public Resumable {
  int child_id;
  forked_resumable_info *info;
protected:
  bool run (void) {
    RESUMABLE_BEGIN
    while (true) {
      info = get_forked_resumable_info (child_id);

      if (info->queue_id < 0) {
        break;
      } else {
        if (!resumable_has_finished ()) {
          wait_net (MAX_TIMEOUT);
        }
        f$sched_yield ();
        TRY_WAIT_DROP_RESULT (wait_many_resumable_label1, void);
      }
    }

    output_->save <bool> (true);
    child_id = -1;
    return true;
    RESUMABLE_END
  }

public:
  wait_multiple_resumable (int child_id): child_id (child_id) {
  }
};


void process_wait_timeout (int wait_resumable_id) {
  php_assert (is_started_resumable_id (wait_resumable_id));

  resumable_run_ready (wait_resumable_id);
}

class wait_result_resumable: public Resumable {
  typedef var ReturnT;
  int resumable_id;
  double timeout;
  bool ready;
protected:
  bool run (void) {
    RESUMABLE_BEGIN
      ready = f$wait (resumable_id, timeout);
      TRY_WAIT(wait_result_resumable_label_1, ready, bool);
      if (!ready) {
        if (last_wait_error == NULL) {
          last_wait_error = "Timeout in wait_result";
        }
        RETURN(false);
      }

      forked_resumable_info *resumable = get_forked_resumable_info (resumable_id);

      if (resumable->output.getter_ == NULL) {
        last_wait_error = "Result already was gotten";
        RETURN(false);
      }

      RETURN(resumable->output.load_as_var());
    RESUMABLE_END
  }

public:
  wait_result_resumable (int resumable_id, double timeout): resumable_id (resumable_id), timeout (timeout) {
  }
};


bool f$wait (int resumable_id, double timeout) {
  resumable_finished = true;

  last_wait_error = NULL;
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
    wait_net (0);
  }

  forked_resumable_info *resumable = get_forked_resumable_info (resumable_id);

  if (resumable->queue_id < 0) {
    return true;
  }

  if (resumable->queue_id > 0) {
    last_wait_error = "Someone already waits for this resumable";
    return false;
  }

  if (timeout == 0.0) {
    last_wait_error = "Zero timeout in wait";
    return false;
  }

  if (in_main_thread()) {
    if (!wait_forked_resumable (resumable_id, timeout)) {
      last_wait_error = "Timeout in wait";
      return false;
    }

    return true;
  }

  wait_resumable *res = new wait_resumable (resumable_id);
  resumable->queue_id = register_started_resumable (res);
  if (has_timeout) {
    res->set_timer (timeout, resumable->queue_id);
  }

  resumable_finished = false;
  return false;
}

bool f$wait_multiple (int resumable_id) {
  resumable_finished = true;

  last_wait_error = NULL;
  if (!is_forked_resumable_id(resumable_id)) {
    last_wait_error = "Wrong resumable id";
    return false;
  }

  wait_net (0);

  forked_resumable_info *resumable = get_forked_resumable_info (resumable_id);

  if (resumable->queue_id < 0) {
    return true;
  }

  if (resumable->queue_id > 0) {
    return start_resumable<bool> (new wait_multiple_resumable(resumable_id));
  }

  return f$wait(resumable_id);
}

var f$wait_result (int resumable_id, double timeout) {
  return start_resumable <var> (new wait_result_resumable (resumable_id, timeout));
}

int wait_queue_push (int queue_id, int resumable_id) {
  if (!is_forked_resumable_id(resumable_id)) {
    php_warning ("Wrong resumable_id %d in function wait_queue_push", resumable_id);

    return queue_id;
  }
  forked_resumable_info *resumable = get_forked_resumable_info (resumable_id);
  if (resumable->queue_id != 0 && resumable->queue_id != -1) {
    php_warning ("Someone already waits resumable %d", resumable_id);

    return queue_id;
  }
  if (resumable->queue_id < 0 && resumable->output.getter_ == NULL) {
    return queue_id;
  }

  if (queue_id == -1) {
    queue_id = f$wait_queue_create();
  } else {
    if (!is_wait_queue_id (queue_id)) {
      php_warning ("Wrong queue_id %d", queue_id);

      return queue_id;
    }
  }
  wait_queue *q = get_wait_queue (queue_id);

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

int wait_queue_push_unsafe (int queue_id, int resumable_id) {
  if (queue_id == -1) {
    queue_id = f$wait_queue_create();
  }

  forked_resumable_info *resumable = get_forked_resumable_info (resumable_id);
  wait_queue *q = get_wait_queue (queue_id);
  if (resumable->queue_id == 0) {
    resumable->queue_id = queue_id;
    q->left_functions++;
  } else {
    resumable->queue_id = q->first_finished_function;
    q->first_finished_function = -resumable_id;
  }

  return queue_id;
}

int first_free_queue_id;

void unregister_wait_queue(int queue_id) {
  if (queue_id == -1) {
    return;
  }
  get_wait_queue (queue_id)->resumable_id = -first_free_queue_id - 1;
  first_free_queue_id = queue_id;
}

int f$wait_queue_create (void) {
  int res_id;
  if (first_free_queue_id != 0) {
    res_id = first_free_queue_id;
    first_free_queue_id = -wait_queues[res_id - 1].resumable_id - 1;
    php_assert (0 <= first_free_queue_id && first_free_queue_id <= wait_next_queue_id);
  } else {
    if (wait_next_queue_id >= wait_queues_size) {
      php_assert (wait_next_queue_id == wait_queues_size);
      wait_queues = static_cast <wait_queue *> (dl::reallocate (wait_queues, sizeof (wait_queue) * 2 * wait_queues_size, sizeof (wait_queue) * wait_queues_size));
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

int f$wait_queue_create (const var &resumable_ids) {
  return f$wait_queue_push (-1, resumable_ids);
}

int f$wait_queue_push (int queue_id, const var &resumable_ids) {
  if (queue_id == -1) {
    queue_id = f$wait_queue_create();
  }

  if (resumable_ids.is_array()) {
    for (array <var>::const_iterator p = resumable_ids.begin(), p_end = resumable_ids.end(); p != p_end; ++p) {
      wait_queue_push (queue_id, f$intval (p.get_value()));
    }
    return queue_id;
  } else {
    return wait_queue_push (queue_id, f$intval (resumable_ids));
  }
}

int wait_queue_create (const array <int> &resumable_ids) {
  if (resumable_ids.count() == 0) {
    return -1;
  }

  int queue_id = f$wait_queue_create();

  for (array <int>::const_iterator p = resumable_ids.begin(), p_end = resumable_ids.end(); p != p_end; ++p) {
    wait_queue_push (queue_id, p.get_value());
  }
  return queue_id;
}


static void wait_queue_skip_gotten (wait_queue *q) {
  while (q->first_finished_function != -2 ) {
    forked_resumable_info *resumable = get_forked_resumable_info (-q->first_finished_function);
    if (resumable->output.getter_ != NULL) {
      break;
    }
    q->first_finished_function = resumable->queue_id;
    resumable->queue_id = -1;
    php_assert (q->first_finished_function != -1);
  }
}

bool f$wait_queue_empty (int queue_id) {
  if (!is_wait_queue_id (queue_id)) {
    if (queue_id != -1) {
      php_warning("Wrong queue_id %d in function wait_queue_empty", queue_id);
    }
    return true;
  }

  wait_queue *q = get_wait_queue (queue_id);
  wait_queue_skip_gotten (q);
  return q->left_functions == 0 && q->first_finished_function == -2;
}

static void wait_queue_next (int queue_id, double timeout) {
  php_assert (timeout > get_precise_now());//TODO remove asserts
  php_assert (in_main_thread());//TODO remove asserts
  php_assert (is_wait_queue_id (queue_id));

  wait_queue *q = get_wait_queue (queue_id);

  do {
    if (q->first_finished_function != -2) {
      return;
    }

    run_scheduller (timeout);

    q = get_wait_queue (queue_id);//can change in scheduller
    wait_queue_skip_gotten (q);
    if (q->first_finished_function != -2 || q->left_functions == 0) {
      return;
    }

    if (get_precise_now() > timeout) {
      return;
    }
    update_precise_now();
    
    if (resumable_has_finished()) {
      wait_net (0);
      continue;
    }
  } while (resumable_has_finished() || wait_net (timeout_convert_to_ms (timeout - get_precise_now())));

  return;
}

int wait_queue_next_synchronously (int queue_id) {
  wait_queue *q = get_wait_queue (queue_id);
  wait_queue_skip_gotten (q);

  if (q->first_finished_function != -2) {
    return -q->first_finished_function;
  }

  if (q->left_functions == 0) {
    return 0;
  }

  update_precise_now();
  while (wait_net (MAX_TIMEOUT * 1000) && q->first_finished_function == -2) {
    update_precise_now();
  } 

  return q->first_finished_function == -2 ? 0 : -q->first_finished_function;
}

static int wait_queue_timeout_wakeup_id = -1;

class wait_queue_resumable: public Resumable {
  int queue_id;
  event_timer *timer;
protected:
  bool run (void) {
    if (timer != NULL) {
      remove_event_timer (timer);
      timer = NULL;
    }

    wait_queue *q = get_wait_queue (queue_id);

    php_assert (q->resumable_id > 0);
    q->resumable_id = 0;

    if (q->first_finished_function != -2) {
      output_->save <int> (-q->first_finished_function);
    } else {
      php_assert (input_ == NULL);//timeout
      output_->save <int> (0);
    }

    queue_id = -1;
    return true;
  }

public:
  wait_queue_resumable (int queue_id): queue_id (queue_id), timer (NULL) {
  }

  void set_timer (double timeout, int wakeup_extra) {
    timer = allocate_event_timer (timeout, wait_queue_timeout_wakeup_id, wakeup_extra);
  }
};

void process_wait_queue_timeout (int wait_queue_resumable_id) {
  php_assert (is_started_resumable_id (wait_queue_resumable_id));

  resumable_run_ready (wait_queue_resumable_id);
}

int f$wait_queue_next (int queue_id, double timeout) {
  resumable_finished = true;

//  fprintf (stderr, "Waiting for queue %d\n", queue_id);
  if (!is_wait_queue_id (queue_id)) {
    if (queue_id != -1) {
      php_warning ("Wrong queue_id %d in function wait_queue_next", queue_id);
    }

    return 0;
  }

  wait_queue *q = get_wait_queue (queue_id );
  wait_queue_skip_gotten (q);
  int finished_slot_id = q->first_finished_function;
  if (finished_slot_id != -2) {
    php_assert (finished_slot_id != -1);
    return -finished_slot_id;
  }
  if (q->left_functions == 0) {
    return 0;
  }

  if (timeout == 0.0) {
    wait_net (0);

    return q->first_finished_function == -2 ? 0 : -q->first_finished_function;
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
    wait_queue_next (queue_id, timeout);

    q = get_wait_queue (queue_id);//can change in scheduller
    return q->first_finished_function == -2 ? 0 : -q->first_finished_function;
  }

  wait_queue_resumable *res = new wait_queue_resumable (queue_id);
  q->resumable_id = register_started_resumable (res);
  if (has_timeout) {
    res->set_timer (timeout, q->resumable_id);
  }

  resumable_finished = false;
  return 0;
}

int f$wait_queue_next_synchronously (int queue_id) {
  if (!is_wait_queue_id (queue_id)) {
    if (queue_id != -1) {
      php_warning ("Wrong queue_id %d in function wait_queue_next_synchronously", queue_id);
    }

    return 0;
  }

  return wait_queue_next_synchronously (queue_id);
}

void f$sched_yield (void) {
  if (in_main_thread()) {
    resumable_finished = true;
    return;
  }

  resumable_finished = false;

  int id = register_started_resumable (NULL);

  get_started_storage (id)->save_void();

  yielded_resumables_push (id);
}

void resumable_init_static_once (void) {
  php_assert (wait_timeout_wakeup_id == -1);
  php_assert (wait_queue_timeout_wakeup_id == -1);

  wait_timeout_wakeup_id = register_wakeup_callback (&process_wait_timeout);
  wait_queue_timeout_wakeup_id = register_wakeup_callback (&process_wait_queue_timeout);
}

void resumable_init_static (void) {
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
  forked_resumables = static_cast <forked_resumable_info *> (dl::allocate (sizeof (forked_resumable_info) * forked_resumables_size));

  started_resumables_size = 170;
  started_resumables = static_cast <started_resumable_info *> (dl::allocate (sizeof (started_resumable_info) * started_resumables_size));

  finished_resumables_size = 170;
  finished_resumables_count = 0;
  finished_resumables = static_cast <int *> (dl::allocate (sizeof (int) * finished_resumables_size));

  yielded_resumables_size = 170;
  yielded_resumables_l = yielded_resumables_r = 0;
  yielded_resumables = static_cast <int *> (dl::allocate (sizeof (int) * yielded_resumables_size));

  wait_queues_size = 101;
  wait_queues = static_cast <wait_queue *> (dl::allocate (sizeof (wait_queue) * wait_queues_size));
  wait_next_queue_id = 0;
  first_free_queue_id = 0;
  new(&gotten_forked_resumable_info.output) Storage;
  gotten_forked_resumable_info.queue_id = -1;
  gotten_forked_resumable_info.continuation = NULL;
}
