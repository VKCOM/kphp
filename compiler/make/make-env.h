#pragma once

#include <string>

class KphpMakeEnv {
  friend class KphpMake;

private:
  std::string cxx;
  std::string cxx_flags;
  std::string ld;
  std::string ld_flags;
  std::string ar;
  std::string debug_level;

public:
  const std::string &get_cxx() const;
  const std::string &get_cxx_flags() const;
  const std::string &get_ld() const;
  const std::string &get_ld_flags() const;
  const std::string &get_ar() const;
  const std::string &get_debug_level() const;

  void add_gch_dir(const std::string &gch_dir);
};
