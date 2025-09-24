#pragma once

#include <dirent.h>

namespace kphp_ml {

// TODO: maybe store traverse_kml_dir(kphp_ml::stl::string) here?
struct fs_function_table {
  using stat_t = int (*)(const char*, struct stat*);
  using opendir_t = DIR* (*)(const char*);
  using closedir_t = int (*)(DIR*);
  using readdir_t = struct dirent* (*)(DIR*);
};
} // namespace kphp_ml
