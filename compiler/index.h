// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <forward_list>
#include <vector>
#include <tuple>

#include "common/wrappers/iterator_range.h"
#include "common/wrappers/string_view.h"

/*** Index ***/
class Target;

class File {
private:
  File(const File &from);
  File &operator=(const File &from);
public:
  std::string path;
  vk::string_view name;
  vk::string_view ext;
  vk::string_view name_without_ext;
  vk::string_view subdir;
  long long mtime{0};
  long long file_size{0};
  unsigned long long crc64{static_cast<unsigned long long>(-1)};                // actually, some hash of cpp contents
  unsigned long long crc64_with_comments{static_cast<unsigned long long>(-1)};  // actually, some hash of comments (php lines)
  bool on_disk{false};
  bool needed{false};
  Target *target{nullptr};
  //Don't know where else I can save it
  std::forward_list<std::string> includes;
  std::forward_list<std::string> lib_includes;
  bool compile_with_debug_info_flag{true};
  bool is_changed{false};

  explicit File(const std::string &path = {});
  long long read_stat() __attribute__ ((warn_unused_result));
  void set_mtime(long long mtime_value);
  void unlink();
};

class Index {
private:
  std::unordered_map<std::string, File *> files;
  std::string dir;
  std::set<vk::string_view> subdirs;
  std::string index_file;
  void create_subdir(vk::string_view subdir);

  static Index *current_index;
  static int scan_dir_callback(const char *fpath, const struct stat *sb,
                               int typeflag, struct FTW *ftwbuf);

  void fix_path(std::string &path) const;

public:
  void set_dir(const std::string &dir);
  const std::string &get_dir() const;
  void sync_with_dir(const std::string &dir);
  void del_extra_files();
  File *insert_file(std::string path);
  File *get_file(std::string path) const;

  auto get_files() const {
    return vk::make_transform_iterator_range(vk::pair_second_getter{}, files.cbegin(), files.cend());
  }

  // stupid text version. to be improved
  void save_into_index_file();
  void load_from_index_file();
};
