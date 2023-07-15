// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <vector>

#include "common/mixin/not_copyable.h"

#include "compiler/index.h"

class CompilerSettings;

class Target : private vk::not_copyable {
  friend class MakeRunner;

private:
  std::vector<Target *> rdeps;

  long long mtime = 0;
  int pending_deps = 0;
  bool is_required = false;
  bool is_waiting = false;
  bool is_ready = false;
  File *file = nullptr;

protected:
  bool upd_mtime(long long new_mtime);
  void set_mtime(long long new_mtime);

  std::vector<Target *> deps;
  const CompilerSettings *settings{nullptr};
public:
  long long priority;
  double start_time;
  Target() = default;
  virtual ~Target() = default;

  virtual void compute_priority();
  virtual std::string get_cmd() = 0;
  std::string get_name();

  void on_require();
  virtual bool after_run_success();
  virtual void after_run_fail();

  void force_changed(long long new_mtime);
  bool require();
  std::string output();
  std::string dep_list();

  void set_file(File *new_file);
  File *get_file() const;
  void set_settings(const CompilerSettings *new_settings);
};
