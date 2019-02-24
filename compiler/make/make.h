#pragma once

#include "common/mixin/not_copyable.h"

#include "compiler/enviroment.h"
#include "compiler/index.h"
#include "compiler/make/file-target.h"

class Make : private vk::not_copyable {
  class compare_by_priority {
  public:
    bool operator()(Target *a, Target *b) const {
      return a->priority < b->priority;
    }
  };
private:
  int targets_waiting;
  int targets_left;
  vector<Target *> all_targets;

  std::priority_queue<Target *, vector<Target *>, compare_by_priority> pending_jobs;
  map<int, Target *> jobs;

  bool fail_flag;
  static int signal_flag;
  static void sigint_handler(int sig);

  bool start_job(Target *target) __attribute__ ((warn_unused_result));
  bool finish_job(int pid, int return_code, int by_signal) __attribute__ ((warn_unused_result));
  void on_fail();

  void run_target(Target *target);
  void ready_target(Target *target);
  void one_dep_ready_target(Target *target);
  void wait_target(Target *target);
  void require_target(Target *target);

public:
  void register_target(Target *target, const vector<Target *> &deps = vector<Target *>());
  bool make_target(Target *target, int jobs_count = 32);
  Make();
  ~Make();
};


class KphpMake {
private:
  Make make;
  KphpMakeEnv env;
  void target_set_file(KphpTarget *target, File *file);
  void target_set_env(KphpTarget *target);
  KphpTarget *to_target(File *file);
  vector<Target *> to_targets(File *file);
  vector<Target *> to_targets(vector<File *> files);
public:
  KphpTarget *create_cpp_target(File *cpp);
  KphpTarget *create_cpp2obj_target(File *cpp, File *obj);
  KphpTarget *create_objs2obj_target(const vector<File *> &objs, File *obj);
  KphpTarget *create_objs2bin_target(const vector<File *> &objs, File *bin);
  KphpTarget *create_objs2static_lib_target(const vector<File *> &objs, File *lib);
  void init_env(const KphpEnviroment &kphp_env);
  void add_gch_dir(const std::string &gch_dir) { env.add_gch_dir(gch_dir); }
  bool make_target(File *bin, int jobs_count = 32);
};

void run_make();
