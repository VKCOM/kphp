#pragma once

#include "runtime/allocator.h"
#include "runtime/exception.h"
#include "runtime/kphp_core.h"
#include "runtime/storage.h"

extern bool resumable_finished;

extern const char *last_wait_error;

#define WAIT return false;
#define RETURN(x) output_->save <ReturnT> (x); return true;
#define RETURN_VOID() output_->save_void (); return true;

#define TRY_WAIT(labelName, a, T)             \
  if (!resumable_finished) {                  \
    pos__ = &&labelName;                      \
    WAIT;                                     \
    labelName: php_assert (input_ != nullptr);\
    a = input_->load <T> ();                  \
  }

#define TRY_WAIT_DROP_RESULT(labelName, T)    \
  if (!resumable_finished) {                  \
    pos__ = &&labelName;                      \
    WAIT;                                     \
    labelName: php_assert (input_ != nullptr);\
    input_->load <T> ();                      \
  }

#define RESUMABLE_BEGIN   \
  if (pos__ != nullptr) { \
    goto *pos__;          \
  }                       \
  do {

#define RESUMABLE_END                   \
  } while (0);                          \
  pos__ = &&unreachable_end_label;      \
  unreachable_end_label: php_assert (0);\
  return false;                         \


class Resumable : public ManagedThroughDlAllocator {
protected:
  static Storage *input_;
  static Storage *output_;
  void *pos__;

  virtual bool run() = 0;

public:
  Resumable();

  virtual ~Resumable();

  bool resume(int64_t resumable_id, Storage *input);

  static void update_output();

  void* get_stack_ptr() { return pos__; }
};


template<class T>
T start_resumable(Resumable *resumable);

template<class T>
int64_t fork_resumable(Resumable *resumable);


void resumable_run_ready(int64_t resumable_id);


bool wait_started_resumable(int64_t resumable_id);

void wait_synchronously(int64_t resumable_id);

bool f$wait_synchronously(int64_t resumable_id);

bool f$wait(int64_t resumable_id, double timeout = -1.0);

bool f$wait_multiple(int64_t resumable_id);

inline bool f$wait_synchronously(Optional<int64_t> resumable_id) { return f$wait_synchronously(resumable_id.val());}

inline bool f$wait(Optional<int64_t> resumable_id, double timeout = -1.0) { return f$wait(resumable_id.val(), timeout); }

inline bool f$wait_multiple(Optional<int64_t> resumable_id) { return f$wait_multiple(resumable_id.val());};

inline bool f$wait_multiple(const mixed &resumable_id) { return f$wait_multiple(resumable_id.to_int());};


void f$sched_yield();
void f$sched_yield_sleep(double timeout);


bool in_main_thread();

int64_t register_forked_resumable(Resumable *resumable);

int64_t register_started_resumable(Resumable *resumable);

void finish_forked_resumable(int64_t resumable_id);

void finish_started_resumable(int64_t resumable_id);

void unregister_started_resumable(int64_t resumable_id);

Storage *get_started_storage(int64_t resumable_id);

Storage *get_forked_storage(int64_t resumable_id);

Resumable *get_forked_resumable(int64_t resumable_id);

int32_t get_resumable_stack(void **buffer, int32_t limit);

int64_t f$wait_queue_create();
int64_t f$wait_queue_create(const mixed &resumable_ids);
int64_t wait_queue_create(const array<int64_t> &resumable_ids);
void unregister_wait_queue(int64_t queue_id);

int64_t f$wait_queue_push(int64_t queue_id, const mixed &resumable_ids);
int64_t wait_queue_push(int64_t queue_id, int64_t resumable_id);
int64_t wait_queue_push_unsafe(int64_t queue_id, int64_t resumable_id);

bool f$wait_queue_empty(int64_t queue_id);

Optional<int64_t> f$wait_queue_next(int64_t queue_id, double timeout = -1.0);
Optional<int64_t> wait_queue_next_synchronously(int64_t queue_id);
Optional<int64_t> f$wait_queue_next_synchronously(int64_t queue_id);

void global_init_resumable_lib();

void init_resumable_lib();

int64_t f$get_running_fork_id();
Optional<array<mixed>> f$get_fork_stat(int64_t fork_id);

/*
 *
 *     IMPLEMENTATION
 *
 */


template<class T>
T start_resumable(Resumable *resumable) {
  int64_t id = register_started_resumable(resumable);

  if (resumable->resume(id, nullptr)) {
    Storage *output = get_started_storage(id);
    finish_started_resumable(id);
    unregister_started_resumable(id);
    resumable_finished = true;
    return output->load<T>();
  }

  if (in_main_thread()) {
    php_assert (wait_started_resumable(id));
    Storage *output = get_started_storage(id);
    resumable_finished = true;
    unregister_started_resumable(id);
    return output->load<T>();
  }

  resumable_finished = false;
  return T();
}

template<class T>
int64_t fork_resumable(Resumable *resumable) {
  int64_t id = register_forked_resumable(resumable);

  if (resumable->resume(id, nullptr)) {
    finish_forked_resumable(id);
  }

  return id;
}

template<typename T>
class wait_result_resumable : public Resumable {
  using ReturnT = T;
  int64_t resumable_id;
  double timeout;
  bool ready;

protected:
  bool run() {
    RESUMABLE_BEGIN
      ready = f$wait(resumable_id, timeout);
      TRY_WAIT(wait_result_resumable_label_1, ready, bool);
      if (!ready) {
        if (last_wait_error == nullptr) {
          last_wait_error = "Timeout in wait_result";
        }
        RETURN(Optional<bool>{});
      }
      Storage *output = get_forked_storage(resumable_id);

      if (output->tag == 0) {
        last_wait_error = "Result already was gotten";
        RETURN(Optional<bool>{});
      }

      RETURN(output->load_as<T>());
    RESUMABLE_END
  }

public:
  wait_result_resumable(int64_t resumable_id, double timeout) :
    resumable_id(resumable_id),
    timeout(timeout),
    ready(false) {
  }
};

template<>
inline bool wait_result_resumable<void>::run() {
  RESUMABLE_BEGIN
    ready = f$wait(resumable_id, timeout);
    TRY_WAIT(wait_result_resumable_label_1, ready, bool);
    if (!ready) {
      if (last_wait_error == nullptr) {
        last_wait_error = "Timeout in wait_result";
      }
      RETURN_VOID();
    }
    Storage *output = get_forked_storage(resumable_id);

    if (output->tag == 0) {
      last_wait_error = "Result already was gotten";
      RETURN_VOID();
    }

    output->load_as<void>();
    RETURN_VOID();
  RESUMABLE_END
}

template<typename T>
T f$wait_result(int64_t resumable_id, double timeout = -1) {
  return start_resumable<T>(new wait_result_resumable<T>(resumable_id, timeout));
}

template<typename T>
T f$wait_result(Optional<int64_t> resumable_id, double timeout = -1) {
  return f$wait_result<T>(resumable_id.val(), timeout);
}

template<typename T>
class wait_result_multi_resumable : public Resumable {
  using ReturnT = T;
  array<int64_t> resumable_ids;
  T result;

  array<int64_t>::const_iterator resumable_it;
  typename T::value_type next_waited_result;

protected:
  bool run() {
    RESUMABLE_BEGIN
      for (resumable_it = const_begin(resumable_ids); resumable_it != const_end(resumable_ids); ++resumable_it) {
        next_waited_result = f$wait_result<typename T::value_type>(resumable_it.get_value());

        TRY_WAIT(wait_result_multi_label, next_waited_result, typename T::value_type);
        CHECK_EXCEPTION(RETURN(T()));

        result.set_value(resumable_it.get_key(), next_waited_result);
      }

      RETURN(result);
    RESUMABLE_END
  }

public:
  explicit wait_result_multi_resumable(const array<int64_t> &resumable_ids) :
    resumable_ids(resumable_ids),
    result(resumable_ids.size()) {
  }
};

template<typename T>
T f$wait_result_multi(const array<Optional<int64_t>> &resumable_ids) {
  auto ids = array<int64_t>::convert_from(resumable_ids);
  return f$wait_result_multi<T>(ids);
}

template<typename T>
T f$wait_result_multi(const array<int64_t> &resumable_ids) {
  return start_resumable<T>(new wait_result_multi_resumable<T>(resumable_ids));
}

