// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/composer.h"

#include <yaml-cpp/yaml.h> // using YAML parser to handle JSON files

#include <filesystem>

#include "common/algorithms/contains.h"
#include "common/wrappers/fmt_format.h"
#include "compiler/kphp_assert.h"
#include "compiler/stage.h"

namespace {
bool file_exists(std::string_view filename) noexcept {
  return access(filename.data(), F_OK) == 0;
};

std::string make_file_path(std::string_view pkg_root, std::string &&dir) {
  if (dir.empty()) {
    dir = "./"; // composer interprets "" as "./" or "."
  }
  std::replace(dir.begin(), dir.end(), '\\', '/');
  // ensure that dir always ends with '/'
  if (dir.back() != '/') {
    dir.push_back('/');
  }

  return std::string{pkg_root} + "/" + std::move(dir);
}

class PsrLoader {
public:
  using Map = ComposerAutoloader::PsrMap;

  PsrLoader(const YAML::Node &psr_section, Map &map, std::string_view pkg_root) noexcept:
    psr_section_(psr_section),
    map_(map),
    pkg_root_(pkg_root) {}

  void load();

  virtual ~PsrLoader() = default;

protected:
  void load_entry_namespace(std::string &&namespace_str, std::string &&path) {
    auto full_path = make_file_path(pkg_root_, std::move(path));
    map_[std::move(namespace_str)].emplace_back(std::move(full_path));
  }

  static bool is_classname(std::string_view namespace_str) noexcept {
    return !namespace_str.empty() && namespace_str.back() != '\\';
  }

  virtual void load_entry(std::string &&namespace_str, std::string &&path) = 0;

  const YAML::Node psr_section_;
  Map &map_;
  std::string_view pkg_root_;
};

void PsrLoader::load() {
  for (const auto &kv : psr_section_) {
    if (kv.second.IsSequence()) {
      for (const auto &dir : kv.second) {
        load_entry(kv.first.as<std::string>(), dir.as<std::string>());
      }
    } else if (kv.second.IsScalar()) {
      load_entry(kv.first.as<std::string>(), kv.second.as<std::string>());
    } else {
      kphp_error(false, fmt_format("load composer file {}/composer.json: invalid autoload psr item", pkg_root_));
    }
  }
}

class Psr4Loader : public PsrLoader {
public:
  Psr4Loader(const YAML::Node &autoload_section, Map &map, std::string_view pkg_root)
    : PsrLoader(autoload_section["psr-4"], map, pkg_root) {}

protected:
  void load_entry(std::string &&namespace_str, std::string &&path) final {
    kphp_error(!is_classname(namespace_str), fmt_format(
      "load composer file {}/composer.json: namespace must ends with '\\' sign in psr-4", pkg_root_));
    std::replace(namespace_str.begin(), namespace_str.end(), '\\', '/');
    load_entry_namespace(std::move(namespace_str), std::move(path));
  }
};

class Psr0Loader : public PsrLoader {
public:
  Psr0Loader(const YAML::Node &autoload_section, std::map<std::string, std::string> &classmap, Map &map, std::string_view pkg_root)
    : PsrLoader(autoload_section["psr-0"], map, pkg_root)
    , classmap_(classmap) {}

protected:
  void load_entry(std::string &&namespace_str, std::string &&path) final {
    const bool classname = is_classname(namespace_str);
    std::replace(namespace_str.begin(), namespace_str.end(), '\\', '/');

    if (classname) {
      load_entry_class(std::move(namespace_str), std::move(path));
    } else {
      load_entry_namespace(std::move(namespace_str), std::move(path) + "/" + namespace_str);
    }
  }

private:
  void load_entry_class(std::string &&namespace_str, std::string &&path) {
    auto class_name = namespace_str;
    auto last_slash = class_name.find_last_of('/');
    auto begin = class_name.begin();
    if (last_slash != std::string::npos) {
      std::advance(begin, last_slash);
    }
    std::replace(begin, class_name.end(), '_', '/');
    std::string file{pkg_root_};
    file.append(1, '/').append(path).append(1, '/').append(class_name).append(".php");
    classmap_.emplace(std::move(namespace_str), std::move(file));
  }

  std::map<std::string, std::string> &classmap_;
};
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

std::string ComposerAutoloader::psr_lookup_nocache(const PsrMap &psr, const std::string &class_name, bool transform_underscore) {
  std::string prefix = class_name;
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
  auto file = psr_lookup_nocache(autoload_psr0_, class_name, true);
  if (file.empty()) {
    auto it = autoload_psr0_classmap_.find(class_name);
    if (it != autoload_psr0_classmap_.end() && file_exists(it->second)) {
      return it->second;
    }
  }
  return file;
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
  Psr4Loader psr4_loader{autoload, autoload_psr4_, pkg_root};
  psr4_loader.load();

  // https://getcomposer.org/doc/04-schema.md#psr-0
  Psr0Loader psr0_loader{autoload, autoload_psr0_classmap_, autoload_psr0_, pkg_root};
  psr0_loader.load();

  // https://getcomposer.org/doc/04-schema.md#classmap
  const auto &classmap_src = autoload["classmap"];
  for (const auto &elem : classmap_src) {
    scan_classmap(pkg_root + elem.as<std::string>());
  }

  if (require_files) {
    // files that are required by the composer-generated autoload.php
    // https://getcomposer.org/doc/04-schema.md#files
    for (const auto &autoload_filename : autoload["files"]) {
      files_to_require_.emplace_back(pkg_root + "/" + autoload_filename.as<std::string>());
    }
  }
}
