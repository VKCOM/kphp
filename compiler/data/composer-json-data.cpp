// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/composer-json-data.h"

#include "yaml-cpp/yaml.h"

#include "compiler/compiler-core.h"
#include "compiler/data/src-dir.h"
#include "compiler/data/src-file.h"


[[gnu::cold]] static void fire_json_error(ComposerJsonPtr inside_j, const std::string &reason, int line) {
  inside_j->json_file->load();  // load a file from disk, so that error message in console outputs a line

  stage::set_file(inside_j->json_file);
  stage::set_line(line);
  kphp_error(0, fmt_format("Failed loading {}:\n{}", inside_j->json_file->relative_file_name, reason));
}

[[gnu::cold]] static void fire_json_error(ComposerJsonPtr inside_p, const std::string &reason, const YAML::Node &y_node) {
  fire_json_error(inside_p, reason, y_node.Mark().line + 1);
}


class ComposerJsonParser {
  SrcDirPtr composer_json_dir;   // /path/to/folder/ (where composer.json is placed)
  ComposerJsonPtr out;

  [[gnu::always_inline]] const std::string &as_string(const YAML::Node &y) {
    if (!y.IsScalar() || y.Scalar().empty()) {
      fire_json_error(out, "expected non-empty string", y);
    }
    return y.Scalar();
  }

  [[gnu::always_inline]] const std::string &as_string_maybe_empty(const YAML::Node &y) {
    if (!y.IsScalar()) {
      fire_json_error(out, "expected string", y);
    }
    return y.Scalar();
  }

  // in autoload/psr-0 and autoload/psr-4, dirs can occur in {"key":"dir"} or {"key":["dir1","dir2"]}
  std::string parse_composer_json_autoload_psr_dir(const YAML::Node &y_dir) {
    std::string dir = as_string_maybe_empty(y_dir);
    if (dir.empty()) {
      dir = "./"; // composer interprets "" as "./" or "."
    }
    std::replace(dir.begin(), dir.end(), '\\', '/');
    if (dir.back() != '/') {
      dir.push_back('/');   // ensure that dir always ends with '/'
    }

    return composer_json_dir->full_dir_name + dir;
  }

  // parse autoload/psr-0 {"namespace\\classname":"filename"} case; \\ was already replaced by /
  // according to psr-0 rules, {"foo\\my_test4":"src"} means that class my_test4 is in {root}/src/foo/my/test4.php
  std::string parse_composer_json_autoload_psr0_classname(std::string class_name, const YAML::Node &y_filename) {
    std::string path = as_string_maybe_empty(y_filename);
    auto last_slash = class_name.find_last_of('/');
    auto begin = class_name.begin();
    if (last_slash != std::string::npos) {
      std::advance(begin, last_slash);
    }
    std::replace(begin, class_name.end(), '_', '/');

    std::string filename{composer_json_dir->full_dir_name};
    filename.append(path).append("/").append(class_name).append(".php");
    return filename;
  }

  // in autoload/psr-0, either {"ns\\":"dir"} or {"ns\\":["dir1", "dir2"]} or {"ns\\classname":"file"} is allowed
  // here we handle all cases
  void parse_composer_json_autoload_psr0_kv(const YAML::Node &y_key, const YAML::Node &y_dirs, bool is_autoload_dev) {
    std::string key = as_string_maybe_empty(y_key);
    bool is_classname = !key.empty() && key.back() != '\\';
    std::replace(key.begin(), key.end(), '\\', '/');

    std::vector<std::string> dirs;
    if (y_dirs.IsSequence()) {
      for (const auto &y_require_dir : y_dirs) {
        if (is_classname) {
          dirs.emplace_back(parse_composer_json_autoload_psr0_classname(key, y_require_dir));
        } else {
          dirs.emplace_back(parse_composer_json_autoload_psr_dir(y_require_dir) + key);
        }
      }
    } else if (y_dirs.IsScalar()) {
      if (is_classname) {
        dirs.emplace_back(parse_composer_json_autoload_psr0_classname(key, y_dirs));
      } else {
        dirs.emplace_back(parse_composer_json_autoload_psr_dir(y_dirs) + key);
      }
    } else {
      fire_json_error(out, "invalid autoload psr-0 item", y_dirs);
    }

    out->autoload_psr0.emplace_back(ComposerJsonData::AutoloadPsr0Item{std::move(key), std::move(dirs), is_classname, is_autoload_dev});
  }

  // in autoload/psr-4, either {"ns\\":"dir"} or {"ns\\":["dir1", "dir2"]} is allowed
  void parse_composer_json_autoload_psr4_kv(const YAML::Node &y_prefix, const YAML::Node &y_dirs, bool is_autoload_dev) {
    std::string prefix = as_string_maybe_empty(y_prefix);
    std::replace(prefix.begin(), prefix.end(), '\\', '/');

    std::vector<std::string> dirs;
    if (y_dirs.IsSequence()) {
      for (const auto &y_require_dir : y_dirs) {
        dirs.emplace_back(parse_composer_json_autoload_psr_dir(y_require_dir));
      }
    } else if (y_dirs.IsScalar()) {
      dirs.emplace_back(parse_composer_json_autoload_psr_dir(y_dirs));
    } else {
      fire_json_error(out, "invalid autoload psr-4 item", y_dirs);
    }

    out->autoload_psr4.emplace_back(ComposerJsonData::AutoloadPsr4Item{std::move(prefix), std::move(dirs), is_autoload_dev});
  };

  // in autoload/files, every file.php is a relative path
  void parse_composer_json_autoload_file(const YAML::iterator::value_type &y_filename, bool is_autoload_dev) {
    std::string abs_file_name = composer_json_dir->full_dir_name + as_string(y_filename);
    out->autoload_files.emplace_back(ComposerJsonData::AutoloadFileItem{std::move(abs_file_name), is_autoload_dev});
  }

  // parse composer.json "autoload" and "autoload-dev"
  void parse_composer_json_autoload(const YAML::Node &y_autoload, bool is_autoload_dev) {
    if (const auto &y_psr0 = y_autoload["psr-0"]) {
      for (const auto &kv : y_psr0) {
        parse_composer_json_autoload_psr0_kv(kv.first, kv.second, is_autoload_dev);
      }
    }
    if (const auto &y_psr4 = y_autoload["psr-4"]) {
      for (const auto &kv : y_psr4) {
        parse_composer_json_autoload_psr4_kv(kv.first, kv.second, is_autoload_dev);
      }
    }
    if (const auto &y_files = y_autoload["files"]) {
      for (const auto &y_filename : y_files) {
        parse_composer_json_autoload_file(y_filename, is_autoload_dev);
      }
    }
  }

  // parse composer.json "require" and "require-dev"
  void parse_composer_json_require(const YAML::Node &y_require, bool is_require_dev) {
    for (const auto &kv : y_require) {
      out->require.emplace_back(ComposerJsonData::RequireItem{as_string(kv.first), as_string(kv.second), is_require_dev});
    }
  }

  // parse composer.json "name"
  std::string parse_composer_json_name(const YAML::Node &y_name) {
    std::string name = as_string(y_name);
    // it's supposed to be "vendorname/packagename", but we allow arbitrary
    return name;
  }

public:
  explicit ComposerJsonParser(ComposerJsonPtr out)
    : composer_json_dir(out->json_file->dir)
    , out(std::move(out)) {}

  void parse_composer_json_file(const YAML::Node &y_file) {
    const auto &y_name = y_file["name"];
    if (y_name && y_name.IsScalar()) {
      out->package_name = parse_composer_json_name(y_name);
    } else {
      fire_json_error(out, "'name' not specified", -1);
    }

    if (const auto &y_require = y_file["require"]) {
      parse_composer_json_require(y_require, false);
    }
    if (const auto &y_require_dev = y_file["require-dev"]) {
      parse_composer_json_require(y_require_dev, true);
    }

    if (const auto &y_autoload = y_file["autoload"]) {
      parse_composer_json_autoload(y_autoload, false);
    }
    if (const auto &y_autoload_dev = y_file["autoload-dev"]) {
      parse_composer_json_autoload(y_autoload_dev, true);
    }
  }
};

ComposerJsonData::ComposerJsonData(const std::string &json_filename)
  : json_file(G->register_file(json_filename, LibPtr{})) {
  try {
    YAML::Node y_file = YAML::LoadFile(json_file->file_name);
    ComposerJsonParser parser(get_self_ptr());
    parser.parse_composer_json_file(y_file);
  } catch (const YAML::Exception &ex) {
    fire_json_error(get_self_ptr(), ex.msg, ex.mark.line + 1);
  }
}
