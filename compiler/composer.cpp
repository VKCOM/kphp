// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/composer.h"

#include <yaml-cpp/yaml.h> // using YAML parser to handle JSON files

#include "compiler/kphp_assert.h"
#include "common/wrappers/fmt_format.h"
#include "compiler/stage.h"

std::string ComposerClassLoader::psr4_lookup_nocache(const std::string &class_name) const {
  std::string prefix = class_name;

  auto file_exists = [](const std::string &filename) {
    return access(filename.c_str(), F_OK) == 0;
  };

  // we start from a longest prefix and then try to match it
  // against the psr4 map; if there is no match, the last prefix
  // part is dropped and the process is repeated until we
  // don't have any parts left
  while (true) {
    auto slash_pos = prefix.find_last_of('/');
    if (slash_pos == std::string::npos) {
      break;
    }
    prefix = prefix.substr(0, slash_pos);
    std::string key = prefix + "/";
    auto candidates = autoload_psr4_.find(key);
    if (candidates == autoload_psr4_.end()) {
      continue;
    }
    // for a single prefix like "VK\Feed" there almost always more than 1
    // candidate (e.g. "VK\Feed"->"tests/" and "VK\Feed"->"src/";
    // we choose a first successful guess based on a file existence: if \VK\Feed\C
    // exists in VK\Feed\src\C.php, we return that, otherwise we try VK\Feed\tests\C.php
    for (const auto &dir : candidates->second) {
      const auto &psr4_filename = dir + class_name.substr(key.size()) + ".php";
      if (file_exists(psr4_filename)) {
        return psr4_filename;
      }
    }
  }

  // "" key contains fallback directories
  auto fallback_dirs = autoload_psr4_.find("");
  if (fallback_dirs != autoload_psr4_.end()) {
    for (const auto &dir : fallback_dirs->second) {
      const auto &psr4_filename = dir + class_name + ".php";
      if (file_exists(psr4_filename)) {
        return psr4_filename;
      }
    }
  }

  return "";
}

std::string ComposerClassLoader::psr4_lookup(const std::string &class_name) const {
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

  std::string psr4_filename = psr4_lookup_nocache(class_name);
  {
    // the "" result is cached as well
    std::lock_guard<std::mutex> lock{mutex_};
    psr4_cache_[class_name] = psr4_filename;
  }

  return psr4_filename;
}

void ComposerClassLoader::set_use_dev(bool v) {
  use_dev_ = v;
}

void ComposerClassLoader::load_root_file(const std::string &pkg_root) {
  kphp_assert(!pkg_root.empty() && pkg_root.back() == '/');
  kphp_assert(autoload_filename_.empty());
  autoload_filename_ = pkg_root + "vendor/autoload.php";
  load_file(pkg_root, true);
}

void ComposerClassLoader::load_file(const std::string &pkg_root) {
  load_file(pkg_root, false);
}

void ComposerClassLoader::load_file(const std::string &pkg_root, bool is_root_file) {
  // composer.json structure (that we care about):
  // {
  //   "autoload": {
  //     "psr-4": {
  //       "prefix1": "dir",
  //       "prefix2": ["dir/", ...],
  //       "": "fallback-dir/",
  //       <...>
  //     }
  //   }
  //   "autoload-dev": {
  //     "psr-4": {
  //       "prefix": "dir",
  //       <...>
  //     }
  //   }
  // }

  auto filename = pkg_root + "/composer.json";

  auto add_autoload_dir = [&](const std::string &prefix, std::string dir) {
    if (dir.empty()) {
      dir = "./"; // composer interprets "" as "./" or "."
    }
    std::replace(dir.begin(), dir.end(), '\\', '/');
    // ensure that dir always ends with '/'
    if (dir.back() != '/') {
      dir.push_back('/');
    }

    autoload_psr4_[prefix].emplace_back(pkg_root + "/" + dir);
  };

  auto add_autoload_section = [&](YAML::Node autoload) {
    // https://getcomposer.org/doc/04-schema.md#psr-4
    const auto psr4_src = autoload["psr-4"];
    for (const auto &kv : psr4_src) {
      std::string prefix = kv.first.as<std::string>();
      std::replace(prefix.begin(), prefix.end(), '\\', '/');

      if (kv.second.IsSequence()) {
        for (const auto &dir : kv.second) {
          add_autoload_dir(prefix, dir.as<std::string>());
        }
      } else if (kv.second.IsScalar()) {
        add_autoload_dir(prefix, kv.second.as<std::string>());
      } else {
        kphp_error(false, fmt_format("load composer file {}: invalid autoload psr-4 item", filename.c_str()));
      }
    }

    // files that are required by the composer-generated autoload.php
    // https://getcomposer.org/doc/04-schema.md#files
    for (const auto &autoload_filename : autoload["files"]) {
      files_to_require_.emplace_back(pkg_root + "/" + autoload_filename.as<std::string>());
    }
  };

  try {
    YAML::Node root = YAML::LoadFile(filename);
    if (auto autoload = root["autoload"]) {
      add_autoload_section(autoload);
    }
    if (is_root_file && use_dev_) {
      if (auto autoload_dev = root["autoload-dev"]) {
        add_autoload_section(autoload_dev);
      }
    }
  } catch (const std::exception &e) {
    kphp_error(false, fmt_format("load composer file {}: {}", filename.c_str(), e.what()));
  }
}
