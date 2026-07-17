// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/composer.h"

#include <filesystem>
#include <fstream>

#include "common/algorithms/contains.h"
#include "common/wrappers/fmt_format.h"

#include "compiler/compiler-core.h"
#include "compiler/data/composer-json-data.h"
#include "compiler/kphp_assert.h"
#include "compiler/lexer.h"
#include "compiler/token.h"


static bool file_exists(const std::string &filename) {
  return access(filename.c_str(), F_OK) == 0;
};

// Scans a single PHP file using KPHP's own lexer and returns all fully-qualified
// class names declared in it.  Uses '/' as the namespace separator (the same
// convention used by the rest of this file).
//
// Requires lexer_init() to have been called beforehand (guaranteed by the
// compiler pipeline: lexer_init() runs before init_composer_class_loader()).
static std::vector<std::string> collect_php_class_names(const std::string &filepath) {
  std::ifstream f(filepath, std::ios::binary);
  if (!f) {
    return {};
  }
  std::string content{std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};

  std::vector<Token> tokens = php_text_to_tokens(content);

  std::vector<std::string> result;
  std::string ns_prefix;  // e.g. "Foo/Bar/" or ""

  for (size_t i = 0; i < tokens.size(); ++i) {
    const auto type = tokens[i].type();

    // track namespace declarations: "namespace Foo\Bar;"
    if (type == tok_namespace && i + 1 < tokens.size() && tokens[i + 1].type() == tok_func_name) {
      std::string ns = static_cast<std::string>(tokens[i + 1].str_val);
      std::replace(ns.begin(), ns.end(), '\\', '/');
      ns_prefix = ns + "/";
      ++i;
      continue;
    }

    // detect class/interface/trait declarations
    // the lexer produces: tok_class / tok_interface / tok_trait
    // optionally preceded by tok_abstract or tok_final (which we can just skip)
    if (type == tok_class || type == tok_interface || type == tok_trait) {
      if (i + 1 < tokens.size() && tokens[i + 1].type() == tok_func_name) {
        result.push_back(ns_prefix + static_cast<std::string>(tokens[i + 1].str_val));
        ++i;
      }
      continue;
    }
  }

  return result;
}

// Scans 'path' (a directory or a single .php file) for PHP class declarations
// and inserts the found class_name→file_path pairs into 'out'.
// Class names use '/' as the namespace separator.
static void scan_classmap_path(const std::string &path,
                                std::unordered_map<std::string, std::string> &out) {
  namespace fs = std::filesystem;

  std::error_code ec;
  fs::path fsp{path};

  if (fs::is_regular_file(fsp, ec) && fsp.extension() == ".php") {
    for (const auto &cls : collect_php_class_names(path)) {
      out.emplace(cls, path);
    }
    return;
  }

  if (fs::is_directory(fsp, ec)) {
    const auto opts = fs::directory_options::skip_permission_denied;
    for (const auto &entry : fs::recursive_directory_iterator(fsp, opts, ec)) {
      if (!entry.is_regular_file()) {
        continue;
      }
      if (entry.path().extension() != ".php") {
        continue;
      }
      const std::string file_path = entry.path().string();
      for (const auto &cls : collect_php_class_names(file_path)) {
        out.emplace(cls, file_path);
      }
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

std::string ComposerAutoloader::classmap_lookup(const std::string &class_name) const {
  auto it = autoload_classmap_.find(class_name);
  if (it != autoload_classmap_.end() && file_exists(it->second)) {
    return it->second;
  }
  return "";
}

void ComposerAutoloader::set_use_dev(bool v) {
  use_dev_ = v;
}

void ComposerAutoloader::load_root_file(const std::string &pkg_root) {
  kphp_assert(!pkg_root.empty() && pkg_root.back() == '/');
  kphp_assert(autoload_filename_.empty());
  kphp_error_return(file_exists(pkg_root + "composer.json"), fmt_format("composer.json does not exist in --composer-root {}", pkg_root));

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
  //     "classmap": [
  //       "src/",        // directory to scan for class declarations
  //       "lib/Foo.php", // single file
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
  //     "classmap": [
  //       "src/",
  //       <...>
  //     ],
  //   }
  // }

  // loading composer.json does not throw exceptions
  // on invalid json, in just fires kphp_error, but returns a valid pointer (probably, with empty package_name)
  ComposerJsonPtr composer_json = ComposerJsonPtr(new ComposerJsonData(pkg_root + "/composer.json"));
  if (!composer_json->package_name.empty()) {
    G->register_composer_json(composer_json);
  }
//  printf("composer package %s in %s\n", composer_json->package_name.c_str(), pkg_root.c_str());

  // we handle "require/autoload/files" only in direct root deps, see https://github.com/VKCOM/kphp/pull/465
  bool require_autoload_files = is_root_file || vk::contains(deps_, composer_json->package_name);
  bool use_dev = is_root_file && use_dev_;

  for (const auto &autoload_file : composer_json->autoload_files) {
    if (require_autoload_files && (!autoload_file.is_dev || use_dev)) {
      files_to_require_.emplace_back(autoload_file.file_name);
    }
  }

  for (const auto &autoload_psr0 : composer_json->autoload_psr0) {
    if (!autoload_psr0.is_dev || use_dev) {
      if (autoload_psr0.prefix_is_classname) {
        kphp_assert(autoload_psr0.dirs.size() == 1);
        autoload_psr0_classmap_[autoload_psr0.prefix] = autoload_psr0.dirs.front();
      } else {
        auto &at_prefix = autoload_psr0_[autoload_psr0.prefix];
        at_prefix.insert(at_prefix.end(), autoload_psr0.dirs.begin(), autoload_psr0.dirs.end());
      }
    }
  }

  for (const auto &autoload_psr4 : composer_json->autoload_psr4) {
    if (!autoload_psr4.is_dev || use_dev) {
      auto &at_prefix = autoload_psr4_[autoload_psr4.prefix];
      at_prefix.insert(at_prefix.end(), autoload_psr4.dirs.begin(), autoload_psr4.dirs.end());
    }
  }

  for (const auto &classmap_entry : composer_json->autoload_classmap) {
    if (!classmap_entry.is_dev || use_dev) {
      scan_classmap_path(classmap_entry.path, autoload_classmap_);
    }
  }

  for (const auto &require : composer_json->require) {
    if (is_root_file && (!require.is_dev || use_dev)) {
      deps_.insert(require.package_name);
    }
  }
}
