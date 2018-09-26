#pragma once

#include "common/algorithms/find.h"

#include "compiler/common.h"
#include "compiler/stage.h"

extern volatile int bicycle_counter;

const int MAX_THREADS_COUNT = 101;
int get_thread_id();
void set_thread_id(int new_thread_id);

template<class T>
bool try_lock(T);

template<class T>
void lock(T locker) {
  locker->lock();
}

template<class T>
void unlock(T locker) {
  locker->unlock();
}

inline bool try_lock(volatile int *locker) {
  return __sync_lock_test_and_set(locker, 1) == 0;
}

inline void lock(volatile int *locker) {
  while (!try_lock(locker)) {
    usleep(250);
  }
}

inline void unlock(volatile int *locker) {
  assert (*locker == 1);
  __sync_lock_release(locker);
}

class Lockable {
private:
  volatile int x;
public:
  Lockable() :
    x(0) {}

  virtual ~Lockable() = default;

  void lock() {
    ::lock(&x);
  }

  void unlock() {
    ::unlock(&x);
  }
};

template<class DataT>
class AutoLocker {
private:
  DataT ptr;
public:
  explicit AutoLocker(DataT ptr) :
    ptr(ptr) {
    lock(ptr);
  }

  ~AutoLocker() {
    unlock(ptr);
  }
};

template<class ValueType>
class Maybe {
public:
  bool has_value = false;
  char data[sizeof(ValueType)];

  Maybe() = default;

  Maybe(const ValueType &value) :
    has_value(true) {
    new(data) ValueType(value);
  }

  operator const ValueType &() const {
    assert (has_value);
    return *(ValueType *)data;
  }

  bool empty() {
    return !has_value;
  }
};

inline int atomic_int_inc(volatile int *x) {
  int old_x;
  do {
    old_x = *x;
  } while (!__sync_bool_compare_and_swap(x, old_x, old_x + 1));
  return old_x;
}

inline void atomic_int_dec(volatile int *x) {
  int old_x;
  do {
    old_x = *x;
  } while (!__sync_bool_compare_and_swap(x, old_x, old_x - 1));
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

template<class T>
struct TLS {
private:
  struct TLSRaw {
    T data{};
    volatile int locker = 0;
    char dummy[4096];
  };

  TLSRaw arr[MAX_THREADS_COUNT + 1];
public:
  TLS() :
    arr() {
  }

  TLSRaw *get_raw(int id) {
    assert (0 <= id && id <= MAX_THREADS_COUNT);
    return &arr[id];
  }

  TLSRaw *get_raw() {
    return get_raw(get_thread_id());
  }

  T *get() {
    return &get_raw()->data;
  }

  T *get(int i) {
    return &get_raw(i)->data;
  }

  T *operator->() {
    return get();
  }

  T &operator*() {
    return *get();
  }

  int size() {
    return MAX_THREADS_COUNT + 1;
  }

  T *lock_get() {
    TLSRaw *raw = get_raw();
    bool ok = try_lock(&raw->locker);
    assert (ok);
    return &raw->data;
  }

  void unlock_get(T *ptr) {
    TLSRaw *raw = get_raw();
    assert (&raw->data == ptr);
    unlock(&raw->locker);
  }
};

#pragma GCC diagnostic pop

class Task {
public:
  virtual void execute() = 0;

  virtual ~Task() = default;
};

inline void execute_task(Task *task) {
  task->execute();
  delete task;
  atomic_int_dec(&bicycle_counter);
}

class Node;

class SchedulerBase {
public:
  SchedulerBase();
  virtual ~SchedulerBase();
  virtual void add_node(Node *node) = 0;
  virtual void add_sync_node(Node *node) = 0;
  virtual void add_task(Task *task) = 0;
  virtual void execute() = 0;
};

SchedulerBase *get_scheduler();
void set_scheduler(SchedulerBase *new_scheduler);
void unset_scheduler(SchedulerBase *old_scheduler);


inline void register_async_task(Task *task) {
  get_scheduler()->add_task(task);
}

class Node {
  SchedulerBase *in_scheduler;
  bool parallel;

public:
  explicit Node(bool parallel = true) :
    in_scheduler(nullptr),
    parallel(parallel) {}

  virtual ~Node() = default;

  void add_to_scheduler(SchedulerBase *scheduler) {
    if (in_scheduler == scheduler) {
      return;
    }
    assert (in_scheduler == nullptr);
    in_scheduler = scheduler;
    in_scheduler->add_node(this);
  }

  void add_to_scheduler_as_sync_node() {
    assert(in_scheduler);
    in_scheduler->add_sync_node(this);
  }

  virtual bool is_parallel() {
    return parallel;
  }

  virtual Task *get_task() = 0;
  virtual void on_finish() = 0;
};

#define DUMMY_ON_FINISH template <class OutputStreamT> void on_finish (OutputStreamT &os __attribute__((unused))) {}

class TaskPull;

class OneThreadScheduler : public SchedulerBase {
private:
  vector<Node *> nodes;
  queue<Node *> sync_nodes;
  TaskPull *task_pull;
public:
  OneThreadScheduler();

  void add_node(Node *node) override;
  void add_sync_node(Node *node) override;
  void add_task(Task *task) override;
  void execute() override;
  void set_threads_count(int threads_count);
};

class ThreadLocalStorage {
public:
  pthread_t pthread_id;
  int thread_id;

  class Scheduler *scheduler;

  Node *node;
  bool run_flag;
  double started;
  double finished;
};

void *thread_execute(void *arg);

class Scheduler : public SchedulerBase {
private:
  vector<Node *> nodes;
  vector<Node *> one_thread_nodes;
  queue<Node *> sync_nodes;
  int threads_count;
  TaskPull *task_pull;

public:
  Scheduler();

  void add_node(Node *node) override;
  void add_sync_node(Node *node) override;
  void add_task(Task *task) override;
  void execute() override;

  void set_threads_count(int new_threads_count);
  void thread_execute(ThreadLocalStorage *tls);
  bool thread_process_node(Node *node);
};

template<class DataT>
class DataStream : Lockable {
private:
  static const int MAX_STREAM_ELEMENTS = 200000;
  DataT *data;
  volatile int *ready;
  volatile int write_i, read_i;
  bool sink;
public:
  using DataType = DataT;
  using StreamType = DataStream<DataT>;

  template<size_t data_id>
  using NthDataType = DataType;

  DataStream() :
    write_i(0),
    read_i(0),
    sink(false) {
    //FIXME
    data = new DataT[MAX_STREAM_ELEMENTS]();
    ready = new int[MAX_STREAM_ELEMENTS]();
  }

  bool empty() {
    return read_i == write_i;
  }

  Maybe<DataT> get() {
    //AutoLocker <Lockable *> (this);
    while (true) {
      int old_read_i = read_i;
      if (old_read_i < write_i) {
        if (__sync_bool_compare_and_swap(&read_i, old_read_i, old_read_i + 1)) {
          while (!ready[old_read_i]) {
            usleep(250);
          }
          DataT result;
          std::swap(result, data[old_read_i]);
          return result;
        }
        usleep(250);
      } else {
        return {};
      }
    }
  }

  void operator<<(const DataType &input) {
    //AutoLocker <Lockable *> (this);
    if (!sink) {
      atomic_int_inc(&bicycle_counter);
    }
    while (true) {
      int old_write_i = write_i;
      assert (old_write_i < MAX_STREAM_ELEMENTS);
      if (__sync_bool_compare_and_swap(&write_i, old_write_i, old_write_i + 1)) {
        data[old_write_i] = input;
        __sync_synchronize();
        ready[old_write_i] = 1;
        return;
      }
      usleep(250);
    }
  }

  int size() {
    return write_i - read_i;
  }

  vector<DataT> get_as_vector() {
    return vector<DataT>(data + read_i, data + write_i);
  }

  void set_sink(bool new_sink) {
    if (new_sink == sink) {
      return;
    }
    sink = new_sink;
    if (sink) {
      bicycle_counter -= size();
    } else {
      bicycle_counter += size();
    }
  }
};

class TaskPull : public Node {
private:
  DataStream<Task *> stream;
public:
  inline void add_task(Task *task) {
    stream << task;
  }

  inline Task *get_task() override {
    Maybe<Task *> x = stream.get();
    if (x.empty()) {
      return nullptr;
    }
    return x;
  }

  inline void on_finish() override {}
};

template<class PipeType>
class PipeTask : public Task {
private:
  typedef typename PipeType::InputType InputType;
  InputType input;
  PipeType *pipe_ptr;
public:
  PipeTask(InputType input, PipeType *pipe_ptr) :
    input(input),
    pipe_ptr(pipe_ptr) {
  }

  void execute() override {
    pipe_ptr->process_input(input);
  }
};

template<class PipeF, class InputStreamT, class OutputStreamT>
class Pipe : public Node {
public:
  using PipeFunctionType = PipeF;
  using InputStreamType = InputStreamT;
  using OutputStreamType = OutputStreamT;

  using InputType = typename InputStreamT::DataType;
  using SelfType = Pipe<PipeF, InputStreamT, OutputStreamT>;
  using TaskType = PipeTask<SelfType>;

private:
  InputStreamType *input_stream = nullptr;
  OutputStreamType *output_stream = nullptr;
  PipeF function;

public:
  explicit Pipe(bool parallel = true) :
    Node(parallel) {}

  InputStreamType *&get_input_stream() { return input_stream; }

  OutputStreamType *&get_output_stream() { return output_stream; }

  void init_output_stream_if_not_inited() {
    if (!output_stream) {
      output_stream = new OutputStreamType();
    }
  }

  void set_input_stream(InputStreamType *is) { input_stream = is; }

  void process_input(const InputType &input) { function.execute(input, *output_stream); }

  Task *get_task() override {
    Maybe<InputType> x = input_stream->get();
    if (x.empty()) {
      return nullptr;
    }
    return new TaskType(x, this);
  }

  void on_finish() override { function.on_finish(*output_stream); }
};

struct EmptyStream {
  template<size_t stream_id>
  using NthDataType = EmptyStream;
};

template<class ...DataTypes>
class MultipleDataStreams {
private:
  std::tuple<DataStream<DataTypes> *...> streams_;

public:
  template<size_t data_id>
  using NthDataType = vk::get_nth_type<data_id, DataTypes...>;

  template<size_t stream_id>
  decltype(std::get<stream_id>(streams_)) &project_to_nth_data_stream() {
    return std::get<stream_id>(streams_);
  }

  template<class DataType>
  DataStream<DataType> *&project_to_single_data_stream() {
    constexpr size_t data_id = vk::index_of_type<DataType, DataTypes...>::value;
    return std::get<data_id>(streams_);
  }

  template<class DataType>
  void operator<<(const DataType &data) {
    *project_to_single_data_stream<DataType>() << data;
  }
};

template<size_t id, class StreamT>
using ConcreteIndexedStream = DataStream<typename StreamT::template NthDataType<id>>;

struct sync_node_tag {
};

struct use_previous_pipe_as_sync_node_tag {
};

template<size_t id>
struct use_nth_output_tag {
};

template<class StreamT>
class SC_Pipe {
private:
  SchedulerBase *scheduler;
  StreamT *&previous_output_stream;
  Node *previous_node;

private:
  class SyncPipeF {
    StreamT tmp_stream;
  public:
    SyncPipeF() { tmp_stream.set_sink(true); }

    void execute(typename StreamT::DataType input, StreamT &) { tmp_stream << input; }

    void on_finish(StreamT &os) {
      mem_info_t mem_info;
      get_mem_stats(getpid(), &mem_info);

      stage::die_if_global_errors();
      for (auto &el : tmp_stream.get_as_vector()) {
        os << el;
      }
    }
  };

  static void connect(StreamT *&first_stream, StreamT *&second_stream) {
    if (first_stream != nullptr && second_stream != nullptr) {
      assert(first_stream == second_stream);
      return;
    }

    if (first_stream != nullptr) {
      second_stream = first_stream;
    } else if (second_stream != nullptr) {
      first_stream = second_stream;
    } else {
      first_stream = new StreamT();
      second_stream = first_stream;
    }
  }

public:
  SC_Pipe(SchedulerBase *scheduler, StreamT *&stream, Node *previous_node) :
    scheduler(scheduler),
    previous_output_stream(stream),
    previous_node(previous_node) {}

  template<class PipeT>
  SC_Pipe(SchedulerBase *scheduler, PipeT *pipe) :
    scheduler(scheduler),
    previous_output_stream(pipe->get_output_stream()),
    previous_node(pipe) {
    pipe->add_to_scheduler(scheduler);
  }

  SC_Pipe operator>>(sync_node_tag) {
    auto *sync_pipe = new Pipe<SyncPipeF, StreamT, StreamT>();
    return *this >> *sync_pipe >> use_previous_pipe_as_sync_node_tag{};
  }

  template<size_t id>
  SC_Pipe<ConcreteIndexedStream<id, StreamT>> operator>>(use_nth_output_tag<id>) {
    return SC_Pipe<ConcreteIndexedStream<id, StreamT>>{scheduler, previous_output_stream->template project_to_nth_data_stream<id>(), previous_node};
  }

  SC_Pipe &operator>>(use_previous_pipe_as_sync_node_tag) {
    previous_node->add_to_scheduler_as_sync_node();
    return *this;
  }

  template<class NextPipeT>
  SC_Pipe<typename NextPipeT::OutputStreamType> operator>>(NextPipeT &next_pipe) {
    connect(previous_output_stream, next_pipe.get_input_stream());
    next_pipe.init_output_stream_if_not_inited();
    return {scheduler, &next_pipe};
  }
};

template<class PipeT>
SC_Pipe<typename PipeT::OutputStreamType> scheduler_constructor(SchedulerBase *scheduler, PipeT &pipe) {
  return {scheduler, &pipe};
}

/*** Multithreaded profiler ***/
#define TACT_SPEED (1e-6 / 2266.0)

class ProfilerRaw {
private:
  long long count;
  long long ticks;
  size_t memory;
  int flag;
public:
  void alloc_memory(size_t size) {
    count++;
    memory += size;
  }

  size_t get_memory() {
    return memory;
  }

  void start() {
    if (flag == 0) {
      ticks -= cycleclock_now();
      count++;
    }
    flag++;
  }

  void finish() {
    //assert (flag == 1);
    flag--;
    if (flag == 0) {
      ticks += cycleclock_now();
    }
  }

  long long get_ticks() {
    return ticks;
  }

  long long get_count() {
    return count;
  }

  double get_time() {
    return get_ticks() * TACT_SPEED;
  }
};


#define PROF_E_(x) prof_ ## x
#define PROF_E(x) PROF_E_(x)
#define FOREACH_PROF(F)\
  F (A)\
  F (B)\
  F (C)\
  F (D)\
  F (E)\
  F (next_name)\
  F (next_const_string_name)\
  F (create_function)\
  F (load_files)\
  F (lexer)\
  F (gentree)\
  F (split_switch)\
  F (create_switch_vars)\
  F (collect_required)\
  F (calc_locations)\
  F (preprocess_defines)\
  F (preprocess_defines_finish)\
  F (collect_defines)\
  F (check_returns)\
  F (register_defines)\
  F (preprocess_eq3)\
  F (preprocess_function_c)\
  F (preprocess_break)\
  F (register_variables)\
  F (check_instance_props)\
  F (calc_const_type)\
  F (collect_const_vars)\
  F (calc_actual_calls_edges)\
  F (calc_actual_calls)\
  F (calc_throws)\
  F (check_function_calls)\
  F (calc_rl)\
  F (CFG)\
  F (type_infence)\
  F (tinf_infer)\
  F (tinf_infer_gen_dep)\
  F (tinf_infer_infer)\
  F (tinf_find_isset)\
  F (tinf_check)\
  F (CFG_End)\
  F (optimization)\
  F (calc_val_ref)\
  F (calc_func_dep)\
  F (calc_bad_vars)\
  F (check_ub)\
  F (extract_resumable_calls)\
  F (extract_async)\
  F (check_access_modifiers)\
  F (final_check)\
  F (code_gen)\
  F (writer)\
  F (end_write)\
  F (make)\
  F (sync_with_dir)\
  \
  F (vertex_inner)\
  F (vertex_inner_data)\
  F (total)

#define DECLARE_PROF_E(x) PROF_E(x),
enum ProfilerId {
  FOREACH_PROF (DECLARE_PROF_E)
  ProfilerId_size
};

class Profiler {
public:
  ProfilerRaw raw[ProfilerId_size];
  char dummy[4096];
};

extern TLS<Profiler> profiler;

inline ProfilerRaw &get_profiler(ProfilerId id) {
  return profiler->raw[id];
}

#define PROF(x) get_profiler (PROF_E (x))

inline void profiler_print(ProfilerId id, const char *desc) {
  double total_time = 0;
  long long total_ticks = 0;
  long long total_count = 0;
  size_t total_memory = 0;
  for (int i = 0; i <= MAX_THREADS_COUNT; i++) {
    total_time += profiler.get(i)->raw[id].get_time();
    total_count += profiler.get(i)->raw[id].get_count();
    total_ticks += profiler.get(i)->raw[id].get_ticks();
    total_memory += profiler.get(i)->raw[id].get_memory();
  }
  if (total_count > 0) {
    if (total_ticks > 0) {
      fprintf(
        stderr, "%40s:\t\%lf %lld %lld\n",
        desc, total_time, total_count, total_ticks / max(1ll, total_count)
      );
    }
    if (total_memory > 0) {
      fprintf(
        stderr, "%40s:\t\%.5lfMb %lld %.5lf\n",
        desc, (double)total_memory / (1 << 20), total_count, (double)total_memory / total_count
      );
    }
  }
}

inline void profiler_print_all() {
  #define PRINT_PROF(x) profiler_print (PROF_E(x), #x);
  FOREACH_PROF (PRINT_PROF);
}

template<ProfilerId Id>
class AutoProfiler {
private:
  ProfilerRaw &prof;
public:
  AutoProfiler() :
    prof(get_profiler(Id)) {
    prof.start();
  }

  ~AutoProfiler() {
    prof.finish();
  }
};

#define AUTO_PROF(x) AutoProfiler <PROF_E (x)> x ## _auto_prof

/*** Multithreaded version of IdGen ***/
class BikeIdGen {
private:
  int used_n;

  struct IdRange {
    int l = 0;
    int r = 0;
  };
  TLS<IdRange> range;
public:

  BikeIdGen() :
    used_n(0) {}

  int next_id() {
    IdRange &cur = *range;
    if (unlikely (cur.l == cur.r)) {
      int old_used_n;
      while (true) {
        old_used_n = used_n;
        if (__sync_bool_compare_and_swap(&used_n, old_used_n, old_used_n + 4096)) {
          break;
        }
        usleep(250);
      }
      cur.l = old_used_n;
      cur.r = old_used_n + 4096;
    }
    int index = cur.l++;
    return index;
  }
};

/*** Multithreaded hash table ***/
//long long -> T
//Too much memory, not resizable, do not support collisions. Yep.
static const int N = 1000000;

template<class T>
class HT {
public:
  struct HTNode : Lockable {
    unsigned long long hash;
    T data;

    HTNode() :
      hash(0),
      data() {
    }
  };

private:
  BikeIdGen id_gen;
  HTNode *nodes;
  int nodes_size;
public:
  HT() :
    nodes(new HTNode[N]),
    nodes_size(N) {
  }

  HTNode *at(unsigned long long hash) {
    int i = (unsigned)hash % (unsigned)nodes_size;
    while (true) {
      while (nodes[i].hash != 0 && nodes[i].hash != hash) {
        i++;
        if (i == nodes_size) {
          i = 0;
        }
      }
      if (nodes[i].hash == 0 && !__sync_bool_compare_and_swap(&nodes[i].hash, 0, hash)) {
        int id = id_gen.next_id();
        assert (id * 2 < N);
        continue;
      }
      break;
    }
    return &nodes[i];
  }

  vector<T> get_all() {
    vector<T> res;
    for (int i = 0; i < N; i++) {
      if (nodes[i].hash != 0) {
        res.push_back(nodes[i].data);
      }
    }
    return res;
  }
};


#define BICYCLE_DL_PSTR
#define dl_pstr bicycle_dl_pstr
char *bicycle_dl_pstr(char const *message, ...) __attribute__ ((format (printf, 1, 2)));

