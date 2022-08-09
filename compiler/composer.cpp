// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/composer.h"

#include <yaml-cpp/yaml.h> // using YAML parser to handle JSON files

#include "common/algorithms/contains.h"
#include "common/wrappers/fmt_format.h"
#include "compiler/kphp_assert.h"
#include "compiler/stage.h"

std::string ComposerAutoloader::psr_lookup_nocache(const PsrMap &psr, const std::string &class_name, bool transform_underscore) {
  std::string prefix = class_name;

  auto file_exists = [](const std::string &filename) { return access(filename.c_str(), F_OK) == 0; };

  // we start from a longest prefix and then try to match it
  // against the psr4/psr0 map; if there is no match, the last prefix
  // part is dropped and the process is repeated until we
  // don't have any parts left
  while (true) {
    auto slash_pos = prefix.find_last_of('/');
    if (slash_pos == std::string::npos) {
      break;
    }
    prefix = prefix.substr(0, slash_pos);
    std::string key = prefix + "/";
    auto candidates = psr.find(key);
    if (candidates == psr.end()) {
      continue;
    }
    // for a single prefix like "VK\Feed" there almost always more than 1
    // candidate (e.g. "VK\Feed"->"tests/" and "VK\Feed"->"src/";
    // we choose a first successful guess based on a file existence: if \VK\Feed\C
    // exists in VK\Feed\src\C.php, we return that, otherwise we try VK\Feed\tests\C.php
    for (const auto &dir : candidates->second) {
      auto short_class_name = class_name.substr(key.size());
      if (transform_underscore) {
        std::replace(short_class_name.begin(), short_class_name.end(), '_', '/');
      }
      const auto &psr4_filename = dir + short_class_name + ".php";
      if (file_exists(psr4_filename)) {
        return psr4_filename;
      }
    }
  }

  // "" key contains fallback directories
  auto fallback_dirs = psr.find("");
  if (fallback_dirs != psr.end()) {
    for (const auto &dir : fallback_dirs->second) {
      auto class_name_copy = class_name;
      auto last_slash = class_name_copy.find_last_of('/');
      if (transform_underscore && last_slash != std::string::npos) {
        std::replace(std::next(class_name_copy.begin(), last_slash), class_name_copy.end(), '_', '/');
      }
      const auto &psr4_filename = dir + class_name_copy + ".php";
      if (file_exists(psr4_filename)) {
        return psr4_filename;
      }
    }
  }

  return "";
}

std::string ComposerAutoloader::psr4_lookup(const std::string &class_name) const {
  // we could add a special path for packages with a single candidate, but
  // they seem to be almost non-existent; a single candidate package can
  // be handled without caching/touching FS/locking

  {
    std::lock_guard<std::mutex> lock{mutex_};
    auto it = psr4_cache_.find(class_name);
    if (it != psr4_cache_.end()) {
      return it->second;
    }
  }

  std::string psr4_filename = psr_lookup_nocache(autoload_psr4_, class_name);
  {
    // the "" result is cached as well
    std::lock_guard<std::mutex> lock{mutex_};
    psr4_cache_[class_name] = psr4_filename;
  }
  return psr4_filename;
}

std::string ComposerAutoloader::psr0_lookup(const std::string &class_name) const {
  return psr_lookup_nocache(autoload_psr0_, class_name, true);
}

void ComposerAutoloader::set_use_dev(bool v) {
  use_dev_ = v;
}

void ComposerAutoloader::load_root_file(const std::string &pkg_root) {
  kphp_assert(!pkg_root.empty() && pkg_root.back() == '/');
  kphp_assert(autoload_filename_.empty());
  autoload_filename_ = pkg_root + "vendor/autoload.php";
  load_file(pkg_root, true);
}

void ComposerAutoloader::load_file(const std::string &pkg_root) {
  load_file(pkg_root, false);
}

void ComposerAutoloader::load_file(const std::string &pkg_root, bool is_root_file) {
  // composer.json structure (that we care about):
  // {
  //   // name -- used to identify this composer package
  //   // deps_[name] tells whether we need to include autoload files
  //   "name": "pkgvendor/pkgname",
  //   // require -- fills deps_ if it's a root file
  //   "require": {
  //     "php": ">=7.4", // ignored
  //     "vkcom/kphp-polyfills": "^1.0",
  //     <...>
  //   },
  //   // require-dev -- fills deps_ if it's a root file and use_dev_ is true
  //   "require-dev": {
  //     "phpunit/phpunit": "9.5.16",
  //     <...>
  //   },
  //   "autoload": {
  //     "psr-4": {
  //       "prefix1": "dir",
  //       "prefix2": ["dir/", ...],
  //       "": "fallback-dir/",
  //       <...>
  //     },
  //     "psr-0": {
  //       "prefix1": "dir",
  //       "prefix2": ["dir/", ...],
  //       "": "fallback-dir/",
  //       <...>
  //     },
  //     "files": [
  //       "file.php",
  //       <...>
  //     ],
  //   }
  //   "autoload-dev": {
  //     "psr-4": {
  //       "prefix": "dir",
  //       <...>
  //     },
  //     "psr-0": {
  //       "prefix": "dir",
  //       <...>
  //     },
  //     "files": [
  //       "file.php",
  //       <...>
  //     ],
  //   }
  // }

  auto filename = pkg_root + "/composer.json";

  try {
    YAML::Node root = YAML::LoadFile(filename);
    auto name = root["name"];
    bool require_autoload_files = is_root_file || (name && vk::contains(deps_, name.as<std::string>()));
    if (auto autoload = root["autoload"]) {
      add_autoload_section(autoload, pkg_root, require_autoload_files);
    }
    if (is_root_file) {
      if (auto require = root["require"]) {
        for (const auto &kv : require) {
          deps_.insert(kv.first.as<std::string>());
        }
      }
      auto require_dev = root["require-dev"];
      if (require_dev && use_dev_) {
        for (const auto &kv : require_dev) {
          deps_.insert(kv.first.as<std::string>());
        }
      }
    }
    if (is_root_file && use_dev_) {
      if (auto autoload_dev = root["autoload-dev"]) {
        add_autoload_section(autoload_dev, pkg_root, require_autoload_files);
      }
    }
  } catch (const std::exception &e) {
    kphp_error(false, fmt_format("load composer file {}: {}", filename.c_str(), e.what()));
  }
}

void ComposerAutoloader::add_autoload_section(const YAML::Node &autoload, const std::string &pkg_root, bool require_files) {
  // https://getcomposer.org/doc/04-schema.md#psr-4
  add_autoload_psr_section(autoload["psr-4"], autoload_psr4_, pkg_root, false);

  // https://getcomposer.org/doc/04-schema.md#psr-0
  add_autoload_psr_section(autoload["psr-0"], autoload_psr0_, pkg_root, true);

  if (require_files) {
    // files that are required by the composer-generated autoload.php
    // https://getcomposer.org/doc/04-schema.md#files
    for (const auto &autoload_filename : autoload["files"]) {
      files_to_require_.emplace_back(pkg_root + "/" + autoload_filename.as<std::string>());
    }
  }
}

static std::string make_file_path(const std::string &pkg_root, std::string dir, const std::string &prefix, bool add_prefix) {
  if (add_prefix) {
    dir += '/';
    dir += prefix;
  }
  if (dir.empty()) {
    dir = "./"; // composer interprets "" as "./" or "."
  }
  std::replace(dir.begin(), dir.end(), '\\', '/');
  // ensure that dir always ends with '/'
  if (dir.back() != '/') {
    dir.push_back('/');
  }

  return pkg_root + "/" + dir;
}

void ComposerAutoloader::add_autoload_psr_section(const YAML::Node &psr_src, PsrMap &psr_map, const std::string &pkg_root, bool add_prefix) {
  for (const auto &kv : psr_src) {
    auto prefix = kv.first.as<std::string>();
    std::replace(prefix.begin(), prefix.end(), '\\', '/');

    if (kv.second.IsSequence()) {
      for (const auto &dir : kv.second) {
        auto dir_str = dir.as<std::string>();
        psr_map[prefix].emplace_back(make_file_path(pkg_root, dir_str, prefix, add_prefix));
      }
    } else if (kv.second.IsScalar()) {
      auto dir_str = kv.second.as<std::string>();
      psr_map[prefix].emplace_back(make_file_path(pkg_root, dir_str, prefix, add_prefix));
    } else {
      kphp_error(false, fmt_format("load composer file {}/composer.json: invalid autoload psr item", pkg_root));
    }
  }
}
