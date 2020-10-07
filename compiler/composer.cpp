#include "compiler/composer.h"

#include <yaml-cpp/yaml.h> // using YAML parser to handle JSON files

#include "compiler/kphp_assert.h"
#include "common/wrappers/fmt_format.h"
#include "compiler/stage.h"

std::string ComposerClassLoader::psr4_lookup_nocache(const std::string &class_name) const {
  std::string prefix = class_name;

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
      if (access(psr4_filename.c_str(), F_OK) == 0) {
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

void ComposerClassLoader::load_root_file(const std::string &filename) {
  load_file(filename, true);
}

void ComposerClassLoader::load_file(const std::string &filename) {
  load_file(filename, false);
}

void ComposerClassLoader::load_file(const std::string &pkg_root, bool is_root_file) {
  // composer.json structure (that we care about):
  // {
  //   "autoload": {
  //     "psr-4": {
  //       "prefix": "dir",
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

  auto add_autoload_section = [&](YAML::Node autoload) {
    const auto psr4_src = autoload["psr-4"];
    for (const auto &kv : psr4_src) {
      std::string prefix = kv.first.as<std::string>();
      std::replace(prefix.begin(), prefix.end(), '\\', '/');

      std::string dir = kv.second.as<std::string>();
      if (dir.empty()) {
        dir = "./"; // composer interprets "" as "./" or "."
      }
      std::replace(dir.begin(), dir.end(), '\\', '/');
      // ensure that dir always ends with '/'
      if (dir.back() != '/') {
        dir.push_back('/');
      }

      autoload_psr4_[prefix].emplace_back(pkg_root + "/" + dir);
    }
  };

  auto filename = pkg_root + "/composer.json";
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
