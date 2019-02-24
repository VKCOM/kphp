#pragma once

#include <string>

struct KphpMakeEnv {
  std::string cxx;
  std::string cxx_flags;
  std::string ld;
  std::string ld_flags;
  std::string ar;
  std::string debug_level;

  void add_gch_dir(const std::string &gch_dir) {
    cxx_flags.insert(0, "-iquote" + gch_dir + " ");
  }
};
