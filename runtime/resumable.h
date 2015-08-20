#pragma once

#include "kphp_core.h"

#include "exception.h"

extern bool resumable_finished;

extern const char *last_wait_error;

#define WAIT return false;
#define RETURN(x) output_->save <ReturnT> (x); return true;
#define TRY_WAIT(labelName, a, T) if (!resumable_finished) { pos__ = &&labelName; WAIT; labelName: php_assert (input_ != NULL); a = input_->load <T, T> (); }
#define TRY_WAIT_VOID(labelName) if (!resumable_finished) { pos__ = &&labelName; WAIT; labelName: ; }
#define RESUMABLE_BEGIN if (pos__ != NULL) goto *pos__; do {
#define RESUMABLE_END \
      } while (0); \
      php_assert (0);\
      return false;\

#ifndef FAST_EXCEPTIONS
#  error C++ exceptions doesn''t supported
#endif

class Storage {
private:
  char storage_[sizeof (var)];

  template <class X, class Y>
  static Y load_implementation (char *storage);

  static var load_exception (char *storage);

public:
  typedef var (*Getter) (char *);

  Getter getter_;

  Storage();

  template <class T1, class T2>
  void save (const T2 &x, Getter getter = load_implementation <T1, var>);

  template <class X, class Y>
  Y load();

  var load_as_var();
};

class Resumable {
protected:
  static Storage *input_;
  static Storage *output_;
  void* pos__;
  int pos_old;

  virtual bool run() = 0;

public:
  void *operator new (size_t size);
  void operator delete (void *ptr, size_t size);

  Resumable();
  explicit Resumable (int pos);

  virtual ~Resumable();

  bool resume (int resumable_id, Storage *input);

  static void update_output (void);
};


template <class T>
T start_resumable (Resumable *resumable);

template <class T>
int fork_resumable (Resumable *resumable);


void resumable_run_ready (int resumable_id);


bool wait_started_resumable (int resumable_id);

void wait_synchronously (int resumable_id);

bool f$wait_synchronously (int resumable_id);

bool f$wait (int resumable_id, double timeout = -1.0);

var f$wait_result (int resumable_id, double timeout = -1.0);

void f$sched_yield (void);


bool in_main_thread (void);

int register_forked_resumable (Resumable *resumable);

int register_started_resumable (Resumable *resumable);

void finish_forked_resumable (int resumable_id);

void finish_started_resumable (int resumable_id);

void unregister_started_resumable (int resumable_id);

Storage *get_started_storage (int resumable_id);

Storage *get_forked_storage (int resumable_id);

Resumable *get_forked_resumable (int resumable_id);


int f$wait_queue_create (void);
int f$wait_queue_create (const var &request_ids);
int wait_queue_create (const array <int> &resumable_ids);

int f$wait_queue_push (int queue_id, const var &request_ids);
int wait_queue_push (int queue_id, int request_id);
int wait_queue_push_unsafe (int queue_id, int resumable_id);

bool f$wait_queue_empty (int queue_id);

int f$wait_queue_next (int queue_id, double timeout = -1.0);

int wait_queue_next_synchronously (int queue_id);
int f$wait_queue_next_synchronously (int queue_id);

void resumable_init_static_once (void);

void resumable_init_static (void);


/*
 *
 *     IMPLEMENTATION
 *
 */


template <class X, class Y>
Y Storage::load_implementation (char *storage) {
  X *data = reinterpret_cast <X *> (storage);
  Y result = *data;
  data->~X();
  return result;
}

template <class T1, class T2>
void Storage::save (const T2 &x, Getter getter) {
  if (CurException) {
    php_assert (sizeof (Exception *) <= sizeof (var));
    *reinterpret_cast <Exception **> (storage_) = CurException;
    CurException = NULL;
    getter_ = load_exception;
  } else {
    php_assert (sizeof (T1) <= sizeof (var));
    new (storage_) T1 (x);
    getter_ = getter;
    php_assert (getter_ != NULL);
  }
}

template <class X, class Y>
Y Storage::load() {
  php_assert (getter_ != NULL);
  if (getter_ == load_exception) {
    getter_ = NULL;
    load_exception (storage_);
    return Y();
  }

  getter_ = NULL;
  return load_implementation <X, Y> (storage_);
}



template <class T>
T start_resumable (Resumable *resumable) {
  int id = register_started_resumable (resumable);

  if (resumable->resume (id, NULL)) {
    Storage *output = get_started_storage (id);
    finish_started_resumable (id);
    unregister_started_resumable (id);
    resumable_finished = true;
    return output->load <T, T>();
  }

  if (in_main_thread()) {
    php_assert (wait_started_resumable (id));
    Storage *output = get_started_storage (id);
    resumable_finished = true;
    unregister_started_resumable (id);
    return output->load <T, T>();
  }

  resumable_finished = false;
  return T();
}

template <class T>
int fork_resumable (Resumable *resumable) {
  int id = register_forked_resumable (resumable);

  if (resumable->resume (id, NULL)) {
    finish_forked_resumable (id);
  }

  return id;
}
