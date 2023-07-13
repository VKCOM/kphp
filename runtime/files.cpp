// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/files.h"

#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <unistd.h>

#undef basename

#include "common/macos-ports.h"
#include "common/wrappers/mkdir_recursive.h"

#include "runtime/critical_section.h"
#include "runtime/interface.h"
#include "runtime/kphp_tracing.h"
#include "runtime/streams.h"
#include "runtime/string_functions.h"//php_buf, TODO

static int32_t opened_fd{-1};

const string LETTER_a("a", 1);

const string file_wrapper_name("file://", 7);

int32_t close_safe(int32_t fd) {
  dl::enter_critical_section();//OK
  int32_t result = close(fd);
  php_assert (fd == opened_fd);
  opened_fd = -1;
  dl::leave_critical_section();
  return result;
}

int32_t open_safe(const char *pathname, int32_t flags) {
  dl::enter_critical_section();//OK
  opened_fd = open(pathname, flags);
  dl::leave_critical_section();
  return opened_fd;
}

int32_t open_safe(const char *pathname, int32_t flags, mode_t mode) {
  dl::enter_critical_section();
  opened_fd = open(pathname, flags, mode);
  dl::leave_critical_section();
  return opened_fd;
}

ssize_t read_safe(int32_t fd, void *buf, size_t len, const string &file_name) {
  if (kphp_tracing::is_turned_on()) {
    kphp_tracing::on_file_io_start(fd, file_name, false);
  }
  dl::enter_critical_section();//OK
  size_t full_len = len;
  do {
    ssize_t cur_res = read(fd, buf, len);
    if (cur_res == -1) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      }
      dl::leave_critical_section();
      if (kphp_tracing::is_turned_on()) {
        kphp_tracing::on_file_io_fail(fd, -errno);  // errno is positive, pass negative
      }
      return -1;
    }
    php_assert (cur_res >= 0);
    if (cur_res == 0) {
      break;
    }

    buf = (char *)buf + cur_res;
    len -= cur_res;
  } while (len > (size_t)0);
  dl::leave_critical_section();
  if (kphp_tracing::is_turned_on()) {
    kphp_tracing::on_file_io_finish(fd, full_len - len);
  }

  return full_len - len;
}

ssize_t write_safe(int32_t fd, const void *buf, size_t len, const string &file_name) {
  if (kphp_tracing::is_turned_on()) {
    kphp_tracing::on_file_io_start(fd, file_name, true);
  }
  dl::enter_critical_section();//OK
  size_t full_len = len;
  do {
    ssize_t cur_res = write(fd, buf, len);
    if (cur_res == -1) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      }
      dl::leave_critical_section();
      if (kphp_tracing::is_turned_on()) {
        kphp_tracing::on_file_io_fail(fd, -errno);  // errno is positive, pass negative
      }
      return -1;
    }
    php_assert (cur_res >= 0);

    buf = (const char *)buf + cur_res;
    len -= cur_res;
  } while (len > (size_t)0);
  dl::leave_critical_section();
  if (kphp_tracing::is_turned_on()) {
    kphp_tracing::on_file_io_finish(fd, full_len - len);
  }

  return full_len - len;
}


#define read read_disabled
#define write write_disabled


string f$basename(const string &name, const string &suffix) {
  string name_copy(name.c_str(), name.size());
  dl::enter_critical_section();//OK
  const char *result_c_str = __xpg_basename(name_copy.buffer());
  dl::leave_critical_section();
  int l = (int)strlen(result_c_str);
  if ((int)suffix.size() <= l && !strcmp(result_c_str + l - suffix.size(), suffix.c_str())) {
    l -= suffix.size();
  }
  return {result_c_str, static_cast<string::size_type>(l)};
}

bool f$chmod(const string &s, int64_t mode) {
  dl::enter_critical_section();//OK
  bool result = (chmod(s.c_str(), static_cast<mode_t>(mode)) >= 0);
  dl::leave_critical_section();
  return result;
}

void f$clearstatcache() {
  //TODO
}

bool f$copy(const string &from, const string &to) {
  struct stat stat_buf;
  dl::enter_critical_section();//OK
  int read_fd = open(from.c_str(), O_RDONLY);
  if (read_fd < 0) {
    dl::leave_critical_section();
    return false;
  }
  if (fstat(read_fd, &stat_buf) < 0) {
    close(read_fd);
    dl::leave_critical_section();
    return false;
  }

  if (!S_ISREG (stat_buf.st_mode)) {
    php_warning("Regular file expected as first argument in function copy, \"%s\" is given", from.c_str());
    close(read_fd);
    dl::leave_critical_section();
    return false;
  }

  int write_fd = open(to.c_str(), O_WRONLY | O_CREAT | O_TRUNC, stat_buf.st_mode);
  if (write_fd < 0) {
    close(read_fd);
    dl::leave_critical_section();
    return false;
  }

  size_t size = stat_buf.st_size;
  while (size > 0) {
    size_t len = min(size, (size_t)PHP_BUF_LEN);
    if (read_safe(read_fd, php_buf, len, from) < (ssize_t)len) {
      break;
    }
    if (write_safe(write_fd, php_buf, len, to) < (ssize_t)len) {
      break;
    }
    size -= len;
  }

  close(read_fd);
  close(write_fd);
  dl::leave_critical_section();
  return size == 0;
}

string f$dirname(const string &name) {
  string name_copy(name.c_str(), name.size());
  dl::enter_critical_section();//OK
  const char *result_c_str = dirname(name_copy.buffer());
  dl::leave_critical_section();
  return string(result_c_str);
}

Optional<array<string>> f$file(const string &name) {
  struct stat stat_buf;
  dl::enter_critical_section();//OK
  int file_fd = open_safe(name.c_str(), O_RDONLY);
  if (file_fd < 0) {
    dl::leave_critical_section();
    return false;
  }
  if (fstat(file_fd, &stat_buf) < 0) {
    close_safe(file_fd);
    dl::leave_critical_section();
    return false;
  }

  if (!S_ISREG (stat_buf.st_mode)) {
    php_warning("Regular file expected as first argument in function file, \"%s\" is given", name.c_str());
    close_safe(file_fd);
    dl::leave_critical_section();
    return false;
  }

  size_t size = stat_buf.st_size;
  if (size > string::max_size()) {
    php_warning("File \"%s\" is too large", name.c_str());
    close_safe(file_fd);
    dl::leave_critical_section();
    return false;
  }
  dl::leave_critical_section();

  string res(static_cast<string::size_type>(size), false);

  dl::enter_critical_section();//OK
  char *s = &res[0];
  if (read_safe(file_fd, s, size, name) < (ssize_t)size) {
    close_safe(file_fd);
    dl::leave_critical_section();
    return false;
  }

  close_safe(file_fd);
  dl::leave_critical_section();

  array<string> result;
  int prev = -1;
  for (int i = 0; i < (int)size; i++) {
    if (s[i] == '\n' || i + 1 == (int)size) {
      result.push_back(string(s + prev + 1, i - prev));
      prev = i;
    }
  }

  return result;
}

bool f$file_exists(const string &name) {
  dl::enter_critical_section();//OK
  bool result = (access(name.c_str(), F_OK) == 0);
  dl::leave_critical_section();
  return result;
}

namespace {

bool get_file_stat(const string &name, struct stat &out, const char *function) noexcept {
  dl::CriticalSectionGuard critical_section;
  int file_fd = open(name.c_str(), O_RDONLY);
  if (file_fd < 0) {
    return false;
  }
  if (fstat(file_fd, &out) < 0) {
    close(file_fd);
    return false;
  }

  if (!S_ISREG (out.st_mode)) {
    php_warning("Regular file expected as first argument in function %s, \"%s\" is given", function, name.c_str());
    close(file_fd);
    return false;
  }
  return true;
}

} // namespace

Optional<int64_t> f$filesize(const string &name) {
  struct stat stat_buf;
  return get_file_stat(name, stat_buf, __FUNCTION__)
         ? Optional<int64_t>{static_cast<int64_t>(stat_buf.st_size)}
         : Optional<int64_t>{false};
}

Optional<int64_t> f$filectime(const string &name) {
  struct stat stat_buf;
  return get_file_stat(name, stat_buf, __FUNCTION__)
         ? Optional<int64_t>{static_cast<int64_t>(stat_buf.st_ctime)}
         : Optional<int64_t>{false};
}

Optional<int64_t> f$filemtime(const string &name) {
  struct stat stat_buf;
  return get_file_stat(name, stat_buf, __FUNCTION__)
         ? Optional<int64_t>{static_cast<int64_t>(stat_buf.st_mtime)}
         : Optional<int64_t>{false};
}

bool f$is_dir(const string &name) {
  struct stat stat_buf;
  dl::enter_critical_section();//OK
  if (lstat(name.c_str(), &stat_buf) < 0) {
    dl::leave_critical_section();
    return false;
  }
  dl::leave_critical_section();

  return S_ISDIR (stat_buf.st_mode);
}

bool f$is_file(const string &name) {
  struct stat stat_buf;
  dl::enter_critical_section();//OK
  if (lstat(name.c_str(), &stat_buf) < 0) {
    dl::leave_critical_section();
    return false;
  }
  dl::leave_critical_section();

  return S_ISREG (stat_buf.st_mode);
}

bool f$is_readable(const string &name) {
  dl::enter_critical_section();//OK
  bool result = (access(name.c_str(), R_OK) == 0);
  dl::leave_critical_section();
  return result;
}

bool f$is_writeable(const string &name) {
  dl::enter_critical_section();//OK
  bool result = (access(name.c_str(), W_OK) == 0);
  dl::leave_critical_section();
  return result;
}

bool f$mkdir(const string &name, int64_t mode, bool recursive) {
  dl::enter_critical_section();//OK

  bool result;
  if (recursive) {
    result = mkdir_recursive(name.c_str(), static_cast<mode_t>(mode));
  } else {
    result = (mkdir(name.c_str(), static_cast<mode_t>(mode)) == 0);
  }

  if (!result) {
    php_warning("mkdir(%s): %s", name.c_str(), strerror(errno));
  }

  dl::leave_critical_section();
  return result;
}

string f$php_uname(const string &name) {
  utsname res;
  dl::enter_critical_section();//OK
  if (uname(&res)) {
    dl::leave_critical_section();
    return {};
  }
  dl::leave_critical_section();

  char mode = name[0];
  switch (mode) {
    case 's':
      return string(res.sysname);
    case 'n':
      return string(res.nodename);
    case 'r':
      return string(res.release);
    case 'v':
      return string(res.version);
    case 'm':
      return string(res.machine);
    default: {
      string result(res.sysname);
      result.push_back(' ');
      result.append(res.nodename);
      result.push_back(' ');
      result.append(res.release);
      result.push_back(' ');
      result.append(res.version);
      result.push_back(' ');
      result.append(res.machine);
      return result;
    }
  }
}

bool f$rename(const string &oldname, const string &newname) {
  dl::enter_critical_section();//OK
  bool result = (rename(oldname.c_str(), newname.c_str()) == 0);
  if (!result && errno == EXDEV) {
    result = f$copy(oldname, newname);
    if (result) {
      result = f$unlink(oldname);
      if (!result) {
        f$unlink(newname);
      }
    }
  }
  if (!result) {
    php_warning("Can't rename \"%s\" to \"%s\"", oldname.c_str(), newname.c_str());
  }
  dl::leave_critical_section();
  return result;
}

Optional<string> f$realpath(const string &path) {
  char real_path[PATH_MAX];

  dl::enter_critical_section();//OK
  bool result = (realpath(path.c_str(), real_path) != nullptr);
  dl::leave_critical_section();

  if (!result) {
    php_warning("Realpath is false on string \"%s\": %m", path.c_str());
    return false;
  }

  return string(real_path);
}

static Optional<string> full_realpath(const string &path) { // realpath resolving only dirname to work with unexisted files
  static char full_realpath_cache_storage[sizeof(array<string>)];
  static array<string> *full_realpath_cache = reinterpret_cast <array<string> *> (full_realpath_cache_storage);
  static long long full_realpath_last_query_num = -1;

  const auto offset = file_wrapper_name.size();
  string wrapped_path;
  if (strncmp(path.c_str(), file_wrapper_name.c_str(), offset)) {
    wrapped_path = file_wrapper_name;
    wrapped_path.append(path);
  } else {
    wrapped_path = path;
  }

  if (wrapped_path.size() == offset + 1 && wrapped_path[offset] == '/') {
    return wrapped_path;
  }

  if (dl::query_num != full_realpath_last_query_num) {
    new(full_realpath_cache_storage) array<string>();
    full_realpath_last_query_num = dl::query_num;
  }

  string &result_cache = (*full_realpath_cache)[wrapped_path];
  if (!result_cache.empty()) {
    if (result_cache.size() == 1) {
      return false;
    }

    php_assert (!strncmp(result_cache.c_str(), file_wrapper_name.c_str(), offset));
    return result_cache;
  }

  char real_path[PATH_MAX];
  string dirname_path_copy(wrapped_path.c_str() + offset, wrapped_path.size() - offset);

  dl::enter_critical_section();//OK
  const char *dirname_c_str = dirname(dirname_path_copy.buffer());
  bool result = (realpath(dirname_c_str, real_path) != nullptr);
  dl::leave_critical_section();

  if (result) {
    string basename_path_copy(wrapped_path.c_str() + offset, wrapped_path.size() - offset);

    dl::enter_critical_section();//OK
    const char *basename_c_str = __xpg_basename(basename_path_copy.buffer());
    dl::leave_critical_section();

    return result_cache = (static_SB.clean() << file_wrapper_name << real_path << '/' << basename_c_str).str();
  }
  result_cache = LETTER_a;
  return false;
}

Optional<string> f$tempnam(const string &dir, const string &prefix) {
  string prefix_new = f$basename(prefix);
  prefix_new.shrink(5);
  if (prefix_new.empty()) {
    prefix_new.assign("tmp.", 4);
  }

  string dir_new;
  Optional<string> dir_real;
  if (dir.empty() || !f$boolval(dir_real = f$realpath(dir))) {
    dl::enter_critical_section();//OK
    const char *s = getenv("TMPDIR");
    dl::leave_critical_section();
    if (s != nullptr && s[0] != 0) {
      int len = (int)strlen(s);

      if (s[len - 1] == '/') {
        len--;
      }

      dir_new.assign(s, len);
    } else if (P_tmpdir != nullptr) {
      dir_new.assign(P_tmpdir);
    } else {
      php_critical_error ("can't compute name of temporary directory in function tempnam");
      return false;
    }

    if (dir_new.empty()) {
      php_critical_error ("can't find directory for temporary file in function tempnam");
      return false;
    }

    dir_real = f$realpath(dir_new);
    if (!f$boolval(dir_real)) {
      php_critical_error ("wrong directory \"%s\" found in function tempnam", dir_new.c_str());
      return false;
    }
  }

  dir_new = dir_real.val();
  php_assert (!dir_new.empty());

  if (dir_new[dir_new.size() - 1] != '/' && prefix_new[0] != '/') {
    dir_new.append(1, '/');
  }

  dir_new.append(prefix_new);
  dir_new.append(6, 'X');

  dl::enter_critical_section();//OK
  int fd = mkstemp(dir_new.buffer());
  if (fd == -1 || close(fd)) {
    dl::leave_critical_section();
    php_warning("Can't create temporary file \"%s\" in function tempnam", dir_new.c_str());
    return false;
  }

  dl::leave_critical_section();
  return dir_new;
}

bool f$unlink(const string &name) {
  dl::enter_critical_section();//OK
  bool result = (unlink(name.c_str()) >= 0);
  dl::leave_critical_section();
  return result;
}

Optional<array<string>> f$scandir(const string &directory) {
  dirent **namelist;
  int namelist_size = scandir(directory.c_str(), &namelist, nullptr, alphasort);
  if (namelist_size < 0) {
    if (errno == ENOENT) {
      php_warning("failed to open dir: No such file or directory");
    } else if (errno == ENOTDIR) {
      php_warning("failed to open dir: Not a directory");
    } else if (errno == ENOMEM) {
      php_warning("failed to open dir: Not enough memory to complete the operation");
    }
    return false;
  }
  array<string> file_list(array_size(namelist_size, 0, true));
  for (int i = 0; i < namelist_size; i++) {
    char const *file_name = namelist[i]->d_name;
    string::size_type file_name_length = (string::size_type)strlen(file_name);
    file_list.set_value(i, string(file_name, file_name_length));
  }
  return file_list;
}

static char opened_files_storage[sizeof(array<FILE *>)];
static array<FILE *> *opened_files = reinterpret_cast <array<FILE *> *> (opened_files_storage);
static long long opened_files_last_query_num = -1;


static bool file_fclose(const Stream &stream);

static FILE *get_file(const Stream &stream) {
  Optional<string> filename_optional = full_realpath(stream.to_string());
  if (!f$boolval(filename_optional)) {
    php_warning("Wrong file \"%s\" specified", stream.to_string().c_str());
    return nullptr;
  }

  string filename = filename_optional.val();
  if (dl::query_num != opened_files_last_query_num || !opened_files->has_key(filename)) {
    php_warning("File \"%s\" is not opened", filename.c_str());
  }

  return opened_files->get_value(filename);
}

static Stream file_fopen(const string &filename, const string &mode) {
  if (dl::query_num != opened_files_last_query_num) {
    new(opened_files_storage) array<FILE *>();

    opened_files_last_query_num = dl::query_num;
  }

  Optional<string> real_filename_optional = full_realpath(filename);
  if (!f$boolval(real_filename_optional)) {
    php_warning("Wrong file \"%s\" specified", filename.c_str());
    return false;
  }
  string real_filename = real_filename_optional.val();
  if (opened_files->has_key(real_filename)) {
    php_warning("File \"%s\" already opened. Closing previous one", real_filename.c_str());
    file_fclose(real_filename);
  }

  dl::enter_critical_section();//NOT OK: opened_files
  FILE *file = fopen(real_filename.c_str() + file_wrapper_name.size(), mode.c_str());
  if (file == nullptr) {
    dl::leave_critical_section();
    return false;
  }

  opened_files->set_value(real_filename, file);
  dl::leave_critical_section();

  return real_filename;
}

static Optional<int64_t> file_fwrite(const Stream &stream, const string &text) {
  FILE *f = get_file(stream);
  if (f == nullptr) {
    return false;
  }

  if (text.empty()) {
    return 0;
  }

  dl::enter_critical_section();//OK
  const auto res = fwrite(text.c_str(), text.size(), 1, f);
  dl::leave_critical_section();

  if (res == 0) {
    return false;
  }
  php_assert (res == 1);
  return text.size();
}

static int64_t file_fseek(const Stream &stream, int64_t offset, int64_t whence) {
  const static int whences[3] = {SEEK_SET, SEEK_END, SEEK_CUR};
  if ((uint64_t)whence >= 3u) {
    php_warning("Wrong parameter whence in function fseek");
    return -1;
  }
  whence = whences[whence];

  FILE *f = get_file(stream);
  if (f == nullptr) {
    return -1;
  }

  dl::enter_critical_section();//OK
  int res = fseek(f, offset, static_cast<int32_t>(whence));
  dl::leave_critical_section();
  return res;
}

static Optional<int64_t> file_ftell(const Stream &stream) {
  FILE *f = get_file(stream);
  if (f == nullptr) {
    return false;
  }

  dl::enter_critical_section();//OK
  int64_t result = ftell(f);
  dl::leave_critical_section();
  return result;
}

static Optional<string> file_fread(const Stream &stream, int64_t length) {
  if (length <= 0) {
    php_warning("Parameter length in function fread must be positive");
    return false;
  }
  if (length > string::max_size()) {
    php_warning("Parameter length in function fread is too large");
    return false;
  }

  FILE *f = get_file(stream);
  if (f == nullptr) {
    return false;
  }

  string res(static_cast<string::size_type>(length), false);
  dl::enter_critical_section();//OK
  clearerr(f);
  size_t res_size = fread(&res[0], 1, static_cast<size_t>(length), f);
  if (ferror(f)) {
    dl::leave_critical_section();
    php_warning("Error happened during fread from file \"%s\"", stream.to_string().c_str());
    return false;
  }
  dl::leave_critical_section();

  res.shrink(static_cast<string::size_type>(res_size));
  return res;
}

static Optional<string> file_fgetc(const Stream &stream) {
  FILE *f = get_file(stream);
  if (f == nullptr) {
    return false;
  }

  dl::enter_critical_section();//OK
  clearerr(f);
  int result = fgetc(f);
  if (ferror(f)) {
    dl::leave_critical_section();
    php_warning("Error happened during fgetc from file \"%s\"", stream.to_string().c_str());
    return false;
  }
  dl::leave_critical_section();
  if (result == EOF) {
    return false;
  }

  return string(1, static_cast<char>(result));
}

static Optional<string> file_fgets(const Stream &stream, int64_t length) {
  FILE *f = get_file(stream);
  if (f == nullptr) {
    return false;
  }

  if (length < 0) {
    struct stat st{};
    int fd = fileno(f);
    fstat(fd, &st);
    if (st.st_size > 0) {
      length = st.st_size + 1;
    } else {
      return false;
    }
  }
  if (length > string::max_size()) {
    php_warning("Parameter length in function fgetc is too large");
    return false;
  }
  string res(static_cast<string::size_type>(length), false);
  dl::enter_critical_section();//OK
  clearerr(f);
  char *result = fgets(&res[0], static_cast<int32_t>(length), f);
  if (ferror(f)) {
    dl::leave_critical_section();
    php_warning("Error happened during fgets from file \"%s\"", stream.to_string().c_str());
    return false;
  }
  dl::leave_critical_section();
  if (result == nullptr) {
    return false;
  }

  res.shrink(static_cast<string::size_type>(strlen(res.c_str())));
  return res;
}

static Optional<int64_t> file_fpassthru(const Stream &stream) {
  FILE *f = get_file(stream);
  if (f == nullptr) {
    return false;
  }

  int64_t result = 0;

  dl::enter_critical_section();//OK
  while (!feof(f)) {
    clearerr(f);
    size_t res_size = fread(&php_buf[0], 1, PHP_BUF_LEN, f);
    if (ferror(f)) {
      dl::leave_critical_section();
      php_warning("Error happened during fpassthru from file \"%s\"", stream.to_string().c_str());
      return false;
    }
    print(php_buf, res_size);
    result += static_cast<int64_t>(res_size);
  }
  dl::leave_critical_section();
  return result;
}

static bool file_fflush(const Stream &stream) {
  FILE *f = get_file(stream);
  if (f != nullptr) {
    dl::enter_critical_section();//OK
    fflush(f);
    dl::leave_critical_section();
    return true;
  }
  return false;
}

static bool file_feof(const Stream &stream) {
  FILE *f = get_file(stream);
  if (f != nullptr) {
    dl::enter_critical_section();//OK
    bool eof = (feof(f) != 0);
    dl::leave_critical_section();
    return eof;
  }
  return true;
}

static bool file_fclose(const Stream &stream) {
  Optional<string> filename_optional = full_realpath(stream.to_string());
  if (!f$boolval(filename_optional)) {
    php_warning("Wrong file \"%s\" specified", stream.to_string().c_str());
    return false;
  }
  string filename = filename_optional.val();
  if (dl::query_num == opened_files_last_query_num && opened_files->has_key(filename)) {
    dl::enter_critical_section();//NOT OK: opened_files
    int result = fclose(opened_files->get_value(filename));
    opened_files->unset(filename);
    dl::leave_critical_section();
    return (result == 0);
  }
  return false;
}


Optional<string> file_file_get_contents(const string &name) {
  auto offset = file_wrapper_name.size();
  if (strncmp(name.c_str(), file_wrapper_name.c_str(), offset)) {
    offset = 0;
  }

  struct stat stat_buf{};
#if ASAN_ENABLED && __GNUC__ == 10 && __GNUC_MINOR__ == 3
  ASAN_UNPOISON_MEMORY_REGION(&stat_buf, sizeof(stat_buf));
#endif
  dl::enter_critical_section();//OK
  int file_fd = open_safe(name.c_str() + offset, O_RDONLY);
  if (file_fd < 0) {
    dl::leave_critical_section();
    return false;
  }
  if (fstat(file_fd, &stat_buf) < 0) {
    close_safe(file_fd);
    dl::leave_critical_section();
    return false;
  }

  if (!S_ISREG (stat_buf.st_mode)) {
    php_warning("Regular file expected as first argument in function file_get_contents, \"%s\" is given", name.c_str());
    close_safe(file_fd);
    dl::leave_critical_section();
    return false;
  }

  size_t size = stat_buf.st_size;
  if (size > string::max_size()) {
    php_warning("File \"%s\" is too large to get its content", name.c_str());
    close_safe(file_fd);
    dl::leave_critical_section();
    return false;
  }
  dl::leave_critical_section();

  string res(static_cast<string::size_type>(size), false);

  dl::enter_critical_section();//OK
  if (read_safe(file_fd, &res[0], size, name) < (ssize_t)size) {
    close_safe(file_fd);
    dl::leave_critical_section();
    return false;
  }

  close_safe(file_fd);
  dl::leave_critical_section();
  return res;
}

static Optional<int64_t> file_file_put_contents(const string &name, const string &content, int64_t flags) {
  auto offset = file_wrapper_name.size();
  if (strncmp(name.c_str(), file_wrapper_name.c_str(), offset)) {
    offset = 0;
  }

  dl::enter_critical_section();//OK
  int append_flag = (flags & FILE_APPEND) ? O_APPEND : O_TRUNC;
  int file_fd = open(name.c_str() + offset, O_WRONLY | O_CREAT | append_flag, 0644);
  if (file_fd < 0) {
    php_warning("Can't open file \"%s\"", name.c_str());
    dl::leave_critical_section();
    return false;
  }

  if (write_safe(file_fd, content.c_str(), content.size(), name) < (ssize_t)content.size()) {
    close(file_fd);
    unlink(name.c_str());
    dl::leave_critical_section();
    return false;
  }

  close(file_fd);
  dl::leave_critical_section();
  return content.size();
}


void global_init_files_lib() {
  static stream_functions file_stream_functions;

  file_stream_functions.name = string("file", 4);
  file_stream_functions.fopen = file_fopen;
  file_stream_functions.fwrite = file_fwrite;
  file_stream_functions.fseek = file_fseek;
  file_stream_functions.ftell = file_ftell;
  file_stream_functions.fread = file_fread;
  file_stream_functions.fgetc = file_fgetc;
  file_stream_functions.fgets = file_fgets;
  file_stream_functions.fpassthru = file_fpassthru;
  file_stream_functions.fflush = file_fflush;
  file_stream_functions.feof = file_feof;
  file_stream_functions.fclose = file_fclose;

  file_stream_functions.file_get_contents = file_file_get_contents;
  file_stream_functions.file_put_contents = file_file_put_contents;

  file_stream_functions.stream_socket_client = nullptr;
  file_stream_functions.context_set_option = nullptr;
  file_stream_functions.stream_set_option = nullptr;
  file_stream_functions.get_fd = nullptr;

  register_stream_functions(&file_stream_functions, true);
}

void free_files_lib() {
  dl::enter_critical_section();//OK
  if (dl::query_num == opened_files_last_query_num) {
    const array<FILE *> *const_opened_files = opened_files;
    for (array<FILE *>::const_iterator p = const_opened_files->begin(); p != const_opened_files->end(); ++p) {
      fclose(p.get_value());
    }
    opened_files_last_query_num--;
  }

  if (opened_fd != -1) {
    close_safe(opened_fd);
  }
  opened_fd = -1;
  dl::leave_critical_section();
}

