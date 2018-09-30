#include "compiler/scheduler/scheduler.h"

#include "compiler/scheduler/task.h"

class ThreadContext {
public:
  pthread_t pthread_id;
  int thread_id;

  class Scheduler *scheduler;

  Node *node;
  bool run_flag;
  double started;
  double finished;
};


void *scheduler_thread_execute(void *arg) {
  auto tls = (ThreadContext *)arg;
  tls->scheduler->thread_execute(tls);
  return nullptr;
}

Scheduler::Scheduler() :
  threads_count(-1) {
  task_pull = new TaskPull();
}

void Scheduler::add_node(Node *node) {
  if (node->is_parallel()) {
    nodes.push_back(node);
  } else {
    one_thread_nodes.push_back(node);
  }
}

void Scheduler::add_sync_node(Node *node) {
  sync_nodes.push(node);
}

void Scheduler::add_task(Task *task) {
  assert (task_pull != nullptr);
  task_pull->add_task(task);
}

void Scheduler::execute() {
  task_pull->add_to_scheduler(this);
  set_thread_id(0);
  vector<ThreadContext> threads(threads_count + 1);

  assert ((int)one_thread_nodes.size() < threads_count);
  for (int i = 1; i <= threads_count; i++) {
    threads[i].thread_id = i;
    threads[i].scheduler = this;
    threads[i].run_flag = true;
    if (i <= (int)one_thread_nodes.size()) {
      threads[i].node = one_thread_nodes[i - 1];
    }
    pthread_create(&threads[i].pthread_id, nullptr, ::scheduler_thread_execute, &threads[i]);
  }

  while (true) {
    if (bicycle_counter > 0) {
      usleep(250);
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
    pthread_join(threads[i].pthread_id, nullptr);
  }

  for (auto node : nodes) {
    delete node;
  }
}

void Scheduler::set_threads_count(int new_threads_count) {
  assert (1 <= new_threads_count && new_threads_count <= MAX_THREADS_COUNT);
  threads_count = new_threads_count;
}

bool Scheduler::thread_process_node(Node *node) {
  Task *task = node->get_task();
  if (task == nullptr) {
    return false;
  }
  task->execute();
  delete task;
  atomic_int_dec(&bicycle_counter);
  return true;
}

void Scheduler::thread_execute(ThreadContext *tls) {
  tls->started = dl_time();
  set_thread_id(tls->thread_id);

  while (tls->run_flag) {
    if (tls->node != nullptr) {
      while (thread_process_node(tls->node)) {
      }
    } else {
      for (int i = 0; i < (int)nodes.size(); i++) {
        Node *node = nodes[i];
        while (thread_process_node(node)) {
        }
      }
    }
    usleep(250);
  }

  tls->finished = dl_time();
  //fprintf (stderr, "%lf\n", tls->worked / (tls->finished - tls->started));
}
