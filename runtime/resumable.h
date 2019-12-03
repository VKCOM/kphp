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
#define TRY_WAIT(labelName, a, T) if (!resumable_finished) { pos__ = &&labelName; WAIT; labelName: php_assert (input_ != nullptr); a = input_->load <T> (); }
#define TRY_WAIT_DROP_RESULT(labelName, T) if (!resumable_finished) { pos__ = &&labelName; WAIT; labelName: php_assert (input_ != nullptr); input_->load <T> (); }
#define RESUMABLE_BEGIN if (pos__ != nullptr) goto *pos__; do {
#define RESUMABLE_END \
      } while (0); \
      php_assert (0);\
      return false;\


class Resumable : public ManagedThroughDlAllocator {
protected:
  static Storage *input_;
  static Storage *output_;
  void *pos__;

  virtual bool run() = 0;

public:
  Resumable();

  virtual ~Resumable();

  bool resume(int resumable_id, Storage *input);

  static void update_output();

  void* get_stack_ptr() { return pos__; }
};


template<class T>
T start_resumable(Resumable *resumable);

template<class T>
int fork_resumable(Resumable *resumable);


void resumable_run_ready(int resumable_id);


bool wait_started_resumable(int resumable_id);

void wait_synchronously(int resumable_id);

bool f$wait_synchronously(int resumable_id);

bool f$wait(int resumable_id, double timeout = -1.0);

bool f$wait_multiple(int resumable_id);

inline bool f$wait_synchronously(Optional<int> resumable_id) { return f$wait_synchronously(resumable_id.val());}

inline bool f$wait(Optional<int> resumable_id, double timeout = -1.0) { return f$wait(resumable_id.val(), timeout); }

inline bool f$wait_multiple(Optional<int> resumable_id) { return f$wait_multiple(resumable_id.val());};

inline bool f$wait_multiple(const var &resumable_id) { return f$wait_multiple(resumable_id.to_int());};


void f$sched_yield();
void f$sched_yield_sleep(double timeout);


bool in_main_thread();

int register_forked_resumable(Resumable *resumable);

int register_started_resumable(Resumable *resumable);

void finish_forked_resumable(int resumable_id);

void finish_started_resumable(int resumable_id);

void unregister_started_resumable(int resumable_id);

Storage *get_started_storage(int resumable_id);

Storage *get_forked_storage(int resumable_id);

Resumable *get_forked_resumable(int resumable_id);

int get_resumable_stack(void **buffer, int limit);

int f$wait_queue_create();
int f$wait_queue_create(const var &resumable_ids);
int wait_queue_create(const array<int> &resumable_ids);
void unregister_wait_queue(int queue_id);

int f$wait_queue_push(int queue_id, const var &resumable_ids);
int wait_queue_push(int queue_id, int resumable_id);
int wait_queue_push_unsafe(int queue_id, int resumable_id);

bool f$wait_queue_empty(int queue_id);

Optional<int> f$wait_queue_next(int queue_id, double timeout = -1.0);
Optional<int> wait_queue_next_synchronously(int queue_id);
Optional<int> f$wait_queue_next_synchronously(int queue_id);

void global_init_resumable_lib();

void init_resumable_lib();

int f$get_running_fork_id();
Optional<array<var>> f$get_fork_stat(int fork_id);

/*
 *
 *     IMPLEMENTATION
 *
 */


template<class T>
T start_resumable(Resumable *resumable) {
  int id = register_started_resumable(resumable);

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
int fork_resumable(Resumable *resumable) {
  int id = register_forked_resumable(resumable);

  if (resumable->resume(id, nullptr)) {
    finish_forked_resumable(id);
  }

  return id;
}

template<typename T>
class wait_result_resumable : public Resumable {
  using ReturnT = T;
  int resumable_id;
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
        RETURN(false);
      }
      Storage *output = get_forked_storage(resumable_id);

      if (output->tag == 0) {
        last_wait_error = "Result already was gotten";
        RETURN(false);
      }

      RETURN(output->load_as<T>());
    RESUMABLE_END
  }

public:
  wait_result_resumable(int resumable_id, double timeout) :
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
T f$wait_result(int resumable_id, double timeout = -1) {
  return start_resumable<T>(new wait_result_resumable<T>(resumable_id, timeout));
}

template<typename T>
T f$wait_result(Optional<int> resumable_id, double timeout = -1) {
  return f$wait_result<T>(resumable_id.val(), timeout);
}

template<typename T>
class wait_result_multi_resumable : public Resumable {
  using ReturnT = T;
  array<int> resumable_ids;
  T result;

  array<int>::const_iterator resumable_it;
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
  explicit wait_result_multi_resumable(const array<int> &resumable_ids) :
    resumable_ids(resumable_ids),
    result(resumable_ids.size()) {
  }
};

template<typename T>
T f$wait_result_multi(const array<Optional<int>> &resumable_ids) {
  auto ids = array<int>::convert_from(resumable_ids);
  return f$wait_result_multi<T>(ids);
}

template<typename T>
T f$wait_result_multi(const array<int> &resumable_ids) {
  return start_resumable<T>(new wait_result_multi_resumable<T>(resumable_ids));
}

