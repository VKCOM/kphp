// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <vector>

#include "compiler/data/data_ptr.h"
#include "compiler/debug.h"

// represents composer.json structure (the parts we care about)
// they are used for autoloading (in the same manner as Composer does)
// and as implicit modulites
class ComposerJsonData {
  DEBUG_STRING_METHOD {
    return package_name;
  }

  ComposerJsonPtr get_self_ptr() {
    return ComposerJsonPtr(this);
  }

public:
  struct RequireItem {
    std::string package_name;
    std::string version;
    bool is_dev; // whether it's in "require-dev"
  };

  struct AutoloadPsr0Item {
    std::string prefix;
    std::vector<std::string> dirs;
    bool prefix_is_classname; // if true, psr-0 key is "ns\\class" not "ns\\"; dirs.size = 1, dirs[0] is a .php file
    bool is_dev;              // whether it's in "autoload-dev"
  };

  struct AutoloadPsr4Item {
    std::string prefix;
    std::vector<std::string> dirs;
    bool is_dev; // whether it's in "autoload-dev"
  };

  struct AutoloadFileItem {
    std::string file_name;
    bool is_dev; // whether it's in "autoload-dev"
  };

  explicit ComposerJsonData(const std::string &json_filename);

  // full absolute path to composer.json, it's registered in G, it has ->dir, kphp_error can point to it
  SrcFilePtr json_file;

  // "name", e.g. "vk/common"
  std::string package_name;

  // "require" and "require-dev", e.g. [ {"vkcom/kphp-polyfills", "^1.0", false}, ... ]
  std::vector<RequireItem> require;

  // "autoload/psr-0" and "autoload-dev/psr-0", e.g. [ { "foo\\my_c", ["src/foo/my/c.php"], true, false}, ... ]
  std::vector<AutoloadPsr0Item> autoload_psr0;

  // "autoload/psr-4" and "autoload-dev/psr-4", e.g. [ { "prefix1", ["dir"], false}, ... ]
  std::vector<AutoloadPsr4Item> autoload_psr4;

  // "autoload/files" and "autoload-dev/files", e.g. [ {"file.php", true}, ... ]
  std::vector<AutoloadFileItem> autoload_files;
};
