#include "compiler/make/make-runner.h"

#include <wait.h>

#include "drinkless/dl-utils-lite.h"

#include "compiler/compiler-core.h"
#include "compiler/utils/string-utils.h"

void MakeRunner::run_target(Target *target) {
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

void MakeRunner::ready_target(Target *target) {
  //fprintf (stderr, "ready target %s\n", target->get_name().c_str());
  assert (!target->is_ready);

  targets_left--;
  target->is_ready = true;
  for (auto const rdep : target->rdeps) {
    one_dep_ready_target(rdep);
  }
}

void MakeRunner::one_dep_ready_target(Target *target) {
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

void MakeRunner::wait_target(Target *target) {
  assert (!target->is_waiting);
  target->is_waiting = true;
  targets_waiting++;
}

void MakeRunner::register_target(Target *target, vector<Target *> &&deps) {
  for (auto const dep : deps) {
    dep->rdeps.push_back(target);
    target->pending_deps++;
  }
  target->deps = std::move(deps);

  targets_left++;
  all_targets.push_back(target);
}

void MakeRunner::require_target(Target *target) {
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

bool MakeRunner::start_job(Target *target) {
  target->start_time = dl_time();
  string cmd = target->get_cmd();

  int pid = run_cmd(cmd);
  if (pid < 0) {
    return false;
  }
  jobs[pid] = target;
  return true;
}

bool MakeRunner::finish_job(int pid, int return_code, int by_signal) {
  auto it = jobs.find(pid);
  assert (it != jobs.end());
  Target *target = it->second;
  if (G->env().get_stats_file() != nullptr) {
    double passed = dl_time() - target->start_time;
    fmt_fprintf(G->env().get_stats_file(), "{}s {}\n", passed, target->get_name());
  }
  jobs.erase(it);
  if (return_code != 0) {
    if (!fail_flag) {
      fmt_fprintf(stdout, "pid [{}] failed or terminated : ", pid);
      if (return_code != -1) {
        fmt_fprintf(stdout, "return code {}\n", return_code);
      } else if (by_signal != -1) {
        fmt_fprintf(stdout, "killed by signal {}\n", by_signal);
      } else {
        fmt_fprintf(stdout, "killed by unknown reason\n");
      }
      fmt_fprintf(stdout, "Failed [{}]\n", target->get_cmd());
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

void MakeRunner::on_fail() {
  if (fail_flag) {
    return;
  }
  fmt_fprintf(stdout, "Make failed. Waiting for {} children\n", (int)jobs.size());
  fail_flag = true;
  for (const auto &pid_and_target : jobs) {
    int err = kill(pid_and_target.first, SIGINT);
    if (err < 0) {
      perror("kill failed: ");
    }
  }
}

int MakeRunner::signal_flag;

void MakeRunner::sigint_handler(int sig __attribute__((unused))) {
  signal_flag = 1;
}

bool MakeRunner::make_target(Target *target, int jobs_count) {
  if (jobs_count < 1) {
    fmt_print("Invalid jobs_count [{}]\n", jobs_count);
    return false;
  }
  signal_flag = 0;
  fail_flag = false;
  dl_signal(SIGINT, MakeRunner::sigint_handler);
  dl_signal(SIGTERM, MakeRunner::sigint_handler);

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
      fmt_fprintf(stderr, "{:3}% [total jobs {}] [left jobs {}] [running jobs {}] [waiting jobs {}]\n",
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
            fmt_print("Something strange happened with pid [{}]\n", pid);
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

MakeRunner::~MakeRunner() {
  //TODO: delete targets
  for (auto target : all_targets) {
    delete target;
  }
}
