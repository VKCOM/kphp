// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/composer.h"

#include <yaml-cpp/yaml.h> // using YAML parser to handle JSON files

#include <dirent.h>
#include <sys/stat.h> // TODO: remove when std::filesystem is used everywhere instead of stat
#include <filesystem>

#include "common/smart_ptrs/unique_ptr_with_delete_function.h"
#include "common/algorithms/contains.h"
#include "common/wrappers/fmt_format.h"
#include "compiler/kphp_assert.h"
#include "compiler/stage.h"

namespace {

void close_dir(DIR *d) {
  closedir(d);
}

void collect_composer_folders(const std::string &path, std::vector<std::string> &result) {
  // can't use nftw here as it doesn't provide a portable way to stop directory traversal;
  // we don't want to visit *all* files in the vendor tree
  //
  // suppose we have this composer-generated layout:
  //     vendor/pkg1/
  //     * composer.json
  //     * src/
  //     vendor/ns/pkg2/
  //     * composer.json
  //     * src/
  // all pkg directories can have a lot of files inside src/,
  // if we can stop as soon as we find composer.json, a lot of
  // redundant work is avoided
  //
  // TODO: rewrite using C++17 filesystem?

  vk::unique_ptr_with_delete_function<DIR, close_dir> dp{opendir(path.c_str())};
  if (dp == nullptr) {
    kphp_warning(fmt_format("find composer files: opendir({}) failed: {}", path.c_str(), strerror(errno)));
    return;
  }

  // since composer package can't have nested composer.json file, we stop
  // directory traversal if we found it; otherwise we descend further;
  // dirs contains all directories that we need to visit when descending
  bool recurse = true;
  std::vector<std::string> dirs;

  while (const auto *entry = readdir(dp.get())) {
    if (entry->d_name[0] == '.') {
      continue;
    }

    if (std::strcmp(entry->d_name, "composer.json") == 0) {
      result.push_back(path);
      recurse = false;
      break;
    }

    // by default, composer does no copy for packages; it creates a symlink instead
    if (entry->d_type == DT_LNK) {
      // collect only those links that point to a directory
      auto link_path = path + "/" + entry->d_name + "/";
      struct stat link_info;
      stat(link_path.c_str(), &link_info);
      if (S_ISDIR(link_info.st_mode)) {
        dirs.push_back(std::move(link_path));
      }
    } else if (entry->d_type == DT_DIR) {
      dirs.emplace_back(path + "/" + entry->d_name + "/");
    }
  }

  if (recurse) {
    for (const auto &dir : dirs) {
      collect_composer_folders(dir, result);
    }
  }
}

// Collect all composer.json file roots that can be found in the given directory.
std::vector<std::string> find_composer_folders(const std::string &dir) {
  std::vector<std::string> result;
  collect_composer_folders(dir, result);
  return result;
}

} // namespace

bool ComposerAutoloader::is_classmap_file(const std::string &filename) const noexcept {
  return vk::contains(classmap_files_, filename);
}

void ComposerAutoloader::scan_classmap(const std::string &filename) {
  // supporting the real composer classmap is cumbersome: it requires full PHP parsing to
  // fetch all classes from files (the filename doesn't have to follow any conventions);
  // we could also invoke php interpreter over vendor/composer/autoload_classmap.php to
  // print a JSON dump of the generated classmap and then decode that, but then
  // it will be impossible to compile a kphp program that uses a classmap without php interpreter;
  // as an alternative, we add all classmap files to auto-required lists that will be
  // included along "autoload.files" files, if some classes are not needed, they will be
  // discarded after we compute actually used symbols
  //
  // this approach works well as long as there is no significant side effects related to
  // the files being autoloaded (otherwise those side effects will trigger at different point in time)

  const auto add_classmap_file = [&](const std::string &filename) {
    classmap_files_.insert(filename);
    files_to_require_.emplace_back(filename);
  };

  auto file_info = std::filesystem::status(filename);
  kphp_error(file_info.type() != std::filesystem::file_type::not_found,
             fmt_format("can't find {} classmap file", filename));
  if (file_info.type() == std::filesystem::file_type::directory) {
    for (const auto &entry : std::filesystem::directory_iterator(filename)) {
      scan_classmap(entry.path().string());
    }
  } else if (file_info.type() == std::filesystem::file_type::regular) {
    if (vk::string_view(filename).ends_with(".php") || vk::string_view(filename).ends_with(".inc")) {
      add_classmap_file(filename);
    }
  }
}

std::string ComposerAutoloader::psr4_lookup_nocache(const std::string &class_name) const {
  std::string prefix = class_name;

  auto file_exists = [](const std::string &filename) { return access(filename.c_str(), F_OK) == 0; };

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

  std::string psr4_filename = psr4_lookup_nocache(class_name);
  {
    // the "" result is cached as well
    std::lock_guard<std::mutex> lock{mutex_};
    psr4_cache_[class_name] = psr4_filename;
  }

  return psr4_filename;
}

void ComposerAutoloader::set_use_dev(bool v) {
  use_dev_ = v;
}

void ComposerAutoloader::load(const std::string &pkg_root) {
  load_root_file(pkg_root);

  // We could traverse the composer file and collect all "repositories"
  // and map them with "requirements" to get the dependency list,
  // but some projects may use composer plugins that change composer
  // files before "composer install" is invoked, so the final vendor
  // folder may be generated from files that differ from the composer
  // files that we can reach. To avoid that problem, we scan the vendor
  // folder in order to collect all dependencies (both direct and indirect).

  std::string vendor = pkg_root + "vendor";
  bool vendor_folder_exists = access(vendor.c_str(), F_OK) == 0;
  if (vendor_folder_exists) {
    for (const auto &composer_root : find_composer_folders(vendor)) {
      load_file(composer_root);
    }
  }
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
  //     "classmap": [
  //       "src/",
  //       "lib/file.php",
  //       <...>
  //     ],
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
  //     "files": [
  //       "file.php",
  //       <...>
  //     ],
  //   }
  // }

  auto filename = pkg_root + "composer.json";

  auto add_autoload_psr4_dir = [&](const std::string &prefix, std::string dir) {
    if (dir.empty()) {
      dir = "./"; // composer interprets "" as "./" or "."
    }
    std::replace(dir.begin(), dir.end(), '\\', '/');
    // ensure that dir always ends with '/'
    if (dir.back() != '/') {
      dir.push_back('/');
    }

    autoload_psr4_[prefix].emplace_back(pkg_root + dir);
  };

  auto add_autoload_section = [&](YAML::Node autoload, bool require_files) {
    // https://getcomposer.org/doc/04-schema.md#psr-4
    const auto &psr4_src = autoload["psr-4"];
    for (const auto &kv : psr4_src) {
      std::string prefix = kv.first.as<std::string>();
      std::replace(prefix.begin(), prefix.end(), '\\', '/');

      if (kv.second.IsSequence()) {
        for (const auto &dir : kv.second) {
          add_autoload_psr4_dir(prefix, dir.as<std::string>());
        }
      } else if (kv.second.IsScalar()) {
        add_autoload_psr4_dir(prefix, kv.second.as<std::string>());
      } else {
        kphp_error(false, fmt_format("load composer file {}: invalid autoload psr-4 item", filename.c_str()));
      }
    }

    // https://getcomposer.org/doc/04-schema.md#classmap
    const auto &classmap_src = autoload["classmap"];
    for (const auto &elem : classmap_src) {
      scan_classmap(pkg_root + elem.as<std::string>());
    }

    if (require_files) {
      // files that are required by the composer-generated autoload.php
      // https://getcomposer.org/doc/04-schema.md#files
      for (const auto &autoload_filename : autoload["files"]) {
        files_to_require_.emplace_back(pkg_root + autoload_filename.as<std::string>());
      }
    }
  };

  try {
    YAML::Node root = YAML::LoadFile(filename);
    auto name = root["name"];
    bool require_autoload_files = is_root_file || (name && vk::contains(deps_, name.as<std::string>()));
    if (auto autoload = root["autoload"]) {
      add_autoload_section(autoload, require_autoload_files);
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
        add_autoload_section(autoload_dev, require_autoload_files);
      }
    }
  } catch (const std::exception &e) {
    kphp_error(false, fmt_format("load composer file {}: {}", filename.c_str(), e.what()));
  }
}
