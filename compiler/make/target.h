#pragma once

#include <string>
#include <vector>

#include "common/mixin/not_copyable.h"

#include "compiler/index.h"
#include "compiler/make/make-env.h"

class Target : private vk::not_copyable {
  friend class Make;

private:
  std::vector<Target *> deps;
  std::vector<Target *> rdeps;

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
  virtual std::string get_cmd() = 0;
  virtual std::string get_name() = 0;

  virtual void on_require() {} //can't fail
  virtual bool after_run_success() __attribute__ ((warn_unused_result));
  virtual void after_run_fail() = 0;

  void force_changed(long long new_mtime);
  bool require();
  std::string target();
  std::string dep_list();
};

// TODO: merge this two classes to one
class KphpTarget : public Target {
private:
  File *file;
protected:
  const KphpMakeEnv *env;
public:
  KphpTarget();
  virtual std::string get_name() final;
  virtual void on_require() final;
  virtual bool after_run_success() final;
  virtual void after_run_fail() final;
  void set_file(File *new_file);
  File *get_file() const;
  void set_env(KphpMakeEnv *new_env);
};
