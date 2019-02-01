#pragma once

#include "runtime/exception.h"
#include "runtime/kphp_core.h"

extern bool resumable_finished;

extern const char *last_wait_error;

#define WAIT return false;
#define RETURN(x) output_->save <ReturnT> (x); return true;
#define RETURN_VOID() output_->save_void (); return true;
#define TRY_WAIT(labelName, a, T) if (!resumable_finished) { pos__ = &&labelName; WAIT; labelName: php_assert (input_ != nullptr); a = input_->load <T, T> (); }
#define TRY_WAIT_DROP_RESULT(labelName, T) if (!resumable_finished) { pos__ = &&labelName; WAIT; labelName: php_assert (input_ != nullptr); input_->load <T, T> (); }
#define RESUMABLE_BEGIN if (pos__ != nullptr) goto *pos__; do {
#define RESUMABLE_END \
      } while (0); \
      php_assert (0);\
      return false;\


class Storage {
private:
  char storage_[sizeof(var)];

  template<class X, class Y, class Tag = typename std::is_convertible<X, Y>::type>
  struct load_implementation_helper;

  static var load_exception(char *storage);

  void save_exception();

public:
  typedef var (*Getter)(char *);

  Getter getter_;

  Storage();

  template<class T1, class T2>
  void save(const T2 &x, Getter getter);

  template<class T1, class T2>
  void save(const T2 &x);

  void save_void();

  template<class X, class Y>
  Y load();

  var load_as_var();
};

class Resumable {
protected:
  static Storage *input_;
  static Storage *output_;
  void *pos__;

  virtual bool run() = 0;

public:
  void *operator new(size_t size);
  void operator delete(void *ptr, size_t size);

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

var f$wait_result(int resumable_id, double timeout = -1.0);

void f$sched_yield();


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
int f$wait_queue_create(const var &request_ids);
int wait_queue_create(const array<int> &resumable_ids);
void unregister_wait_queue(int queue_id);

int f$wait_queue_push(int queue_id, const var &request_ids);
int wait_queue_push(int queue_id, int request_id);
int wait_queue_push_unsafe(int queue_id, int resumable_id);

bool f$wait_queue_empty(int queue_id);

int f$wait_queue_next(int queue_id, double timeout = -1.0);

int wait_queue_next_synchronously(int queue_id);
int f$wait_queue_next_synchronously(int queue_id);

void resumable_init_static_once();

void resumable_init_static();

int f$get_running_fork_id();

/*
 *
 *     IMPLEMENTATION
 *
 */


template<class X>
struct Storage::load_implementation_helper<X, var, std::false_type> {
  static var load(char *storage __attribute__((unused))) {
    php_assert(0);      // should be never called in runtime, used just to prevent compilation errors
    return var();
  }
};

template<class X, class Y>
struct Storage::load_implementation_helper<X, Y, std::true_type> {
  static Y load(char *storage) {
    if (sizeof(X) > sizeof(storage_)) {
      // какие-нибудь длинные tuple'ы (см. save())
      // тогда в storage лежит указатель на выделенную память
      storage = static_cast<char *>(*reinterpret_cast<void **>(storage));
    }
    X *data = reinterpret_cast <X *> (storage);
    Y result = *data;
    data->~X();
    if (sizeof(X) > sizeof(storage_)) {
      dl::deallocate(storage, sizeof(X));
    }
    return result;
  }
};

template<>
struct Storage::load_implementation_helper<void, void, std::true_type> {
  static void load(char *storage __attribute__((unused))) {}
};

template<>
struct Storage::load_implementation_helper<void, var, std::false_type> {
  static var load(char *storage __attribute__((unused))) { return var(); }
};


template<class T1, class T2>
void Storage::save(const T2 &x, Getter getter) {
  if (CurException) {
    save_exception();
  } else {
    if (sizeof(T1) <= sizeof(storage_)) {
      #pragma GCC diagnostic push
      #if __GNUC__ >= 6
      #pragma GCC diagnostic ignored "-Wplacement-new="
      #endif
      new(storage_) T1(x);
      #pragma GCC diagnostic pop
    } else {
      // какие-нибудь длинные tuple'ы, которые не влазят в var
      // для них выделяем память отдельно, а в storage сохраняем указатель
      void *mem = dl::allocate(sizeof(T1));
      new(mem) T1(x);
      *reinterpret_cast<void **>(storage_) = mem;
    }

    getter_ = getter;
    php_assert (getter_ != nullptr);
  }
}

template<class T1, class T2>
void Storage::save(const T2 &x) {
  save<T1, T2>(x, load_implementation_helper<T1, var>::load);
}

template<class X, class Y>
Y Storage::load() {
  php_assert (getter_ != nullptr);
  if (getter_ == load_exception) {
    getter_ = nullptr;
    load_exception(storage_);
    return Y();
  }

  getter_ = nullptr;
  return load_implementation_helper<X, Y>::load(storage_);
}

template<class T>
T start_resumable(Resumable *resumable) {
  int id = register_started_resumable(resumable);

  if (resumable->resume(id, nullptr)) {
    Storage *output = get_started_storage(id);
    finish_started_resumable(id);
    unregister_started_resumable(id);
    resumable_finished = true;
    return output->load<T, T>();
  }

  if (in_main_thread()) {
    php_assert (wait_started_resumable(id));
    Storage *output = get_started_storage(id);
    resumable_finished = true;
    unregister_started_resumable(id);
    return output->load<T, T>();
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
