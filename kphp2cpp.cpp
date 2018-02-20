#include <pwd.h>

#include "compiler/compiler.h"
#include "compiler/enviroment.h"
#include "common/server-functions.h"
#include "common/version-string.h"

/***
 * Kitten compiler for PHP interface
 **/

extern "C" {

void usage(void) {
  printf("%s\nConvert php code into C++ code, and compile it into a binary\n",
         get_version_string());
  parse_usage();
  exit(1);
}

}

/*
   KPHP_PATH ?= "~/engine/src"
   KPHP_FUNCTIONS ?= "$KPHP_PATH/PHP/functions.txt"
   KPHP_LIB_VERSION ?= "$KPHP_PATH/objs/PHP/php_lib_version.o"
   KPHP_MODE ?= "server";
   ---KPHP_LINK_FILE_NAME = $KPHP_MODE == "cli" ? "php-cli.a" : "php-server.a"
   KPHP_LINK_FILE ?= $KPHP_PATH/objs/PHP/$KPHP_LINK_FILE_NAME
 */

static KphpEnviroment *env;

int parse_args_f(int i) {
  switch (i) {
  case 'h':
    usage();
    exit(2);
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
  case 'T':
    env->set_tl_schema_file (optarg);
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
    env->inc_verbosity();
    break;
  case 'W':
    env->set_error_on_warns();
    break;
  case 2000:
    env->set_warnings_filename (optarg);
    break;
  case 2001:
    env->set_stats_filename (optarg);
    break;
  case 2002: {
    int level = atoi (optarg);
    if (level < 0) {
      return -1;
    }
    env->set_warnings_level (level);
    break;
  }
  case 2003:
    printf("%s\n", get_version_string());
    exit(0);
    break;
  case 2004:
    env->set_debug_level(optarg);
    break;
  default:
    return -1;
  }
  return 0;
}

int main (int argc, char *argv[]) {
  init_version_string("kphp2cpp");
  set_debug_handlers();

  env = new KphpEnviroment();
  struct passwd *user_pwd = getpwuid (getuid());
  dl_passert (user_pwd != NULL, "Failed to get user name");
  char *user = user_pwd->pw_name;
  if (user != NULL && (!strcmp (user, "levlam") || !strcmp (user, "arseny30") || !strcmp (user, "pkunyavskiy"))) {
    env->inc_verbosity();
    env->inc_verbosity();
    env->inc_verbosity();
  }

  remove_all_options();
  parse_option("help", no_argument, 'h', "prints help and exits");
  parse_option("safe-arithmetic", no_argument, 'a', "Use safe integer arithmetic");
  parse_option("base-directiory", required_argument, 'b', "Base directory. Use it when compiling the same code from different directories");
  parse_option("destination-directory", required_argument, 'd', "Destination directory");
  parse_option("force-make", no_argument, 'F', "Force make. Old object files and binary will be removed");
  parse_option("functions-file", required_argument, 'f', "Internal file with library headers and e.t.c. Equals to $KPHP_FUNCTIONS. $KPHP_PATH/PHP/functions.txt is used by default");
  parse_option("profiler", no_argument, 'g', "Generate slower code, but with profiling");
  parse_option("index-file", required_argument, 'i', "Experimental. Index for faster compilations");
  parse_option("include-dir", required_argument, 'I', "Directory where php files will be searched");
  parse_option("jobs-num", required_argument, 'j', "Specifies the number of jobs (commands) to run simultaneously by make. By default equals to 1");
  parse_option("link-with", required_argument, 'l', "Link with <file>. Equals to $KPHP_LINK_FILE. $KPHP_PATH/objs/PHP/$KPHP_LINK_FILE_NAME is used by default");
  parse_option("mode", required_argument, 'M', "server, net or cli. If <mode> == server/net, than $KPHP_LINK_FILE_NAME=php-server.a. If <mode> == cli, than $KPHP_LINK_FILE_NAME=php-cli.a");
  parse_option("make", no_argument, 'm', "Run make");
  parse_option("output-file", required_argument, 'o', "Place output into <file>");
  parse_option("print-graph", no_argument, 'p', "Print graph of resumable calls to stderr");
  parse_option("incremental-link", no_argument, 'r', "Split output into multiple directories");
  parse_option("threads-count", required_argument, 't', "Use <threads_count> threads. By default equals to 16");
  parse_option("tl-schema", required_argument, 'T', "Add builtin tl schema to kphp binary");
  parse_option("auto-dest-dir", no_argument, 'S', "Automatic destination dir");
  parse_option("source-path", required_argument, 's', "Path to kphp source. Equals to $KPHP_PATH. ~/engine/src is used by default");
  parse_option("version-file", required_argument, 'V', "<file> will be use as kphp library version. Equals to $KPHP_LIB_VERSION. $KPHP_PATH/objs/PHP/php_lib_version.o is used by default");
  parse_option("verbosity", no_argument, 'v', "Verbosity");
  parse_option("Werror", no_argument, 'W', "All compile time warnings will be errors");
  parse_option("warnings-file", required_argument, 2000, "Print all warnings to <file>, otherwise warnings are printed to stderr");
  parse_option("stats-file", required_argument, 2001, "Print some statistics to <file>");
  parse_option("warnings-level", required_argument, 2002, "Sets warnings level to <level>: prints more warnings, according to level set (Default value: 0)");
  parse_option("version", no_argument, 2003, "prints version and exits");
  parse_option("debug-level", required_argument, 2004, "Sets debug level to <level> but excluding autogenerated, useless for debug, files");
  parse_engine_options_long (argc, argv, parse_args_f);


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
  if (!compiler_execute (env)) {
    return 1;
  }

  return 0;
}
