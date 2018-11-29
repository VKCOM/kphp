#pragma once

#include "common/mixin/not_copyable.h"

#include "compiler/enviroment.h"
#include "compiler/index.h"

class Target : private vk::not_copyable {
  friend class Make;

private:
  vector<Target *> deps;
  vector<Target *> rdeps;

  long long mtime;
  int pending_deps;
  bool is_required;
  bool is_waiting;
  bool is_ready;
protected:
  bool upd_mtime(long long new_mtime) __attribute__ ((warn_unused_result));
  void set_mtime(long long new_mtime);
public:
  long long priority;
  double start_time;
  Target();
  virtual ~Target() = default;

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

class Make : private vk::not_copyable {
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

class KphpMakeEnv {
  friend class KphpMake;

private:
  string cxx;
  string cxx_flags;
  string ld;
  string ld_flags;
  string ar;
  string debug_level;

public:
  const string &get_cxx() const;
  const string &get_cxx_flags() const;
  const string &get_ld() const;
  const string &get_ld_flags() const;
  const string &get_ar() const;
  const string &get_debug_level() const;

  void add_gch_dir(const std::string &gch_dir);
};

class KphpTarget : public Target {
private:
  File *file;
protected:
  const KphpMakeEnv *env;
public:
  KphpTarget();
  virtual string get_name() final;
  virtual void on_require() final;
  virtual bool after_run_success() final;
  virtual void after_run_fail() final;
  void set_file(File *new_file);
  File *get_file() const;
  void set_env(KphpMakeEnv *new_env);
};

class FileTarget : public KphpTarget {
public:
  string get_cmd() final;
};

class Cpp2ObjTarget : public KphpTarget {
public:
  string get_cmd() final;
  void compute_priority();
};

class Objs2ObjTarget : public KphpTarget {
public:
  string get_cmd() final;
};

class Objs2BinTarget : public KphpTarget {
public:
  string get_cmd() final;
};

class Objs2StaticLibTarget : public KphpTarget {
public:
  string get_cmd() final;
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
