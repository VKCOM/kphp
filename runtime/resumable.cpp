#include "net_events.h"
#include "resumable.h"

bool resumable_finished;

const char *last_wait_error;

static int runned_resumable_id;

Storage::Storage():
  getter_ (NULL) {
  memset (storage_, 0, sizeof (var));
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
};

struct started_resumable_info {
  Storage output;
  Resumable *continuation;
  int parent_id;// x > 0 - has parent x, == 0 - in main thread, == -1 - finished
  int fork_id;
};

static int first_forked_resumable_id;
static int current_forked_resumable_id = 1123456789;
static forked_resumable_info *forked_resumables;
static int forked_resumables_size;

static int first_started_resumable_id;
static int current_started_resumable_id = 123456789;
static started_resumable_info *started_resumables;
static int started_resumables_size;
static int first_free_started_resumable_id;


struct wait_queue {
  int first_finished_function;
  int left_functions;
  int resumable_id;//0 - default, x > 0 - id of wait_queue_next resumable
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

int register_forked_resumable (Resumable *resumable) {
  if (current_forked_resumable_id == first_forked_resumable_id + forked_resumables_size) {
    forked_resumables = static_cast <forked_resumable_info *> (dl::reallocate (forked_resumables, sizeof (forked_resumable_info) * 2 * forked_resumables_size, sizeof (forked_resumable_info) * forked_resumables_size));
    forked_resumables_size *= 2;

    Resumable::update_output();
  }
  if (current_forked_resumable_id >= 2000000000) {
    php_critical_error ("too many forked resumables");
  }

  forked_resumable_info *res = &forked_resumables[current_forked_resumable_id - first_forked_resumable_id];

  new (&res->output) Storage();
  res->continuation = resumable;
  res->queue_id = 0;

  return current_forked_resumable_id++;
}

int f$get_running_fork_id() {
  if (runned_resumable_id == 0) {
    return 0;
  }
  if (first_forked_resumable_id <= runned_resumable_id && runned_resumable_id < current_forked_resumable_id) {
    return runned_resumable_id;
  }
  if (first_started_resumable_id <= runned_resumable_id && runned_resumable_id < current_started_resumable_id) {
    return started_resumables[runned_resumable_id - first_started_resumable_id].fork_id;
  }
  php_assert (false);
  return 0;
}

int register_started_resumable (Resumable *resumable) {
  int res_id;
  bool is_new = false;
  if (first_free_started_resumable_id) {
    res_id = first_free_started_resumable_id;
    first_free_started_resumable_id = started_resumables[first_free_started_resumable_id - first_started_resumable_id].parent_id;
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

  started_resumable_info *res = &started_resumables[res_id - first_started_resumable_id];

  php_assert (is_new || res->output.getter_ == NULL);

  new (&res->output) Storage();
  res->continuation = resumable;
  res->parent_id = runned_resumable_id;
  res->fork_id = f$get_running_fork_id();

  return res_id;
}

Storage *get_started_storage (int resumable_id) {
  php_assert (first_started_resumable_id <= resumable_id && resumable_id < current_started_resumable_id);
  return &started_resumables[resumable_id - first_started_resumable_id].output;
}

Storage *get_forked_storage (int resumable_id) {
  php_assert (first_forked_resumable_id <= resumable_id && resumable_id < current_forked_resumable_id);
  return &forked_resumables[resumable_id - first_forked_resumable_id].output;
}

bool check_started_storage(Storage* storage) {
  return ((void*)started_resumables <= (void*)storage && (void*)storage < (void*)(started_resumables + started_resumables_size));
}

bool check_forked_storage(Storage* storage) {
  return ((void*)forked_resumables <= (void*)storage && (void*)storage < (void*)(forked_resumables + forked_resumables_size));
}


Resumable *get_forked_resumable (int resumable_id) {
  php_assert (first_forked_resumable_id <= resumable_id && resumable_id < current_forked_resumable_id);
  return forked_resumables[resumable_id - first_forked_resumable_id].continuation;
}

static void add_resumable_to_queue (int resumable_id, forked_resumable_info *resumable) {
  php_assert (resumable->queue_id <= wait_next_queue_id);

  int queue_id = resumable->queue_id;
  wait_queue *q = &wait_queues[queue_id - 1];
//  fprintf (stderr, "Push resumable %d to queue %d(%d,%d,%d) at %.6lf\n", resumable_id, resumable->queue_id, q->resumable_id, q->first_finished_function, q->left_functions, (update_precise_now(), get_precise_now()));
  resumable->queue_id = q->first_finished_function;
  q->first_finished_function = -resumable_id;

  if (q->resumable_id) {
    resumable_run_ready (q->resumable_id);
    q = &wait_queues[queue_id - 1]; // can be reallocated in run_ready
  }

  q->left_functions--;
}

void finish_forked_resumable (int resumable_id) {
  php_assert (first_forked_resumable_id <= resumable_id && resumable_id < current_forked_resumable_id);

  int slot_id = resumable_id - first_forked_resumable_id;
  forked_resumable_info *res = &forked_resumables[slot_id];
  php_assert (res->continuation != NULL);

  delete res->continuation;
  res->continuation = NULL;

  if (res->queue_id > 100000000) {
    php_assert (first_started_resumable_id <= res->queue_id && res->queue_id < current_started_resumable_id);
    int wait_resumable_id = res->queue_id;
    res->queue_id = -1;
    resumable_run_ready (wait_resumable_id);
  } else if (res->queue_id > 0) {
    add_resumable_to_queue (resumable_id, res);
  } else {
    res->queue_id = -1;
  }

  php_assert (forked_resumables[slot_id].queue_id < 0);
}

void finish_started_resumable (int resumable_id) {
  php_assert (first_started_resumable_id <= resumable_id && resumable_id < current_started_resumable_id);

  int slot_id = resumable_id - first_started_resumable_id;
  started_resumable_info *res = &started_resumables[slot_id];
  php_assert (res->continuation != NULL);

  delete res->continuation;
  res->continuation = NULL;
}

void unregister_started_resumable (int resumable_id) {
  php_assert (first_started_resumable_id <= resumable_id && resumable_id < current_started_resumable_id);

  int slot_id = resumable_id - first_started_resumable_id;
  started_resumable_info *res = &started_resumables[slot_id];
  php_assert (res->continuation == NULL);

  res->parent_id = first_free_started_resumable_id;
  first_free_started_resumable_id = resumable_id;
}


static bool resumable_has_finished (void) {
  return finished_resumables_count > 0 || yielded_resumables_l != yielded_resumables_r;
}

static void yielded_resumables_push(int id) {
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

static int yielded_resumables_pop() {
  php_assert (yielded_resumables_l != yielded_resumables_r);
  int result = yielded_resumables[yielded_resumables_l];
  yielded_resumables_l++;
  if (yielded_resumables_l == yielded_resumables_size) {
    yielded_resumables_l = 0;
  }
  return result;
}


static void resumable_add_finished (int resumable_id, bool is_long) {
  php_assert (first_started_resumable_id <= resumable_id && resumable_id < current_started_resumable_id);
  if (finished_resumables_count >= finished_resumables_size) {
    php_assert (finished_resumables_count == finished_resumables_size);
    finished_resumables = static_cast <int *> (dl::reallocate (finished_resumables, sizeof (int) * 2 * finished_resumables_size, sizeof (int) * finished_resumables_size));
    finished_resumables_size *= 2;
  }

//  fprintf(stderr, "Resumbale %d put to position %d of finised list\n", resumable_id, finished_resumables_count);
  finished_resumables[finished_resumables_count++] = is_long ? -resumable_id : resumable_id;
}

static void resumable_get_finished (int *resumable_id, bool *is_long) {
  php_assert (resumable_has_finished());
  if (finished_resumables_count) {
    if ((*resumable_id = finished_resumables[--finished_resumables_count]) < 0) {
      *resumable_id = -*resumable_id;
      *is_long = true;
    } else {
      *is_long = false;
    }
  } else {
    *resumable_id = yielded_resumables_pop();
    *is_long = true;
  }
}


void resumable_run_ready (int resumable_id) {
//  fprintf (stderr, "run ready %d\n", resumable_id);
  if (resumable_id > 1000000000) {
    php_assert (first_forked_resumable_id <= resumable_id && resumable_id < current_forked_resumable_id);

    int slot_id = resumable_id - first_forked_resumable_id;
    forked_resumable_info *res = &forked_resumables[slot_id];
    php_assert (res->queue_id >= 0);
    php_assert (res->continuation->resume (resumable_id, NULL));
    finish_forked_resumable (resumable_id);
  } else {
    php_assert (first_started_resumable_id <= resumable_id && resumable_id < current_started_resumable_id);

    int slot_id = resumable_id - first_started_resumable_id;
    started_resumable_info *res = &started_resumables[slot_id];
    php_assert (res->continuation->resume (resumable_id, NULL));
    finish_started_resumable (resumable_id);
    resumable_add_finished (resumable_id, false);
  }
}

void run_scheduller (double timeout) {
//  fprintf (stderr, "!!! run scheduller %d\n", finished_resumables_count);
  int left_resumables = 1000;
  while (resumable_has_finished() && --left_resumables >= 0) {
    if (get_precise_now() > timeout) {
      return;
    }

    int resumable_id;
    bool is_long;
    resumable_get_finished (&resumable_id, &is_long);
    if (is_long) {
      left_resumables = 0;
    }
    
    php_assert (first_started_resumable_id <= resumable_id && resumable_id < current_started_resumable_id);

    int slot_id = resumable_id - first_started_resumable_id;
    started_resumable_info *res = &started_resumables[slot_id];
    php_assert (res->continuation == NULL);

//    fprintf (stderr, "!!! process %d(%d) with parent %d in scheduller\n", resumable_id, is_long, res->parent_id);
    int parent_id = res->parent_id;
    if (parent_id == 0) {
      res->parent_id = -1;
      return;
    }

    php_assert (parent_id > 0);
    if (parent_id < 1000000000) {
      php_assert (first_started_resumable_id <= parent_id && parent_id < current_started_resumable_id);
      int parent_slot_id = parent_id - first_started_resumable_id;
      started_resumable_info *parent = &started_resumables[parent_slot_id];
      php_assert (parent->continuation != NULL);
      php_assert (parent->parent_id != -2);
      if (parent->continuation->resume (parent_id, &res->output)) {
        finish_started_resumable (parent_id);
        resumable_add_finished (parent_id, false);
        left_resumables++;
      }
    } else {
      php_assert (first_forked_resumable_id <= parent_id && parent_id < current_forked_resumable_id);
      int parent_slot_id = parent_id - first_forked_resumable_id;
      forked_resumable_info *parent = &forked_resumables[parent_slot_id];
      php_assert (parent->continuation != NULL);
      php_assert (parent->queue_id >= 0);
      if (parent->continuation->resume (parent_id, &res->output)) {
        finish_forked_resumable (parent_id);
      }
    }

    unregister_started_resumable (resumable_id);
  }
}


void wait_synchronously (int resumable_id) {
  php_assert (first_forked_resumable_id <= resumable_id && resumable_id < current_forked_resumable_id);

  int slot_id = resumable_id - first_forked_resumable_id;
  forked_resumable_info *resumable = &forked_resumables[slot_id];

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
  if (resumable_id < first_forked_resumable_id || resumable_id >= current_forked_resumable_id) {
    last_wait_error = "Wrong resumable id";
    return false;
  }

  int slot_id = resumable_id - first_forked_resumable_id;
  forked_resumable_info *resumable = &forked_resumables[slot_id];

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
  php_assert (first_forked_resumable_id <= resumable_id && resumable_id < current_forked_resumable_id);

  int slot_id = resumable_id - first_forked_resumable_id;
  forked_resumable_info *resumable = &forked_resumables[slot_id];

  do {
    if (resumable->queue_id < 0) {
      return true;
    }

    run_scheduller (timeout);

    resumable = &forked_resumables[slot_id];//can change in scheduller
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
  php_assert (first_started_resumable_id <= resumable_id && resumable_id < current_started_resumable_id);

  int slot_id = resumable_id - first_started_resumable_id;
  started_resumable_info *resumable = &started_resumables[slot_id];

  do {
    php_assert (resumable->parent_id == 0);

    run_scheduller (get_precise_now() + MAX_TIMEOUT);

    resumable = &started_resumables[slot_id];//can change in scheduller
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

    php_assert (first_forked_resumable_id <= child_id && child_id < current_forked_resumable_id);
    int slot_id = child_id - first_forked_resumable_id;
    forked_resumable_info *info = &forked_resumables[slot_id];

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
protected:
  bool run (void) {
    RESUMABLE_BEGIN
    while (true) {
      php_assert (first_forked_resumable_id <= child_id && child_id < current_forked_resumable_id);
      int slot_id = child_id - first_forked_resumable_id;
      forked_resumable_info *info = &forked_resumables[slot_id];

      if (info->queue_id < 0) {
        break;
      } else {
        if (!resumable_has_finished ()) {
          wait_net (MAX_TIMEOUT);
        }
        f$sched_yield ();
        TRY_WAIT_VOID (wait_many_resumable_label1);
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
  php_assert (first_started_resumable_id <= wait_resumable_id && wait_resumable_id < current_started_resumable_id);

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
      TRY_WAIT(wait_result_resumbale_label_1, ready, bool);
      if (!ready) {
        if (last_wait_error == NULL) {
          last_wait_error = "Timeout in wait_result";
        }
        RETURN(false);
      }

      int slot_id = resumable_id - first_forked_resumable_id;
      forked_resumable_info *resumable = &forked_resumables[slot_id];

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
  if (resumable_id < first_forked_resumable_id || resumable_id >= current_forked_resumable_id) {
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

  int slot_id = resumable_id - first_forked_resumable_id;
  forked_resumable_info *resumable = &forked_resumables[slot_id];

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
  if (resumable_id < first_forked_resumable_id || resumable_id >= current_forked_resumable_id) {
    last_wait_error = "Wrong resumable id";
    return false;
  }

  wait_net (0);

  int slot_id = resumable_id - first_forked_resumable_id;
  forked_resumable_info *resumable = &forked_resumables[slot_id];

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
  if (resumable_id < first_forked_resumable_id || resumable_id >= current_forked_resumable_id) {
    php_warning ("Wrong resumable_id %d in function wait_queue_push", resumable_id);

    return queue_id;
  }
  int slot_id = resumable_id - first_forked_resumable_id;
  forked_resumable_info *resumable = &forked_resumables[slot_id];
  if (resumable->queue_id != 0 && resumable->queue_id != -1) {
    php_warning ("Someone already waits resumable %d", resumable_id);

    return queue_id;
  }

  if (queue_id == -1) {
    queue_id = f$wait_queue_create();
  } else {
    if (queue_id <= 0 || queue_id > wait_next_queue_id) {
      php_warning ("Wrong queue_id %d", queue_id);

      return queue_id;
    }
  }
  wait_queue *q = &wait_queues[queue_id - 1];

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

  forked_resumable_info *resumable = &forked_resumables[resumable_id - first_forked_resumable_id];
  wait_queue *q = &wait_queues[queue_id - 1];
  if (resumable->queue_id == 0) {
    resumable->queue_id = queue_id;
    q->left_functions++;
  } else {
    resumable->queue_id = q->first_finished_function;
    q->first_finished_function = -resumable_id;
  }

  return queue_id;
}

int f$wait_queue_create (void) {
  if (wait_next_queue_id >= wait_queues_size) {
    php_assert (wait_next_queue_id == wait_queues_size);
    wait_queues = static_cast <wait_queue *> (dl::reallocate (wait_queues, sizeof (wait_queue) * 2 * wait_queues_size, sizeof (wait_queue) * wait_queues_size));
    wait_queues_size *= 2;
  }
  if (wait_next_queue_id >= 99999999) {
    php_critical_error ("too many wait queues");
  }

  wait_queue *q = &wait_queues[wait_next_queue_id];
  q->first_finished_function = -2;
  q->left_functions = 0;
  q->resumable_id = 0;

  return ++wait_next_queue_id;
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
  while (q->first_finished_function != -2 && forked_resumables[-q->first_finished_function - first_forked_resumable_id].output.getter_ == NULL) {
    q->first_finished_function = forked_resumables[-q->first_finished_function - first_forked_resumable_id].queue_id;
    php_assert (q->first_finished_function != -1);
  }
}

bool f$wait_queue_empty (int queue_id) {
  if (queue_id <= 0 || queue_id > wait_next_queue_id) {
    php_warning ("Wrong queue_id %d in function wait_queue_empty", queue_id);

    return true;
  }

  wait_queue *q = &wait_queues[queue_id - 1];
  wait_queue_skip_gotten (q);
  return q->left_functions == 0 && q->first_finished_function == -2;
}

static void wait_queue_next (int queue_id, double timeout) {
  php_assert (timeout > get_precise_now());//TODO remove asserts
  php_assert (in_main_thread());//TODO remove asserts
  php_assert (0 < queue_id && queue_id <= wait_next_queue_id);

  wait_queue *q = &wait_queues[queue_id - 1];

  do {
    if (q->first_finished_function != -2) {
      return;
    }

    run_scheduller (timeout);

    q = &wait_queues[queue_id - 1];//can change in scheduller
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
  wait_queue *q = &wait_queues[queue_id - 1];
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

    php_assert (0 < queue_id && queue_id <= wait_next_queue_id);
    wait_queue *q = &wait_queues[queue_id - 1];

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
  php_assert (first_started_resumable_id <= wait_queue_resumable_id && wait_queue_resumable_id < current_started_resumable_id);

  resumable_run_ready (wait_queue_resumable_id);
}

int f$wait_queue_next (int queue_id, double timeout) {
  resumable_finished = true;

//  fprintf (stderr, "Waiting for queue %d\n", queue_id);
  if (queue_id <= 0 || queue_id > wait_next_queue_id) {
    if (queue_id != -1) {
      php_warning ("Wrong queue_id %d in function wait_queue_next", queue_id);
    }

    return 0;
  }

  wait_queue *q = &wait_queues[queue_id - 1];
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

    q = &wait_queues[queue_id - 1];//can change in scheduller
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
  if (queue_id <= 0 || queue_id > wait_next_queue_id) {
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
  current_forked_resumable_id = first_forked_resumable_id;

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
  yielded_resumables = static_cast <int *> (dl::allocate (sizeof (int) * finished_resumables_size));

  wait_queues_size = 101;
  wait_queues = static_cast <wait_queue *> (dl::allocate (sizeof (wait_queue) * wait_queues_size));
  wait_next_queue_id = 0;
}
