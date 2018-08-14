#include "compiler/make.h"

#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "compiler/compiler-core.h"
#include "compiler/utils.h"

/*** Target ***/
//TODO: review mtime
void Target::set_mtime (long long new_mtime) {
  assert (mtime == 0);
  mtime = new_mtime;
}
bool Target::upd_mtime (long long new_mtime) {
  if (new_mtime < mtime) {
    fprintf (stdout, "Trying to decrease mtime\n");
    return false;
  }
  mtime = new_mtime;
  return true;
}
Target::Target() :
  mtime (0),
  pending_deps (0),
  is_required (false),
  is_waiting (false),
  is_ready (false) {
  }
Target::~Target() {
}
void Target::compute_priority() {
  priority = 0;
}
bool Target::after_run_success() {
  return true;
}
bool Target::require() {
  if (is_required) {
    return false;
  }
  is_required = true;
  on_require();
  return true;
}
void Target::force_changed (long long new_mtime) {
  if (mtime < new_mtime) {
    mtime = new_mtime;
  }
}
string Target::target() {
  return get_name();
}
string Target::dep_list() {
  stringstream ss;
  for (auto const dep : deps) {
    ss << dep->get_name() << " ";
  }
  return ss.str();
}

/*** Make ***/
void Make::run_target (Target *target) {
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
    pending_jobs.push (target);
  } else {
    ready_target (target);
  }
}
void Make::ready_target (Target *target) {
  //fprintf (stderr, "ready target %s\n", target->get_name().c_str());
  assert (!target->is_ready);

  targets_left--;
  target->is_ready = true;
  for (auto const rdep : target->rdeps) {
    one_dep_ready_target (rdep);
  }
}
void Make::one_dep_ready_target (Target *target) {
  //fprintf (stderr, "one_dep_ready target %s\n", target->get_name().c_str());
  target->pending_deps--;
  assert (target->pending_deps >= 0);
  if (target->pending_deps == 0) {
    if (target->is_waiting) {
      target->is_waiting = false;
      targets_waiting--;
      run_target (target);
    }
  }
}
void Make::wait_target (Target *target) {
  assert (!target->is_waiting);
  target->is_waiting = true;
  targets_waiting++;
}
void Make::register_target (Target *target, const vector <Target *> &deps) {
  for (auto const dep : deps) {
    dep->rdeps.push_back (target);
    target->pending_deps++;
  }
  target->deps = deps;

  targets_left++;
  all_targets.push_back (target);
}
void Make::require_target (Target *target) {
  if (!target->require()) {
    return;
  }

  if (target->pending_deps == 0) {
    run_target (target);
  } else {
    wait_target (target);
    for (auto const dep : target->deps) {
      require_target (dep);
    }
  }
}

static int run_cmd (const string &cmd) {
  //fprintf (stdout, "%s\n", cmd.c_str());
  vector <string> args = split(cmd);
  vector <char *> argv (args.size() + 1);
  for (int i = 0; i < (int)args.size(); i++) {
    argv[i] = (char *)args[i].c_str();
  }
  argv.back() = NULL;

  int pid = vfork();
  if (pid < 0) {
    perror ("vfork failed: ");
    return pid;
  }

  if (pid == 0) {
    //prctl (PR_SET_PDEATHSIG, SIGKILL);
    execvp (argv[0], &argv[0]);
    perror ("execvp failed: ");
    _exit (1);
  } else {
    return pid;
  }
}

bool Make::start_job (Target *target) {
  target->start_time = dl_time();
  string cmd = target->get_cmd();

  int pid = run_cmd (cmd);
  if (pid < 0) {
    return false;
  }
  jobs[pid] = target;
  return true;
}

bool Make::finish_job (int pid, int return_code, int by_signal) {
  map <int, Target*>::iterator it = jobs.find (pid);
  assert (it != jobs.end());
  Target *target = it->second;
  if (G->env().get_stats_file() != NULL) {
    double passed = dl_time() - target->start_time;
    fprintf (G->env().get_stats_file(), "%lfs %s\n", passed, target->get_name().c_str());
  }
  jobs.erase (it);
  if (return_code != 0) {
    if (!fail_flag) {
      fprintf (stdout, "pid [%d] failed or terminated : ", pid);
      if (return_code != -1) {
        fprintf (stdout, "return code %d\n", return_code);
      } else if (by_signal != -1) {
        fprintf (stdout, "killed by signal %d\n", by_signal);
      } else {
        fprintf (stdout, "killed by unknown reason\n");
      }
      fprintf (stdout, "Failed [%s]\n", target->get_cmd().c_str());
    }
    target->after_run_fail();
    return false;
  }
  if (!target->after_run_success()) {
    return false;
  }
  ready_target (target);
  return true;
}

void Make::on_fail() {
  if (fail_flag) {
    return;
  }
  fprintf (stdout, "Make failed. Waiting for %d children\n", (int)jobs.size());
  fail_flag = true;
  for (const auto &pid_and_target : jobs) {
    int err = kill (pid_and_target.first, SIGINT);
    if (err < 0) {
      perror ("kill failed: ");
    }
  }
}

int Make::signal_flag;
void Make::sigint_handler (int sig __attribute__((unused))) {
  signal_flag = 1;
}

bool Make::make_target (Target *target, int jobs_count) {
  if (jobs_count < 1) {
    printf ("Invalid jobs_count [%d]\n", jobs_count);
    return false;
  }
  signal_flag = 0;
  fail_flag = false;
  dl_signal (SIGINT, Make::sigint_handler);
  dl_signal (SIGTERM, Make::sigint_handler);

  //fprintf (stderr, "make target: %s\n", target->get_name().c_str());
  //TODO: check timeouts
  require_target (target);

  int total_jobs = targets_left;
  int old_perc = -1;
  enum {wait_jobs_st, start_jobs_st} state = start_jobs_st;
  while (true) {
    int perc = (total_jobs - targets_left) * 100 / max (1, total_jobs);
    if (old_perc != perc) {
      fprintf (stderr, "%3d%% [total jobs %d] [left jobs %d] [running jobs %d] [waiting jobs %d]\n",
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
        if (!start_job (target)) {
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
        int pid = wait (&status);
        if (pid == -1) {
          if (errno != EINTR) {
            perror ("waitpid failed: ");
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
            printf ("Something strange happened with pid [%d]\n", pid);
          }
          if (!finish_job (pid, return_code, by_signal)) {
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
  dl_signal (SIGINT, SIG_DFL);
  dl_signal (SIGTERM, SIG_DFL);
  return !fail_flag && target->is_ready;
}
Make::Make() :
  targets_waiting (0),
  targets_left (0),
  fail_flag(false)
{}
Make::~Make() {
  //TODO: delete targets
  for (auto target : all_targets) {
    delete target;
  }
}

/*** Enviroment ***/
const string &KphpMakeEnv::get_cxx() const {
  return cxx;
}

const string &KphpMakeEnv::get_cxx_flags() const {
  return cxx_flags;
}

const string &KphpMakeEnv::get_ld_flags() const {
  return ld_flags;
}

const string &KphpMakeEnv::get_debug_level() const {
  return debug_level;
}

/*** KphpTarget ***/
KphpTarget::KphpTarget() :
  Target(),
  file (NULL),
  env (NULL) {
}
string KphpTarget::get_name() {
  return file->path;
}
void KphpTarget::on_require() {
  file->needed = true;
}
bool KphpTarget::after_run_success() {
  long long mtime = file->upd_mtime();
  if (mtime < 0) {
    return false;
  } else if (mtime == 0) {
    fprintf (stdout, "Failed to generate target [%s]\n", file->path.c_str());
    return false;
  }
  return upd_mtime (file->mtime);
}
void KphpTarget::after_run_fail() {
  file->unlink();
}
void KphpTarget::set_file (File *new_file) {
  assert (file == NULL);
  file = new_file;
  set_mtime (file->mtime);
}

File * KphpTarget::get_file () const{
  return file;
}

void KphpTarget::set_env (KphpMakeEnv *new_env) {
  assert (env == NULL);
  env = new_env;
}
/*** KphpMake ***/
string FileTarget::get_cmd() {
  assert (0);
  return "";
}
void Cpp2ObjTarget::compute_priority() {
  struct stat st;
  priority = 0;
  if (stat (get_name().c_str(), &st) == 0) {
    priority = st.st_size;
  }
}

string Cpp2ObjTarget::get_cmd() {
  std::stringstream ss;
  //ss << "echo " << target();
  ss << env->get_cxx() << 
    " -c -o " << target() <<
    " " << dep_list() << 
    " " << env->get_cxx_flags();

  if (get_file()->compile_with_debug_info_flag) {
    ss << " " << env->get_debug_level();
  }

  return ss.str();
}
string Objs2ObjTarget::get_cmd() {
  std::stringstream ss;
  ss << "ld" <<
    " -r" <<
    " -o" << target() <<
    " " << dep_list();
  return ss.str();
}
string Objs2BinTarget::get_cmd() {
  std::stringstream ss;
  ss << env->get_cxx() << 
    " -o " << target() <<
    " -Wl,--whole-archive " << dep_list() << " -Wl,--no-whole-archive "
    " " << env->get_ld_flags();
  return ss.str();
}

void KphpMake::target_set_file (KphpTarget *target, File *file) {
  assert (file->target == NULL);
  target->set_file (file);
  file->target = target;
}
void KphpMake::target_set_env (KphpTarget *target) {
  target->set_env (&env);
}
KphpTarget *KphpMake::to_target (File *file) {
  if (file->target != NULL) {
    return dynamic_cast <KphpTarget *> (file->target);
  }
  return create_cpp_target (file);
}
vector <Target *> KphpMake::to_targets (File *file) {
  return {to_target(file)};
}
vector <Target *> KphpMake::to_targets (vector <File *> files) {
  vector <Target *> res (files.size());
  for (int i = 0; i < (int)files.size(); i++) {
    res[i] = to_target (files[i]);
  }
  return res;
}

KphpTarget *KphpMake::create_cpp_target (File *cpp) {
  KphpTarget *res = new FileTarget();
  target_set_file (res, cpp);
  target_set_env (res);
  make.register_target (res);
  return res;
}
KphpTarget *KphpMake::create_cpp2obj_target (File *cpp, File *obj) {
  KphpTarget *res = new Cpp2ObjTarget();
  target_set_file (res, obj);
  target_set_env (res);
  make.register_target (res, to_targets (cpp));
  return res;
}
KphpTarget *KphpMake::create_objs2obj_target (const vector <File *> &objs, File *obj) {
  KphpTarget *res = new Objs2ObjTarget();
  target_set_file (res, obj);
  target_set_env (res);
  make.register_target (res, to_targets (objs));
  return res;
}
KphpTarget *KphpMake::create_objs2bin_target (const vector <File *> &objs, File *bin) {
  KphpTarget *res = new Objs2BinTarget();
  target_set_file (res, bin);
  target_set_env (res);
  make.register_target (res, to_targets (objs));
  return res;
}
void KphpMake::init_env (const KphpEnviroment &kphp_env) {
  env.cxx = kphp_env.get_cxx();
  env.cxx_flags = kphp_env.get_cxx_flags();
  env.ld_flags = kphp_env.get_ld_flags();
  env.debug_level = kphp_env.get_debug_level();
}
bool KphpMake::make_target (File *bin, int jobs_count) {
  return make.make_target (to_target (bin), jobs_count);
}

