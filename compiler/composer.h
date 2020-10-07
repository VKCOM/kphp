#pragma once

#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <mutex>

#include "common/mixin/not_copyable.h"
#include "common/wrappers/string_view.h"

class ComposerClassLoader : private vk::not_copyable {
public:
  void set_use_dev(bool v);

  void load_file(const std::string& filename);
  void load_root_file(const std::string& filename);

  std::string psr4_lookup(const std::string &class_name) const;

private:
  void load_file(const std::string& filename, bool root);
  std::string psr4_lookup_nocache(const std::string& class_name) const;

  bool use_dev_;
  std::map<std::string, std::vector<std::string>, std::less<>> autoload_psr4_;

  mutable std::mutex mutex_;
  mutable std::unordered_map<std::string, std::string> psr4_cache_;
};
