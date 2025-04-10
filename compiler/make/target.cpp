// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/make/target.h"

#include <cassert>
#include <string>

#include "common/wrappers/fmt_format.h"

void Target::set_mtime(long long new_mtime) {
  assert(mtime == 0);
  mtime = new_mtime;
}

bool Target::upd_mtime(long long new_mtime) {
  if (new_mtime < mtime) {
    fmt_print("Trying to decrease mtime\n");
    return false;
  }
  mtime = new_mtime;
  return true;
}

void Target::compute_priority() {
  priority = 0;
}

bool Target::require() {
  if (is_required) {
    return false;
  }
  is_required = true;
  on_require();
  return true;
}

void Target::force_changed(long long new_mtime) {
  if (mtime < new_mtime) {
    mtime = new_mtime;
  }
}

std::string Target::target() {
  return get_name();
}

std::string Target::dep_list() {
  std::string ss;
  for (auto* dep : deps) {
    ss += dep->get_name();
    ss += " ";
  }
  return ss;
}

std::string Target::get_name() {
  return file->path;
}

void Target::on_require() {
  file->needed = true;
}

bool Target::after_run_success() {
  long long res = file->read_stat();
  if (res < 0) {
    return false;
  } else if (res == 0) {
    fmt_print("Failed to generate target [{}]\n", file->path);
    return false;
  }
  return upd_mtime(file->mtime);
}

void Target::after_run_fail() {
  file->unlink();
}

void Target::set_file(File* new_file) {
  assert(file == nullptr);
  file = new_file;
  set_mtime(file->mtime);
}

File* Target::get_file() const {
  return file;
}

void Target::set_settings(const CompilerSettings* new_settings) {
  assert(settings == nullptr);
  settings = new_settings;
}
