#pragma once

#include "compiler/enviroment.h"
#include "compiler/index.h"

class Target {
  friend class Make;

private:
  vector<Target *> deps;
  vector<Target *> rdeps;

  long long mtime;
  int pending_deps;
  bool is_required;
  bool is_waiting;
  bool is_ready;

  DISALLOW_COPY_AND_ASSIGN (Target);
protected:
  bool upd_mtime(long long new_mtime) __attribute__ ((warn_unused_result));
  void set_mtime(long long new_mtime);
public:
  long long priority;
  double start_time;
  Target();
  virtual ~Target();

  virtual void compute_priority();
  virtual string get_cmd() = 0;
  virtual string get_name() = 0;

  virtual void on_require() {} //can't fail
  virtual bool after_run_success() __attribute__ ((warn_unused_result));
  virtual void after_run_fail() = 0;

  void force_changed(long long new_mtime);
  bool require();
  string target();
  string dep_list();
};

class compare_by_priority {
public:
  bool operator()(Target *a, Target *b) const {
    return a->priority < b->priority;
  }
};

class Make {
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

  DISALLOW_COPY_AND_ASSIGN (Make);
public:
  void register_target(Target *target, const vector<Target *> &deps = vector<Target *>());
  bool make_target(Target *target, int jobs_count = 32);
  Make();
  ~Make();
};

class KphpMakeEnv {
  friend class KphpMake;

private:
  string cxx;
  string cxx_flags;
  string ld;
  string ld_flags;
  string debug_level;
public:
  const string &get_cxx() const;
  const string &get_cxx_flags() const;
  const string &get_ld() const;
  const string &get_ld_flags() const;
  const string &get_debug_level() const;
};

class KphpTarget : public Target {
private:
  File *file;
protected:
  const KphpMakeEnv *env;
public:
  KphpTarget();
  virtual string get_name();
  virtual void on_require();
  virtual bool after_run_success();
  virtual void after_run_fail();
  void set_file(File *new_file);
  File *get_file() const;
  void set_env(KphpMakeEnv *new_env);
};

class FileTarget : public KphpTarget {
public:
  string get_cmd();
};

class Cpp2ObjTarget : public KphpTarget {
public:
  string get_cmd();
  void compute_priority();
};

class Objs2ObjTarget : public KphpTarget {
public:
  string get_cmd();
};

class Objs2BinTarget : public KphpTarget {
public:
  string get_cmd();
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
  void init_env(const KphpEnviroment &kphp_env);
  bool make_target(File *bin, int jobs_count = 32);
};
