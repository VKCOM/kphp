#include "compiler/make/make-env.h"

const std::string &KphpMakeEnv::get_cxx() const {
  return cxx;
}

const std::string &KphpMakeEnv::get_cxx_flags() const {
  return cxx_flags;
}

const std::string &KphpMakeEnv::get_ld() const {
  return ld;
}

const std::string &KphpMakeEnv::get_ld_flags() const {
  return ld_flags;
}

const std::string &KphpMakeEnv::get_ar() const {
  return ar;
}

const std::string &KphpMakeEnv::get_debug_level() const {
  return debug_level;
}

void KphpMakeEnv::add_gch_dir(const std::string &gch_dir) {
  cxx_flags.insert(0, "-iquote" + gch_dir + " ");
}
