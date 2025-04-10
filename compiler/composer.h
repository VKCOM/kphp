// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "common/mixin/not_copyable.h"
#include "common/wrappers/string_view.h"

// TODO: reminder: use string_view here instead of const string refs

namespace YAML {
class Node;
} // namespace YAML

class ComposerAutoloader : private vk::not_copyable {
public:
  // whether to load dev autoloading settings;
  // inverse of the `composer --no-dev` option
  void set_use_dev(bool v);

  // load_file parses the "$pkg_root/composer.json" file and saves
  // all relevant definitions inside loader;
  // this method is not thread-safe and should be called only during the compiler init
  void load_file(const std::string& pkg_root);

  // load_root_file is like load_file, but should be only used
  // for the main composer file;
  // this method is not thread-safe and should be called only during the compiler init
  void load_root_file(const std::string& pkg_root);

  // psr4_lookup/psr0_lookup tries to find a filename that should contain
  // a class class_name; the lookup is based on the loaded
  // composer files and their autoload definitions
  std::string psr4_lookup(const std::string& class_name) const;
  std::string psr0_lookup(const std::string& class_name) const;

  // is_autoload_file reports whether the specified absolute filename
  // is a composer-generated autoload.php file
  bool is_autoload_file(const std::string& filename) const noexcept {
    return filename == autoload_filename_;
  }

  const std::vector<std::string>& get_files_to_require() const noexcept {
    return files_to_require_;
  }

  using PsrMap = std::map<std::string, std::vector<std::string>, std::less<>>;

private:
  void load_file(const std::string& filename, bool root);

  static std::string psr_lookup_nocache(const PsrMap& psr, const std::string& class_name, bool transform_underscore = false);

  bool use_dev_;
  PsrMap autoload_psr4_;
  PsrMap autoload_psr0_;
  std::map<std::string, std::string> autoload_psr0_classmap_;
  std::unordered_set<std::string> deps_;

  std::string autoload_filename_;
  std::vector<std::string> files_to_require_;

  mutable std::mutex mutex_;
  mutable std::unordered_map<std::string, std::string> psr4_cache_;
};
