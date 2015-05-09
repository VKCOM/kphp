#include <pwd.h>
#include "compiler/compiler.h"
#include "compiler/enviroment.h"
#include "version-string.h"

/***
 * Kitten compiler for PHP interface
 **/
void usage (void) {
  printf ( "%s\nConvert php code into C++ code, and compile it into a binary\n"
          "-a\tUse safe integer arithmetic\n"
          "-b <directory>\tBase directory. Use it when compiling the same code from different directories\n"
          "-d <directory>\tDestination directory\n"
          "-F\tForce make. Old object files and binary will be removed\n"
          "-f <file>\tInternal file with library headers and e.t.c. Equals to $KPHP_FUNCTIONS. $KPHP_PATH/PHP/functions.txt is used by default\n"
          "-g\tGenerate slower code, but with profiling\n"
          "-i <file>\tExperimental. Index for faster compilations\n"
          "-I <directory>\tDirectory where php files will be searched\n"
          "-j <jobs_count>\tSpecifies the number of jobs (commands) to run simultaneously by make. By default equals to 1\n"
          "-l <file>\tLink with <file>. Equals to $KPHP_LINK_FILE. $KPHP_PATH/objs/PHP/$KPHP_LINK_FILE_NAME is used by default\n"
          "-M <mode>\tserver, net or cli. If <mode> == server/net, than $KPHP_LINK_FILE_NAME=php-server.a. If <mode> == cli, than $KPHP_LINK_FILE_NAME=php-cli.a\n"
          "-m\tRun make\n"
          "-o <file>\tPlace output into <file>\n"
          "-p\tPrint graph of resumbale calls to stderr\n"
          "-r\tSplit output into multiple directories\n"
          "-t <thread_count>\tUse <threads_count> threads. By default equals to 16\n"
          "-s <directory>\tPath to kphp source. Equals to $KPHP_PATH. ~/engine/src is used by default\n"
          "-V <file>\t<file> will be use as kphp library version. Equals to $KPHP_LIB_VERSION. $KPHP_PATH/objs/PHP/php_lib_version.o is used by default\n"
          "-v\tVerbosity\n"
          "-W\tAll compile time warnings will be errors\n",
          FullVersionStr);
}

/*
   KPHP_PATH ?= "~/engine/src"
   KPHP_FUNCTIONS ?= "$KPHP_PATH/PHP/functions.txt"
   KPHP_LIB_VERSION ?= "$KPHP_PATH/objs/PHP/php_lib_version.o"
   KPHP_MODE ?= "server";
   ---KPHP_LINK_FILE_NAME = $KPHP_MODE == "cli" ? "php-cli.a" : "php-server.a"
   KPHP_LINK_FILE ?= $KPHP_PATH/objs/PHP/$KPHP_LINK_FILE_NAME
 */

static void runtime_handler (const int sig) {
  fprintf (stderr, "%s caught, terminating program\n", sig == SIGSEGV ? "SIGSEGV" : "SIGABRT");
  dl_print_backtrace();
  dl_print_backtrace_gdb();
  _exit (EXIT_FAILURE);
}

static void set_debug_handlers (void) {
  dl_signal (SIGSEGV, runtime_handler);
  dl_signal (SIGABRT, runtime_handler);
}

int main (int argc, char *argv[]) {
  init_version_string("kphp2cpp");
  set_debug_handlers();
  int i;

  KphpEnviroment *env = new KphpEnviroment();
  int verbosity = 0;
  struct passwd *user_pwd = getpwuid (getuid());
  dl_passert (user_pwd != NULL, "Failed to get user name");
  char *user = user_pwd->pw_name;
  if (user != NULL && (!strcmp (user, "levlam") || !strcmp (user, "arseny30"))) {
    verbosity = 3;
    env->inc_verbosity();
    env->inc_verbosity();
    env->inc_verbosity();
  }

  //TODO: long options
  while ((i = getopt (argc, argv, "ab:d:Ff:gi:I:j:l:M:mo:prt:Ss:V:vW")) != -1) {
    switch (i) {
    case 'a':
      env->set_use_safe_integer_arithmetic ("1");
      break;
    case 'b':
      env->set_base_dir (optarg);
      break;
    case 'd':
      env->set_dest_dir (optarg);
      break;
    case 'F':
      env->set_make_force ("1");
      break;
    case 'f':
      env->set_functions (optarg);
      break;
    case 'g':
      env->set_enable_profiler ();
      break;
    case 'i':
      env->set_index (optarg);
      break;
    case 'I':
      env->add_include (optarg);
      break;
    case 'j':
      env->set_jobs_count (optarg);
      break;
    case 'M':
      env->set_mode (optarg);
      break;
    case 'l':
      env->set_link_file (optarg);
      break;
    case 'm':
      env->set_use_make ("1");
      break;
    case 'r':
      env->set_use_subdirs ("1");
      break;
    case 'o':
      env->set_user_binary_path (optarg);
      break;
    case 'p':
      env->set_print_resumable_graph ();
      break;
    case 't':
      env->set_threads_count (optarg);
      break;
    case 's':
      env->set_path (optarg);
      break;
    case 'S':
      env->set_use_auto_dest ("1");
      break;
    case 'V':
      env->set_lib_version (optarg);
      break;
    case 'v':
      verbosity++;
      env->inc_verbosity();
      break;
    case 'W':
      env->set_error_on_warns();
      break;
    default:
      printf ("Unknown option '%c'", (char)i);
      usage();
      exit (1);
    }
  }


  if (optind >= argc) {
    usage();
    return 2;
  }

  while (optind < argc) {
    env->add_main_file (argv[optind]);
    optind++;
  }

  bool ok = env->init();
  if (env->get_verbosity() >= 3) {
    env->debug();
  }
  if (!ok) {
    return 1;
  }
  compiler_execute (env);

  return 0;
}
