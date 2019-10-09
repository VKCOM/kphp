#include "compiler/index.h"

#include <fcntl.h>
#include <ftw.h>
#include <linux/limits.h>
#include <sys/stat.h>

#include "common/containers/final_action.h"
#include "common/wrappers/mkdir_recursive.h"
#include "drinkless/dl-utils-lite.h"

#include "compiler/compiler-core.h"
#include "compiler/stage.h"
#include "compiler/utils/string-utils.h"

bool is_dir(const string &path) {
  struct stat s;
  int err = stat(path.c_str(), &s);
  dl_passert (err == 0, fmt_format("Failed to stat [{}]", path).c_str());
  return S_ISDIR (s.st_mode);
}

int64_t get_mtime(const struct stat &sb) {
  return sb.st_mtime * 1000000000ll + sb.st_mtim.tv_nsec;
}

//Index:
//  holds some information about all files in given directory (and all its subdirs)
//  can be synchronized with file system
//  can be saved to file
//  can be loaded from file
//
//  for a start, files will be indexed by their names
//               hashes will be used in future

File::File(const string &path) :
  path(path) {
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

long long File::upd_mtime() {
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
  //TODO: check if it is file
  mtime = get_mtime(buf);
  //fprintf (stderr, "%lld [%d %d] %s\n", mtime, (int)buf.st_mtime, (int)buf.st_mtim.tv_nsec, path.c_str());
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

void Index::set_dir(const string &new_dir) {
  bool res_mkdir = mkdir_recursive(new_dir.c_str(), 0777);
  dl_assert(res_mkdir, fmt_format("Failed to mkdir [{}] ({})", new_dir, strerror(errno)).c_str());

  dir = get_full_path(new_dir);
  kphp_assert (!dir.empty());
  if (!is_dir(dir)) {
    kphp_error (0, fmt_format("[{}] is not a directory", dir));
    kphp_fail();
  }
  if (dir[dir.size() - 1] != '/') {
    dir += "/";
  }
  index_file = dir + "index_file";
}

const string &Index::get_dir() const {
  return dir;
}

Index *Index::current_index = nullptr;

int Index::scan_dir_callback(const char *fpath, const struct stat *sb, int typeflag,
                             struct FTW *ftwbuf __attribute__((unused))) {
  if (typeflag == FTW_D) {
    //skip
  } else if (typeflag == FTW_F) {
    // ignore index file
    if (current_index->index_file == fpath) {
      return 0;
    }
    File *f = current_index->insert_file(fpath);
    f->on_disk = true;
    long long new_mtime = get_mtime(*sb);
    //fprintf (stderr, "%lld [%d %d] %s\n", new_mtime, (int)sb->st_mtime, (int)sb->st_mtim.tv_nsec, fpath);
    if (f->mtime != new_mtime) {
      f->crc64 = -1;
      f->crc64_with_comments = -1;
    }
    f->mtime = new_mtime;
  } else {
    kphp_error (0, fmt_format("Failed to scan directory [fpath={}]\n", fpath));
    kphp_fail();
  }
  return 0;
}

void Index::sync_with_dir(const string &new_dir) {
  set_dir(new_dir);
  for (const auto &path_and_file : files) {
    path_and_file.second->on_disk = false;
  }
  current_index = this;
  int err = nftw(dir.c_str(), scan_dir_callback, 10, FTW_PHYS/*ignore symbolic links*/);
  dl_passert (err == 0, fmt_format("ftw [{}] failed", dir).c_str());

  for (auto it = files.begin(); it != files.end();) {
    if (it->second->on_disk) {
      ++it;
    } else {
      delete it->second;
      it = files.erase(it);
    }
  }
}

void Index::del_extra_files() {
  for (auto it = files.begin(); it != files.end();) {
    File *file = it->second;
    if (!file->needed) {
      if (file->on_disk) {
        if (G->env().get_verbosity() > 0) {
          fprintf(stderr, "unlink %s\n", file->path.c_str());
        }
        int err = unlink(file->path.c_str());
        if (err != 0) {
          kphp_error (0, fmt_format("Failed to unlink file {}: {}", file->path, strerror(errno)));
          kphp_fail();
        }
      }

      delete file;
      it = files.erase(it);
    } else {
      ++it;
    }
  }
}

void Index::create_subdir(const string &subdir) {
  if (!subdirs.insert(subdir).second) {
    return;
  }
  string full_path = get_dir() + subdir;
  int ret = mkdir(full_path.c_str(), 0777);
  dl_passert (ret != -1 || errno == EEXIST, full_path.c_str());
  if (errno == EEXIST && !is_dir(full_path)) {
    kphp_error (0, fmt_format("[{}] is not a directory", full_path.c_str()));
    kphp_fail();
  }
}

void Index::fix_path(std::string &path) const {
  kphp_assert(!path.empty());
  if (path[0] != '/') {
    path.insert(0, get_dir());
  }
  kphp_assert (path[0] == '/');
}

File *Index::get_file(std::string path) const {
  fix_path(path);
  auto it = files.find(path);
  return it == files.end() ? nullptr : it->second;
}

File *Index::insert_file(std::string path) {
  fix_path(path);
  File *&f = files[path];
  if (f == nullptr) {
    f = new File();
    f->path = path;
    kphp_assert (f->path.size() > get_dir().size());
    kphp_assert (strncmp(f->path.c_str(), get_dir().c_str(), get_dir().size()) == 0);
    string name = f->path.substr(get_dir().size());
    string::size_type dot_i = name.find_last_of('.');
    if (dot_i == string::npos) {
      dot_i = name.size();
    }
    string::size_type slash_i = name.find_last_of('/', dot_i - 1);
    if (slash_i != string::npos) {
      f->subdir = name.substr(0, slash_i + 1);
      create_subdir(f->subdir);
    } else {
      f->subdir = "";
    }
    f->ext = name.substr(dot_i);
    f->name_without_ext = name.substr(0, dot_i);
    f->name.swap(name);
  }
  return f;
}

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

  std::unique_ptr<FILE, int (*)(FILE *)> f{fdopen(tmp_index_file_fd, "w"), fclose};
  kphp_assert(f);
  auto file_deleter = vk::finally([&index_file_tmp_name]() { unlink(index_file_tmp_name.c_str()); });

  if (fprintf(f.get(), "%zu\n", files.size()) <= 0) {
    kphp_warning(fmt_format("Can't write the number of files into tmp index file '{}'", index_file_tmp_name));
    return;
  }

  for (const auto &file : get_files()) {
    const std::string path = file->path.substr(dir.length());
    if (fprintf(f.get(), "%s %llu %llu\n", path.c_str(), file->crc64, file->crc64_with_comments) <= 0) {
      kphp_warning (fmt_format("Can't write crc32 into tmp index file '{}'", index_file_tmp_name));
      return;
    }
  }

  f.reset();
  if (rename(index_file_tmp_name.c_str(), index_file.c_str()) == -1) {
    kphp_warning(fmt_format("Can't rename '{}' into '{}': {}", index_file_tmp_name, index_file, strerror(errno)));
  }
}

void Index::load_from_index_file() {
  if (index_file.empty()) {
    return;
  }
  std::unique_ptr<FILE, int (*)(FILE *)> f{fopen(index_file.c_str(), "r"), fclose};
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
    File *file = get_file(tmp);
    if (file && file->mtime < file_index_mtime) {
      file->crc64 = crc64;
      file->crc64_with_comments = crc64_with_comments;
    }
  }
}


