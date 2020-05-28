#pragma once

#include <string>

struct KphpMakeEnv {
  std::string cxx;
  std::string cxx_flags;
  std::string ld_flags;
  std::string ar;
  std::string incremental_linker;
  std::string incremental_linker_flags;
  std::string debug_level;

  void add_gch_dir(const std::string &gch_dir) {
    cxx_flags.insert(0, "-iquote" + gch_dir + " ");
  }
};
