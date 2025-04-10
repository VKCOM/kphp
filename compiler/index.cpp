// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/index.h"

#include <climits>
#include <fcntl.h>
#include <ftw.h>
#include <sys/stat.h>

#include "common/containers/final_action.h"
#include "common/macos-ports.h"
#include "common/wrappers/mkdir_recursive.h"
#include "compiler/kphp_assert.h"

#include "compiler/compiler-core.h"
#include "compiler/stage.h"
#include "compiler/utils/string-utils.h"

bool is_dir(const std::string& path) {
  struct stat s;
  int err = stat(path.c_str(), &s);
  kphp_assert_msg(err == 0, fmt_format("Failed to stat [{}]", path));
  return S_ISDIR(s.st_mode);
}

int64_t get_mtime(const struct stat& sb) {
  return sb.st_mtime * 1000000000ll + sb.st_mtim.tv_nsec;
}

// Index:
//   holds some information about all files in given directory (and all its subdirs)
//   can be synchronized with file system
//   can be saved to file
//   can be loaded from file
//
//   for a start, files will be indexed by their names
//                hashes will be used in future

void File::calc_name_ext_and_others(const std::string& basedir) {
  kphp_assert(vk::string_view{path}.starts_with(basedir));
  name = vk::string_view{path}.substr(basedir.size());
  auto dot_i = name.rfind('.');
  if (dot_i == std::string::npos) {
    dot_i = name.size();
  }
  auto slash_i = name.rfind('/', dot_i - 1);
  if (slash_i != std::string::npos) {
    subdir = name.substr(0, slash_i + 1);
  } else {
    subdir = vk::string_view{};
  }
  ext = name.substr(dot_i);
  name_without_ext = name.substr(0, dot_i);
}

void File::set_mtime(long long mtime_value) {
  timespec times[2]; // {atime, mtime}
  times[0].tv_sec = times[0].tv_nsec = UTIME_NOW;
  times[1].tv_sec = mtime_value / 1000000000ll;
  times[1].tv_nsec = mtime_value % 1000000000ll;
  if (utimensat(AT_FDCWD, path.c_str(), times, 0) < 0) {
    kphp_warning(fmt_format("Can't set mtime for {} with error {}", path.c_str(), strerror(errno)));
  }
}

long long File::read_stat() {
  struct stat buf;
  int err = stat(path.c_str(), &buf);
  if (err == -1) {
    if (errno == ENOENT) {
      mtime = 0;
      on_disk = false;
      return 0;
    }
    perror("stat failed: ");
    return -1;
  }
  on_disk = true;
  // TODO: check if it is file
  mtime = get_mtime(buf);
  file_size = buf.st_size;
  // fprintf (stderr, "%lld [%d %d] %s\n", mtime, (int)buf.st_mtime, (int)buf.st_mtim.tv_nsec, path.c_str());
  return 1;
}

void File::unlink() {
  int err = ::unlink(path.c_str());
  if (err == -1 && errno != ENOENT) {
    perror("unlink failed: ");
    return;
  }
  mtime = 0;
  on_disk = false;
}

void Index::set_dir(const std::string& new_dir) {
  bool res_mkdir = mkdir_recursive(new_dir.c_str(), 0777);
  kphp_assert_msg(res_mkdir, fmt_format("Failed to mkdir [{}] ({})", new_dir, strerror(errno)));

  dir = get_full_path(new_dir);
  kphp_assert(!dir.empty());
  if (!is_dir(dir)) {
    kphp_error(0, fmt_format("[{}] is not a directory", dir));
    kphp_fail();
  }
  if (dir[dir.size() - 1] != '/') {
    dir += "/";
  }
  index_file = dir + "index_file";
}

const std::string& Index::get_dir() const {
  return dir;
}

Index* Index::current_index = nullptr;

int Index::scan_dir_callback(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf __attribute__((unused))) {
  if (typeflag == FTW_D) {
    // skip
  } else if (typeflag == FTW_F) {
    // ignore index file
    if (current_index->index_file == fpath) {
      return 0;
    }
    char full_path[PATH_MAX + 1];
    File* f = current_index->on_new_file_during_scan_dir(realpath(fpath, full_path));
    f->on_disk = true;
    long long new_mtime = get_mtime(*sb);
    // fprintf (stderr, "%lld [%d %d] %s\n", new_mtime, (int)sb->st_mtime, (int)sb->st_mtim.tv_nsec, fpath);
    if (f->mtime != new_mtime) {
      f->crc64 = -1;
      f->crc64_with_comments = -1;
    }
    f->mtime = new_mtime;
  } else {
    kphp_error(0, fmt_format("Failed to scan directory [fpath={}]\n", fpath));
    kphp_fail();
  }
  return 0;
}

void Index::sync_with_dir(const std::string& dir) {
  kphp_assert(files_prev_launch.empty() && files_only_cur_launch.empty() && this->dir.empty());
  set_dir(dir);
  current_index = this; // Presence of this global variable is strange. Used for traversal callback in NFTW function
  int err = nftw(dir.c_str(), scan_dir_callback, 10, FTW_PHYS /*ignore symbolic links*/);
  kphp_assert_msg(err == 0, fmt_format("ftw [{}] failed", dir));
}

void Index::filter_with_whitelist(const std::vector<std::string>& white_list) {
  subdirs.clear();

  std::unordered_set<vk::string_view> white_set;
  white_set.reserve(white_list.size());
  for (const auto& allowed : white_list) {
    white_set.insert(allowed);
  }

  for (auto it = files_prev_launch.begin(); it != files_prev_launch.end();) {
    File* file = it->second;
    if (white_set.count(file->name) == 0) {
      delete file;
      it = files_prev_launch.erase(it);
    } else {
      if (!file->subdir.empty()) {
        subdirs.insert(file->subdir);
      }
      ++it;
    }
  }
}

void Index::del_all_files() {
  for (File* file : get_files()) {
    if (G->settings().verbosity.get() > 1) {
      fprintf(stderr, "unlink %s\n", file->path.c_str());
    }
    int err = unlink(file->path.c_str());
    if (err != 0) {
      kphp_error(0, fmt_format("Failed to unlink file {}: {}", file->path, strerror(errno)));
      kphp_fail();
    }

    delete file;
  }
}

void Index::del_extra_files() {
  // delete files that were not emerged by the current launch
  // we need only to iterate through files_prev_codegen and unlink if !file->needed
  for (auto it = files_prev_launch.begin(); it != files_prev_launch.end();) {
    File* file = it->second;
    if (!file->needed) {
      if (file->on_disk) {
        if (G->settings().verbosity.get() > 1) {
          fprintf(stderr, "unlink %s\n", file->path.c_str());
        }
        int err = unlink(file->path.c_str());
        if (err != 0) {
          kphp_error(0, fmt_format("Failed to unlink file {}: {}", file->path, strerror(errno)));
          kphp_fail();
        }
      }

      delete file;
      it = files_prev_launch.erase(it);
    } else {
      ++it;
    }
  }
}

void Index::create_subdir(vk::string_view subdir) {
  if (subdir.empty() || !subdirs.insert(subdir).second) {
    return;
  }
  std::string full_path = get_dir() + subdir;
  int ret = mkdir_recursive(full_path.c_str(), 0777);
  kphp_assert_msg(ret != -1 || errno == EEXIST, full_path);
  if (errno == EEXIST && !is_dir(full_path)) {
    kphp_error(0, fmt_format("[{}] is not a directory", full_path.c_str()));
    kphp_fail();
  }
}

void Index::fix_path(std::string& path) const {
  kphp_assert(!path.empty());
  if (path[0] != '/') {
    path.insert(0, get_dir());
  }
  kphp_assert(path[0] == '/');
}

File* Index::get_file(std::string path) const {
  fix_path(path);
  auto it = files_prev_launch.find(path);
  if (it != files_prev_launch.end()) {
    return it->second;
  }
  if (files_only_cur_launch.empty()) {
    return nullptr;
  }

  std::lock_guard<std::mutex> guard{mutex_rw_cur_launch};
  it = files_only_cur_launch.find(path);
  if (it != files_only_cur_launch.end()) {
    return it->second;
  }
  return nullptr;
}

File* Index::insert_file(std::string path) {
  fix_path(path);

  File* f = get_file(path);
  if (f != nullptr) {
    return f;
  }
  //  printf("%s not found in index, creating\n", file_name.c_str());

  std::lock_guard<std::mutex> guard{mutex_rw_cur_launch}; // It's reasonable only for Php2Cpp

  f = new File(path);
  f->calc_name_ext_and_others(get_dir());
  create_subdir(f->subdir);

  files_only_cur_launch[path] = f;
  return f;
}

File* Index::on_new_file_during_scan_dir(std::string path) {
  fix_path(path);

  File* f = new File(path);
  f->calc_name_ext_and_others(get_dir());
  create_subdir(f->subdir);

  files_prev_launch[path] = f;
  return f;
}

std::vector<File*> Index::get_files() const {
  // return a vector to traverse values of files_prev_codegen, then files_cur_codegen
  // can be improved by returning a custom iterator; for now, leave a stupid version
  std::vector<File*> files_joined;
  for (const auto& it : files_prev_launch) {
    files_joined.emplace_back(it.second);
  }
  for (const auto& it : files_only_cur_launch) {
    files_joined.emplace_back(it.second);
  }
  return files_joined;
}

size_t Index::get_files_count() const {
  return files_prev_launch.size() + files_only_cur_launch.size();
}

// save hashes of all files into a single index file — to load them in the future instead of reading every file contents
// stupid text version. to be improved
void Index::save_into_index_file() {
  if (index_file.empty()) {
    return;
  }
  std::string index_file_tmp_name = index_file + "XXXXXX";
  const int tmp_index_file_fd = mkstemp(&index_file_tmp_name[0]);
  if (tmp_index_file_fd == -1) {
    kphp_warning(fmt_format("Can't create tmp file for index file '{}': {}", index_file, strerror(errno)));
    return;
  }

  std::unique_ptr<FILE, int (*)(FILE*)> f{fdopen(tmp_index_file_fd, "w"), fclose};
  kphp_assert(f);
  auto file_deleter = vk::finally([&index_file_tmp_name]() { unlink(index_file_tmp_name.c_str()); });

  if (fprintf(f.get(), "%zu\n", get_files_count()) <= 0) {
    kphp_warning(fmt_format("Can't write the number of files into tmp index file '{}'", index_file_tmp_name));
    return;
  }

  for (const auto& file : get_files()) {
    const std::string path = file->path.substr(dir.length());
    if (fprintf(f.get(), "%s %llu %llu\n", path.c_str(), file->crc64, file->crc64_with_comments) <= 0) {
      kphp_warning(fmt_format("Can't write crc32 into tmp index file '{}'", index_file_tmp_name));
      return;
    }
  }

  f.reset();
  if (rename(index_file_tmp_name.c_str(), index_file.c_str()) == -1) {
    kphp_warning(fmt_format("Can't rename '{}' into '{}': {}", index_file_tmp_name, index_file, strerror(errno)));
  }
}

// load a list of crc64 from an index file — hashes of prev codegeneration for cpp/h
// this is an optimization: avoid reading every file contents
// (hashes are also saved in every cpp/h file at the top, so everything works without this index file, but slower)
void Index::load_from_index_file() {
  if (index_file.empty()) {
    return;
  }
  std::unique_ptr<FILE, int (*)(FILE*)> f{fopen(index_file.c_str(), "r"), fclose};
  if (!f) {
    return;
  }
  struct stat statbuf;
  if (fstat(fileno(f.get()), &statbuf) == -1) {
    kphp_warning(fmt_format("Can't get stat from index file '{}': {}", index_file, strerror(errno)));
    return;
  }

  const uint64_t file_index_mtime = get_mtime(statbuf);
  size_t files = 0;
  if (fscanf(f.get(), "%zu\n", &files) != 1) {
    kphp_warning(fmt_format("Can't read the number of files from index file '{}:1'", index_file));
    return;
  }

  char tmp[501] = {0};
  unsigned long long crc64 = -1;
  unsigned long long crc64_with_comments = -1;
  for (size_t i = 0; i < files; i++) {
    if (fscanf(f.get(), "%500s %llu %llu", tmp, &crc64, &crc64_with_comments) != 3) {
      kphp_warning(fmt_format("Can't read crc32 from index file '{}:{}'", index_file, i + 2));
      return;
    }
    File* file = get_file(tmp);
    if (file && file->mtime < file_index_mtime) {
      file->crc64 = crc64;
      file->crc64_with_comments = crc64_with_comments;
    }
  }
}
