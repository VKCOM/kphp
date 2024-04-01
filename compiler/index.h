// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <forward_list>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

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
  unsigned long long crc64{static_cast<unsigned long long>(-1)};               // actually, some hash of cpp contents
  unsigned long long crc64_with_comments{static_cast<unsigned long long>(-1)}; // actually, some hash of comments (php lines)
  bool on_disk{false};
  bool needed{false};
  Target *target{nullptr};
  // Don't know where else I can save it
  std::forward_list<std::string> includes;
  std::forward_list<std::string> lib_includes;
  bool compile_with_debug_info_flag{true};
  bool is_changed{false};

  explicit File(std::string path)
    : path(std::move(path)) {}
  void calc_name_ext_and_others(const std::string &basedir);
  long long read_stat() __attribute__((warn_unused_result));
  void set_mtime(long long mtime_value);
  void unlink();
};

// the Index class represents a set of files appeared during codegeneration/compilation
// not only .cpp/.h files! .o and others can also be here; for .cpp/.h, also see CompilerCode::cpp_index
class Index {
private:
  // when kphp2cpp starts, its destination directory points to a directory with existing files from a previous launch
  // cpp/h files that didn't change since previous launch, are not rewritten (compared by hashes)
  // we separate files from prev launch and newly-appeared in the current launch
  // why? mostly for this reason:
  // * files_prev_launch are loaded from disk / from an indexfile — in a single thread, they are not modified later
  // * files_only_cur_launch are read/written from multiple threads — and therefore a mutex is required
  // keys of these maps are relative file names
  // keys files_only_cur_launch always differ from keys of prev launch: if a file with the same name is generated, it's found in prev
  std::unordered_map<std::string, File *> files_prev_launch;
  std::unordered_map<std::string, File *> files_only_cur_launch;
  mutable std::mutex mutex_rw_cur_launch;

  std::string dir;
  std::set<vk::string_view> subdirs;
  std::string index_file;
  void create_subdir(vk::string_view subdir);

  static Index *current_index;
  static int scan_dir_callback(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
  File *on_new_file_during_scan_dir(std::string path);

  void fix_path(std::string &path) const;

public:
  void set_dir(const std::string &dir);
  const std::string &get_dir() const;
  void sync_with_dir(const std::string &dir);
  void del_extra_files();
  File *insert_file(std::string path);
  File *get_file(std::string path) const;

  // return a vector combining values of files_prev_codegen and files_cur_codegen; can be improved
  std::vector<File *> get_files() const;
  size_t get_files_count() const;

  // stupid text version. to be improved
  void save_into_index_file();
  void load_from_index_file();
};
