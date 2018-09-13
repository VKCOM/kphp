#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

void usage() {
  fprintf(stderr, "Very simple analog of gnu time\n");
}

double dl_get_utime(int clock_id) {
  struct timespec T;
#if _POSIX_TIMERS
  assert (clock_gettime (clock_id, &T) >= 0);
  return (double)T.tv_sec + (double)T.tv_nsec * 1e-9;
#else
#error "No high-precision clock"
  return (double)time();
#endif
}

double dl_time(void) {
  return dl_get_utime(CLOCK_MONOTONIC);
}

int main(int argc, char **argv) {
  const char *output_file = NULL;
  FILE *output = stderr;
  int i;
  while ((i = getopt(argc, argv, "+ho:")) != -1) {
    switch (i) {
      case 'o':
        output_file = optarg;
        break;
      default:
        printf("Unknown option '%c'", (char)i);
        /* fallthrough */
      case 'h':
        usage();
        _exit(1);
    }
  }
  if (output_file != NULL) {
    output = fopen(output_file, "w");
    if (output == NULL) {
      fprintf(stderr, "Failed to open [%.100s]\n", output_file);
      _exit(1);
    }
  }
  if (optind == argc) {
    usage();
    _exit(1);
  }

  char **cmd = &argv[optind];
  double st = dl_time();
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork failed");
    _exit(1);
  }
  if (pid == 0) {
    execvp(cmd[0], cmd);
    perror("execvp failed");
    _exit(1);
  }
  int status;
  struct rusage rusage;
  pid_t waited_pid = wait3(&status, 0, &rusage);
  double en = dl_time();
  assert(waited_pid == pid);


  /*double user_time = rusage.ru_utime.tv_sec + rusage.ru_utime.tv_usec / 1000000.0;*/
  /*double sys_time = rusage.ru_stime.tv_sec + rusage.ru_stime.tv_usec / 1000000.0;*/
  /*double total_time = user_time + sys_time;*/
  double real_time = en - st;
  long rss = rusage.ru_maxrss;
  fprintf(output, "%.4lf %ld\n", real_time, rss);

  if (WIFSTOPPED(status)) {
    exit(WSTOPSIG(status));
  } else if (WIFSIGNALED(status)) {
    exit(WTERMSIG(status));
  } else if (WIFEXITED(status)) {
    exit(WEXITSTATUS(status));
  }
  return 0;
}
