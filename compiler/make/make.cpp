#include "compiler/make/make.h"

#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unordered_map>

#include "common/wrappers/mkdir_recursive.h"

#include "compiler/compiler-core.h"
#include "compiler/make/cpp-to-obj-target.h"
#include "compiler/make/file-target.h"
#include "compiler/make/objs-to-bin-target.h"
#include "compiler/make/objs-to-obj-target.h"
#include "compiler/make/objs-to-static-lib-target.h"
#include "compiler/utils/string-utils.h"

/*** Make ***/
void Make::run_target(Target *target) {
  //todo: multiple threads
  //fprintf (stderr, "run target %s\n", target->get_name().c_str());
  bool ready = target->mtime != 0;
  if (ready) {
    for (auto const dep : target->deps) {
      if (dep->mtime > target->mtime) {
        ready = false;
        break;
      }
    }
  }

  if (!ready) {
    target->compute_priority();
    pending_jobs.push(target);
  } else {
    ready_target(target);
  }
}

void Make::ready_target(Target *target) {
  //fprintf (stderr, "ready target %s\n", target->get_name().c_str());
  assert (!target->is_ready);

  targets_left--;
  target->is_ready = true;
  for (auto const rdep : target->rdeps) {
    one_dep_ready_target(rdep);
  }
}

void Make::one_dep_ready_target(Target *target) {
  //fprintf (stderr, "one_dep_ready target %s\n", target->get_name().c_str());
  target->pending_deps--;
  assert (target->pending_deps >= 0);
  if (target->pending_deps == 0) {
    if (target->is_waiting) {
      target->is_waiting = false;
      targets_waiting--;
      run_target(target);
    }
  }
}

void Make::wait_target(Target *target) {
  assert (!target->is_waiting);
  target->is_waiting = true;
  targets_waiting++;
}

void Make::register_target(Target *target, const vector<Target *> &deps) {
  for (auto const dep : deps) {
    dep->rdeps.push_back(target);
    target->pending_deps++;
  }
  target->deps = deps;

  targets_left++;
  all_targets.push_back(target);
}

void Make::require_target(Target *target) {
  if (!target->require()) {
    return;
  }

  if (target->pending_deps == 0) {
    run_target(target);
  } else {
    wait_target(target);
    for (auto const dep : target->deps) {
      require_target(dep);
    }
  }
}

static int run_cmd(const string &cmd) {
  //fprintf (stdout, "%s\n", cmd.c_str());
  vector<string> args = split(cmd);
  vector<char *> argv(args.size() + 1);
  for (int i = 0; i < (int)args.size(); i++) {
    argv[i] = (char *)args[i].c_str();
  }
  argv.back() = nullptr;

  int pid = vfork();
  if (pid < 0) {
    perror("vfork failed: ");
    return pid;
  }

  if (pid == 0) {
    //prctl (PR_SET_PDEATHSIG, SIGKILL);
    execvp(argv[0], &argv[0]);
    perror("execvp failed: ");
    _exit(1);
  } else {
    return pid;
  }
}

bool Make::start_job(Target *target) {
  target->start_time = dl_time();
  string cmd = target->get_cmd();

  int pid = run_cmd(cmd);
  if (pid < 0) {
    return false;
  }
  jobs[pid] = target;
  return true;
}

bool Make::finish_job(int pid, int return_code, int by_signal) {
  map<int, Target *>::iterator it = jobs.find(pid);
  assert (it != jobs.end());
  Target *target = it->second;
  if (G->env().get_stats_file() != nullptr) {
    double passed = dl_time() - target->start_time;
    fprintf(G->env().get_stats_file(), "%lfs %s\n", passed, target->get_name().c_str());
  }
  jobs.erase(it);
  if (return_code != 0) {
    if (!fail_flag) {
      fprintf(stdout, "pid [%d] failed or terminated : ", pid);
      if (return_code != -1) {
        fprintf(stdout, "return code %d\n", return_code);
      } else if (by_signal != -1) {
        fprintf(stdout, "killed by signal %d\n", by_signal);
      } else {
        fprintf(stdout, "killed by unknown reason\n");
      }
      fprintf(stdout, "Failed [%s]\n", target->get_cmd().c_str());
    }
    target->after_run_fail();
    return false;
  }
  if (!target->after_run_success()) {
    return false;
  }
  ready_target(target);
  return true;
}

void Make::on_fail() {
  if (fail_flag) {
    return;
  }
  fprintf(stdout, "Make failed. Waiting for %d children\n", (int)jobs.size());
  fail_flag = true;
  for (const auto &pid_and_target : jobs) {
    int err = kill(pid_and_target.first, SIGINT);
    if (err < 0) {
      perror("kill failed: ");
    }
  }
}

int Make::signal_flag;

void Make::sigint_handler(int sig __attribute__((unused))) {
  signal_flag = 1;
}

bool Make::make_target(Target *target, int jobs_count) {
  if (jobs_count < 1) {
    printf("Invalid jobs_count [%d]\n", jobs_count);
    return false;
  }
  signal_flag = 0;
  fail_flag = false;
  dl_signal(SIGINT, Make::sigint_handler);
  dl_signal(SIGTERM, Make::sigint_handler);

  //fprintf (stderr, "make target: %s\n", target->get_name().c_str());
  //TODO: check timeouts
  require_target(target);

  int total_jobs = targets_left;
  int old_perc = -1;
  enum {
    wait_jobs_st,
    start_jobs_st
  } state = start_jobs_st;
  while (true) {
    int perc = (total_jobs - targets_left) * 100 / std::max(1, total_jobs);
    if (old_perc != perc) {
      fprintf(stderr, "%3d%% [total jobs %d] [left jobs %d] [running jobs %d] [waiting jobs %d]\n",
              perc, total_jobs, targets_left, (int)jobs.size(), targets_waiting);
      old_perc = perc;
    }
    if (jobs.empty() && (fail_flag || pending_jobs.empty())) {
      break;
    }

    if (signal_flag != 0) {
      on_fail();
      signal_flag = 0;
    }

    switch (state) {
      case start_jobs_st: {
        if (fail_flag || pending_jobs.empty() || (int)jobs.size() >= jobs_count) {
          state = wait_jobs_st;
          break;
        }

        Target *target = pending_jobs.top();
        pending_jobs.pop();
        if (!start_job(target)) {
          on_fail();
        }
        break;
      }
      case wait_jobs_st: {
        if (jobs.empty()) {
          state = start_jobs_st;
          break;
        }
        int status;
        int pid = wait(&status);
        if (pid == -1) {
          if (errno != EINTR) {
            perror("waitpid failed: ");
            on_fail();
          }
        } else {
          int return_code = -1;
          int by_signal = -1;
          if (WIFEXITED (status)) {
            return_code = WEXITSTATUS (status);
          } else if (WIFSIGNALED (status)) {
            by_signal = WTERMSIG (status);
          } else {
            printf("Something strange happened with pid [%d]\n", pid);
          }
          if (!finish_job(pid, return_code, by_signal)) {
            on_fail();
          }
        }
        if (!fail_flag) {
          state = start_jobs_st;
        }
        break;
      }
    }
  }

  //TODO: use old handlers instead SIG_DFL
  dl_signal(SIGINT, SIG_DFL);
  dl_signal(SIGTERM, SIG_DFL);
  return !fail_flag && target->is_ready;
}

Make::Make() :
  targets_waiting(0),
  targets_left(0),
  fail_flag(false) {}

Make::~Make() {
  //TODO: delete targets
  for (auto target : all_targets) {
    delete target;
  }
}


/*** KphpMake ***/

void KphpMake::target_set_file(KphpTarget *target, File *file) {
  assert (file->target == nullptr);
  target->set_file(file);
  file->target = target;
}

void KphpMake::target_set_env(KphpTarget *target) {
  target->set_env(&env);
}

KphpTarget *KphpMake::to_target(File *file) {
  if (file->target != nullptr) {
    return dynamic_cast <KphpTarget *> (file->target);
  }
  return create_cpp_target(file);
}

vector<Target *> KphpMake::to_targets(File *file) {
  return {to_target(file)};
}

vector<Target *> KphpMake::to_targets(vector<File *> files) {
  vector<Target *> res(files.size());
  for (int i = 0; i < (int)files.size(); i++) {
    res[i] = to_target(files[i]);
  }
  return res;
}

KphpTarget *KphpMake::create_cpp_target(File *cpp) {
  KphpTarget *res = new FileTarget();
  target_set_file(res, cpp);
  target_set_env(res);
  make.register_target(res);
  return res;
}

KphpTarget *KphpMake::create_cpp2obj_target(File *cpp, File *obj) {
  KphpTarget *res = new Cpp2ObjTarget();
  target_set_file(res, obj);
  target_set_env(res);
  make.register_target(res, to_targets(cpp));
  return res;
}

KphpTarget *KphpMake::create_objs2obj_target(const vector<File *> &objs, File *obj) {
  KphpTarget *res = new Objs2ObjTarget();
  target_set_file(res, obj);
  target_set_env(res);
  make.register_target(res, to_targets(objs));
  return res;
}

KphpTarget *KphpMake::create_objs2bin_target(const vector<File *> &objs, File *bin) {
  KphpTarget *res = new Objs2BinTarget();
  target_set_file(res, bin);
  target_set_env(res);
  make.register_target(res, to_targets(objs));
  return res;
}

KphpTarget *KphpMake::create_objs2static_lib_target(const vector<File *> &objs, File *static_lib) {
  KphpTarget *res = new Objs2StaticLibTarget();
  target_set_file(res, static_lib);
  target_set_env(res);
  make.register_target(res, to_targets(objs));
  return res;
}

void KphpMake::init_env(const KphpEnviroment &kphp_env) {
  env.cxx = kphp_env.get_cxx();
  env.cxx_flags = kphp_env.get_cxx_flags();
  env.ld = kphp_env.get_ld();
  env.ld_flags = kphp_env.get_ld_flags();
  env.ar = kphp_env.get_ar();
  env.debug_level = kphp_env.get_debug_level();
}

bool KphpMake::make_target(File *bin, int jobs_count) {
  return make.make_target(to_target(bin), jobs_count);
}

static void hard_link_or_copy_impl(const std::string &from, const std::string &to, bool replace, bool allow_copy) noexcept {
  if (!link(from.c_str(), to.c_str())) {
    return;
  }

  if (errno == EEXIST) {
    if (replace) {
      kphp_error_return(!unlink(to.c_str()),
                        format("Can't remove file '%s': %s", to.c_str(), strerror(errno)));
      hard_link_or_copy_impl(from, to, false, allow_copy);
    }
    return;
  }

  // the file is placed on other device, or we have a permission problems, try to copy it
  if (vk::any_of_equal(errno, EXDEV, EPERM) && allow_copy) {
    struct stat file_stat;
    kphp_error_return (!stat(from.c_str(), &file_stat),
                       format("Can't get file stat '%s': %s", from.c_str(), strerror(errno)));

    std::string tmp_file = to + ".XXXXXX";
    int tmp_fd = mkstemp(&tmp_file[0]);
    kphp_error_return(tmp_fd != -1,
                      format("Can't create tmp file '%s': %s", tmp_file.c_str(), strerror(errno)));
    kphp_error_return(fchmod(tmp_fd, file_stat.st_mode) != -1,
                      format("Can't change permissions of tmp file '%s': %s", tmp_file.c_str(), strerror(errno)));
    int from_fd = open(from.c_str(), O_RDONLY);
    const ssize_t s = sendfile(tmp_fd, from_fd, nullptr, file_stat.st_size);
    close(from_fd);
    close(tmp_fd);
    kphp_error_return(s != -1, format("Can't copy file from '%s' to '%s': %s", from.c_str(), tmp_file.c_str(), strerror(errno)));
    kphp_assert(s == file_stat.st_size);
    hard_link_or_copy_impl(tmp_file, to, replace, false);
    unlink(tmp_file.c_str());
    return;
  }

  kphp_error(0, format("Can't copy file from '%s' to '%s': %s", from.c_str(), to.c_str(), strerror(errno)));
}

static void hard_link_or_copy(const std::string &from, const std::string &to, bool replace = true) {
  hard_link_or_copy_impl(from, to, replace, true);
  stage::die_if_global_errors();
}

static void copy_static_lib_to_out_dir(File &&static_archive) {
  Index out_dir;
  out_dir.set_dir(G->env().get_static_lib_out_dir());
  out_dir.del_extra_files();

  // copy static archive
  LibData out_lib(G->env().get_static_lib_name(), out_dir.get_dir());
  hard_link_or_copy(static_archive.path, out_lib.static_archive_path());
  static_archive.unlink();

  // copy functions.txt of this static archive
  File functions_txt_tmp(G->env().get_dest_cpp_dir() + LibData::functions_txt_tmp_file());
  hard_link_or_copy(functions_txt_tmp.path, out_lib.functions_txt_file());
  static_archive.unlink();

  // copy runtime lib sha256 of this static archive
  File runtime_lib_sha256(G->env().get_runtime_sha256_file());
  hard_link_or_copy(runtime_lib_sha256.path, out_lib.runtime_lib_sha256_file());

  Index headers_tmp_dir;
  headers_tmp_dir.sync_with_dir(G->env().get_dest_cpp_dir() + LibData::headers_tmp_dir());
  Index out_headers_dir;
  out_headers_dir.set_dir(out_lib.headers_dir());
  // copy cpp header files of this static archive
  for (File *header_file: headers_tmp_dir.get_files()) {
    hard_link_or_copy(header_file->path, out_headers_dir.get_dir() + header_file->name);
    static_archive.unlink();
  }
}

static std::forward_list<File> collect_imported_libs() {
  const string &binary_runtime_sha256 = G->env().get_runtime_sha256();
  stage::die_if_global_errors();

  std::forward_list<File> imported_libs;
  imported_libs.emplace_front(G->env().get_link_file());
  for (const auto &lib: G->get_libs()) {
    if (lib && !lib->is_raw_php()) {
      std::string lib_runtime_sha256 = KphpEnviroment::read_runtime_sha256_file(lib->runtime_lib_sha256_file());

      kphp_error_act(binary_runtime_sha256 == lib_runtime_sha256,
                     format("Mismatching between sha256 of binary runtime '%s' and lib runtime '%s'",
                            G->env().get_runtime_sha256_file().c_str(), lib->runtime_lib_sha256_file().c_str()),
                     continue);
      imported_libs.emplace_front(lib->static_archive_path());
    }
  }

  for (File &file: imported_libs) {
    kphp_error_act(file.upd_mtime() > 0, format("Can't read mtime of link_file [%s]", file.path.c_str()), continue);
    if (G->env().get_verbosity() >= 1) {
      fprintf(stderr, "Use static lib [%s]\n", file.path.c_str());
    }
  }

  stage::die_if_global_errors();
  return imported_libs;
}

static long long get_imported_header_mtime(const std::string &header_path, const std::forward_list<Index> &imported_headers) {
  for (const Index &lib_headers_dir: imported_headers) {
    if (File *header = lib_headers_dir.get_file(header_path)) {
      return header->mtime;
    }
  }
  kphp_error(false, format("Can't file lib header file '%s'", header_path.c_str()));
  return 0;
}

static std::string kphp_make_precompiled_header(Index *obj_dir, const KphpEnviroment &kphp_env) {
  std::string gch_dir = "/tmp/kphp_gch/";
  gch_dir.append(kphp_env.get_runtime_sha256()).append(1, '/');
  gch_dir.append(kphp_env.get_cxx_flags_sha256()).append(1, '/');

  const std::string header_filename = "php_functions.h";
  const std::string gch_filename = header_filename + ".gch";
  const std::string gch_path = gch_dir + gch_filename;
  if (access(gch_path.c_str(), F_OK) != -1) {
    return gch_dir;
  }

  KphpMake make;
  File php_functions_h(kphp_env.get_path() + "PHP/" + header_filename);
  kphp_error_act(php_functions_h.upd_mtime() > 0,
                 format("Can't read mtime of '%s'", php_functions_h.path.c_str()),
                 return {});

  File *php_functions_h_gch = obj_dir->insert_file(kphp_env.get_dest_objs_dir() + gch_filename);
  make.create_cpp2obj_target(&php_functions_h, php_functions_h_gch);
  File sha256_version_file(kphp_env.get_runtime_sha256_file());
  kphp_assert(sha256_version_file.upd_mtime() > 0);
  php_functions_h.target->force_changed(sha256_version_file.mtime);
  make.init_env(kphp_env);
  if (!make.make_target(php_functions_h_gch, 1)) {
    return {};
  }

  const mode_t old_mask = umask(0);
  const bool dir_created = mkdir_recursive(gch_dir.c_str(), 0777);
  umask(old_mask);
  kphp_error_act(dir_created,
                 format("Can't create tmp dir '%s': %s", gch_dir.c_str(), strerror(errno)),
                 return {});

  hard_link_or_copy(php_functions_h.path, gch_dir + header_filename, false);
  hard_link_or_copy(php_functions_h_gch->path, gch_path, false);
  php_functions_h_gch->unlink();
  return gch_dir;
}

static std::unordered_map<File *, long long> create_dep_mtime(const Index &cpp_dir, const std::forward_list<Index> &imported_headers) {
  std::unordered_map<File *, long long> dep_mtime;
  std::priority_queue<std::pair<long long, File *>> mtime_queue;
  std::unordered_map<File *, std::vector<File *>> reverse_includes;

  std::vector<File *> files = cpp_dir.get_files();

  auto lib_version_it = std::find_if(files.begin(), files.end(), [](File *file) { return file->name == "_lib_version.h"; });
  kphp_assert(lib_version_it != files.end());
  File *lib_version = *lib_version_it;

  for (const auto &file : files) {
    for (const auto &include : file->includes) {
      File *header = cpp_dir.get_file(include);
      kphp_assert (header != nullptr);
      kphp_assert (header->on_disk);
      reverse_includes[header].push_back(file);
    }

    long long max_mtime = std::max(file->mtime, lib_version->mtime);
    for (const auto &lib_include : file->lib_includes) {
      max_mtime = std::max(max_mtime, get_imported_header_mtime(lib_include, imported_headers));
    }

    dep_mtime[file] = max_mtime;
    mtime_queue.emplace(max_mtime, file);
  }

  while (!mtime_queue.empty()) {
    long long mtime;
    File *file;
    std::tie(mtime, file) = mtime_queue.top();
    mtime_queue.pop();

    if (dep_mtime[file] != mtime) {
      continue;
    }

    auto file_includes_it = reverse_includes.find(file);
    if (file_includes_it == reverse_includes.end()) {
      continue;
    }

    for (auto including_file: file_includes_it->second) {
      auto &including_file_mtime = dep_mtime[including_file];
      if (including_file_mtime < mtime) {
        including_file_mtime = mtime;
        mtime_queue.emplace(including_file_mtime, including_file);
      }
    }
  }
  return dep_mtime;
}

static std::vector<File *> create_obj_files(KphpMake *make, Index &obj_dir, const Index &cpp_dir,
                                     const std::forward_list<Index> &imported_headers) {
  std::vector<File *> files = cpp_dir.get_files();
  std::unordered_map<File *, long long> dep_mtime = create_dep_mtime(cpp_dir, imported_headers);

  std::vector<File *> objs;
  for (auto cpp_file : files) {
    if (cpp_file->ext == ".cpp") {
      File *obj_file = obj_dir.insert_file(cpp_file->name_without_ext + ".o");
      obj_file->compile_with_debug_info_flag = cpp_file->compile_with_debug_info_flag;
      make->create_cpp2obj_target(cpp_file, obj_file);
      Target *cpp_target = cpp_file->target;
      cpp_target->force_changed(dep_mtime[cpp_file]);
      objs.push_back(obj_file);
    }
  }
  fprintf(stderr, "objs cnt = %zu\n", objs.size());

  std::map<string, vector<File *>> subdirs;
  std::vector<File *> tmp_objs;
  for (auto obj_file : objs) {
    std::string name = obj_file->subdir;
    if (!name.empty()) {
      name.pop_back();
    }
    if (name.empty()) {
      tmp_objs.push_back(obj_file);
      continue;
    }
    name += ".o";
    subdirs[name].push_back(obj_file);
  }

  objs = std::move(tmp_objs);
  for (const auto &name_and_files : subdirs) {
    File *obj_file = obj_dir.insert_file(name_and_files.first);
    make->create_objs2obj_target(name_and_files.second, obj_file);
    objs.push_back(obj_file);
  }
  fprintf(stderr, "objs cnt = %zu\n", objs.size());
  return objs;
}

static bool kphp_make(File &bin, Index &obj_dir, const Index &cpp_dir, std::forward_list<File> imported_libs,
               const std::forward_list<Index> &imported_headers, const KphpEnviroment &kphp_env,
               const std::string &gch_dir) {
  KphpMake make;
  std::vector<File *> lib_objs;
  for (File &link_file: imported_libs) {
    make.create_cpp_target(&link_file);
    lib_objs.emplace_back(&link_file);
  }
  std::vector<File *> objs = create_obj_files(&make, obj_dir, cpp_dir, imported_headers);
  std::copy(lib_objs.begin(), lib_objs.end(), std::back_inserter(objs));
  make.create_objs2bin_target(objs, &bin);
  make.init_env(kphp_env);
  if (!gch_dir.empty()) {
    make.add_gch_dir(gch_dir);
  }
  return make.make_target(&bin, kphp_env.get_jobs_count());
}

static bool kphp_make_static_lib(File &static_lib, Index &obj_dir, const Index &cpp_dir,
                          const std::forward_list<Index> &imported_headers, const KphpEnviroment &kphp_env,
                          const std::string &gch_dir) {
  KphpMake make;
  std::vector<File *> objs = create_obj_files(&make, obj_dir, cpp_dir, imported_headers);
  make.create_objs2static_lib_target(objs, &static_lib);
  make.init_env(kphp_env);
  if (!gch_dir.empty()) {
    make.add_gch_dir(gch_dir);
  }
  return make.make_target(&static_lib, kphp_env.get_jobs_count());
}

static std::forward_list<Index> collect_imported_headers() {
  std::forward_list<Index> imported_headers;
  for (const auto &lib: G->get_libs()) {
    if (lib && !lib->is_raw_php()) {
      imported_headers.emplace_front();
      imported_headers.front().sync_with_dir(lib->lib_dir());
    }
  }
  return imported_headers;
}

void run_make() {
  AutoProfiler profiler{get_profiler("make")};
  stage::set_name("Make");
  G->del_extra_files();

  auto env = G->env();

  Index obj_index;
  obj_index.sync_with_dir(env.get_dest_objs_dir());

  File bin_file(env.get_binary_path());
  kphp_assert (bin_file.upd_mtime() >= 0);

  if (env.get_make_force()) {
    obj_index.del_extra_files();
    bin_file.unlink();
  }

  std::string gch_dir;
  bool ok = true;
  const bool pch_allowed = !env.get_no_pch();
  if (pch_allowed) {
    gch_dir = kphp_make_precompiled_header(&obj_index, env);
    ok = !gch_dir.empty();
    kphp_error (ok, "Make precompiled header failed");
  }
  if (ok) {
    auto lib_header_dirs = collect_imported_headers();
    ok = env.is_static_lib_mode()
         ? kphp_make_static_lib(bin_file, obj_index, G->get_index(), lib_header_dirs, env, gch_dir)
         : kphp_make(bin_file, obj_index, G->get_index(), collect_imported_libs(), lib_header_dirs, env, gch_dir);
    kphp_error (ok, "Make failed");
  }

  stage::die_if_global_errors();
  obj_index.del_extra_files();

  if (!env.get_user_binary_path().empty()) {
    hard_link_or_copy(bin_file.path, env.get_user_binary_path());
  }
  if (env.is_static_lib_mode()) {
    copy_static_lib_to_out_dir(std::move(bin_file));
  }
}
