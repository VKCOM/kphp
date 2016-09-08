#include <ftw.h>
#include <linux/limits.h>
#include <sys/stat.h>


#include "../common.h"
#include "stage.h"
#include "index.h"
#include "compiler-core.h"

void mkpath (const string &s) {
  string path = s;
  int n = (int)s.size();
  for (int i = 1; i <= n; i++) {
    if (i == n || path[i] == '/') {
      if (i != n) {
        path[i] = 0;
      }

      int f = mkdir (path.c_str(), 0777);
      dl_passert (f == 0 || errno == EEXIST, 
          dl_pstr ("Failed to mkdir [%s]", path.c_str()));

      if (i != n) {
        path[i] = '/';
      }
    }
  }
}

bool is_dir (const string &path) {
  struct stat s;
  int err = stat (path.c_str(), &s);
  dl_passert (err == 0, dl_pstr ("Failed to stat [%s]", path.c_str()));
  return S_ISDIR (s.st_mode);
}

//Index:
//  holds some information about all files in given directory (and all its subdirs)
//  can be synchronized with file system
//  can be saved to file
//  can be loaded from file
//
//  for a start, files will be indexed by their names
//               hashes will be used in future
File::File() :
  path(),
  mtime (0),
  crc64 (-1),
  crc64_with_comments (-1),
  on_disk (false),
  needed (false),
  target (NULL) {
}
File::File(const string &path) :
  path (path),
  mtime (0),
  crc64 (-1),
  crc64_with_comments (-1),
  on_disk (false),
  needed (false),
  target (NULL) {
}
void File::set_mtime(long long mtime_value) {
  timespec times[2]; // {atime, mtime}
  times[0].tv_sec = times[0].tv_nsec = UTIME_NOW;
  times[1].tv_sec = mtime_value / 1000000000ll;
  times[1].tv_nsec = mtime_value % 1000000000ll;
  if (utimensat(AT_FDCWD, path.c_str(), times, 0) < 0) {
    kphp_warning(dl_pstr("Can't set mtime for %s with error %m", path.c_str()));
  }
}
long long File::upd_mtime() {
  struct stat buf;
  int err = stat (path.c_str(), &buf);
  if (err == -1) {
    if (errno == ENOENT) {
      mtime = 0;
      on_disk = false;
      return 0;
    }
    perror ("stat failed: ");
    return -1;
  }
  on_disk = true;
  //TODO: check if it is file
  mtime = buf.st_mtime * 1000000000ll + buf.st_mtim.tv_nsec;
  //fprintf (stderr, "%lld [%d %d] %s\n", mtime, (int)buf.st_mtime, (int)buf.st_mtim.tv_nsec, path.c_str());
  return 1;
}
void File::unlink() {
  int err = ::unlink (path.c_str());
  if (err == -1 && errno != ENOENT) {
    perror ("unlink failed: ");
    return;
  }
  mtime = 0;
  on_disk = false;
}

void Index::set_dir (const string &new_dir) {
  mkpath (new_dir);
  dir = get_full_path (new_dir);
  kphp_assert (!dir.empty());
  if (!is_dir (dir)) {
    kphp_error (0, dl_pstr ("[%s] is not a directory", dir.c_str()));
    kphp_fail();
  }
  if (dir[dir.size() - 1] != '/') {
    dir += "/";
  }
}
const string &Index::get_dir() const {
  return dir;
}

Index *Index::current_index = NULL;
int Index::scan_dir_callback (const char *fpath, const struct stat *sb, int typeflag,
    struct FTW *ftwbuf __attribute__((unused))) {
  if (typeflag == FTW_D) {
    //skip
  } else if (typeflag == FTW_F) {
    File *f = current_index->get_file (fpath, true);
    f->on_disk = true;
    long long new_mtime = sb->st_mtime * 1000000000ll + sb->st_mtim.tv_nsec;
    //fprintf (stderr, "%lld [%d %d] %s\n", new_mtime, (int)sb->st_mtime, (int)sb->st_mtim.tv_nsec, fpath);
    if (f->mtime != new_mtime) {
      f->crc64 = -1;
      f->crc64_with_comments = -1;
    }
    f->mtime = new_mtime;
  } else {
    kphp_error (0, dl_pstr ("Failed to scan directory [fpath=%s]\n", fpath));
    kphp_fail();
  }
  return 0;
}

void Index::sync_with_dir (const string &new_dir) {
  set_dir (new_dir);
  FOREACH (files, it) {
    it->second->on_disk = false;
  }
  current_index = this;
  int err = nftw (dir.c_str(), scan_dir_callback, 10, FTW_PHYS/*ignore symbolic links*/);
  dl_passert (err == 0, dl_pstr ("ftw [%s] failed", dir.c_str()));
  vector <string> to_del;
  FOREACH (files, it) {
    if (!it->second->on_disk) {
      to_del.push_back (it->first);
    }
  }
  FOREACH (to_del, it) {
    remove_file (*it);
  }
}

void Index::del_extra_files() {
  vector <string> to_del;
  FOREACH (files, it) {
    File *file = it->second;
    if (!file->needed) {
      kphp_assert (file->on_disk);
      if (G->env().get_verbosity() > 0) {
        fprintf(stderr, "unlink %s\n", file->path.c_str());
      }
      int err = unlink (file->path.c_str());
      if (err != 0) {
        kphp_error (0, dl_pstr("Failed to unlink file %s: %m", file->path.c_str()));
        kphp_fail();
      }
      file->on_disk = false;
      to_del.push_back (it->first);
    }
  }
  FOREACH (to_del, it) {
    remove_file (*it);
  }
}

void Index::remove_file (const string &path) {
  map <string, File *>::iterator it = files.find (path);
  kphp_assert (it != files.end());
  delete it->second;
  files.erase (it);
}

void Index::create_subdir (const string &subdir) {
  if (!subdirs.insert (subdir).second) {
    return;
  }
  string full_path = get_dir() + subdir;
  int ret = mkdir (full_path.c_str(), 0777);
  dl_passert (ret != -1 || errno == EEXIST, full_path.c_str());
  if (errno == EEXIST && !is_dir (full_path)) {
    kphp_error (0, dl_pstr ("[%s] is not a directory", full_path.c_str()));
    kphp_fail();
  }
}

File *Index::get_file (string path, bool force/* = false*/) {
  if (path[0] != '/') {
    path = get_dir() + path;
  }
  kphp_assert (path[0] == '/');

  if (force) {
    File * &f = files[path];
    if (f == NULL) {
      f = new File();
      f->path = path;
      kphp_assert (f->path.size() > get_dir().size());
      kphp_assert (strncmp (f->path.c_str(), get_dir().c_str(), get_dir().size()) == 0);
      string name = f->path.substr (get_dir().size());
      int dot_i = (int)name.size() - 1;
      while (dot_i >= 0 && name[dot_i] != '.') {
        dot_i--;
      }
      if (dot_i < 0) {
        dot_i = name.size();
      }
      int slash_i = dot_i - 1;
      while (slash_i >= 0 && name[slash_i] != '/') {
        slash_i--;
      }
      if (slash_i < 0) {
        f->subdir = "";
      } else {
        f->subdir = name.substr (0, slash_i + 1);
        create_subdir (f->subdir);
      }
      f->ext = name.substr (dot_i);
      f->name = name;
      f->name_without_ext = name.substr (0, dot_i);
    }
    return f;
  } else {
    map <string, File *>::iterator it = files.find (path);
    if (it == files.end()) {
      return NULL;
    }
    return it->second;
  }
}
vector <File *> Index::get_files() {
  vector <File *> res;
  FOREACH (files, it) {
    res.push_back (it->second);
  }
  return res;
}

//stupid text version. to be improved
void Index::save (FILE *f) {
  dl_pcheck (fprintf (f, "%d\n", (int)files.size()));
  FOREACH (files, it) {
    File *file = it->second;
    dl_pcheck (fprintf (f, "%s %llu %llu %llu\n", file->path.c_str(),
          (unsigned long long)file->mtime, file->crc64, file->crc64_with_comments));
  }
}
void Index::load (FILE *f) {
  int n;
  int err = fscanf (f, "%d\n", &n);
  dl_passert (err == 1, "Failed to load index");
  for (int i = 0; i < n; i++) {
    char tmp[500];
    unsigned long long mtime;
    unsigned long long crc64;
    unsigned long long crc64_with_comments;
    int err = fscanf (f, "%500s %llu %llu %llu", tmp, &mtime, &crc64, &crc64_with_comments);
    dl_passert (err == 4, "Failed to load index");
    File *file = get_file (tmp, true);
    file->mtime = mtime;
    file->crc64 = crc64;
    file->crc64_with_comments = crc64_with_comments;
  }
}


