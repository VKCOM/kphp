#include <memory>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "common/algorithms/string-algorithms.h"
#include "common/options.h"
#include "common/server/signals.h"
#include "common/version-string.h"
#include "common/wrappers/string_view.h"

#include "compiler/compiler.h"
#include "compiler/compiler-settings.h"

namespace {

class OptionParser : vk::not_copyable {
public:
  static OptionParser &get() noexcept {
    static OptionParser parser;
    return parser;
  }

  template<class T>
  void add(const char *description, KphpOption<T> &option,
           const char *long_option, const char *env,
           std::string default_value = {}, std::vector<std::string> choices = {}) noexcept {
    add(description, option, 0, long_option, env, std::move(default_value), std::move(choices));
  }

  template<class T>
  void add(const char *description, KphpOption<T> &option,
           char short_option, const char *long_option, const char *env,
           std::string default_value = {}, std::vector<std::string> choices = {}) noexcept {
    assert(envs_.insert(vk::string_view{env}).second);
    std::string default_str;
    if (!default_value.empty()) {
      default_str = " (default: " + default_value + ")";
    }
    std::string choices_str;
    if (!choices.empty()) {
      choices_str = " {choices: " + vk::join(choices, ", ") + "}";
    }

    int32_t option_id = next_option_id_++;
    parse_option(long_option, std::is_same<T, bool>{} ? no_argument : required_argument, option_id,
                 "[%s] %s%s%s.", env, description, choices_str.c_str(), default_str.c_str());
    if (short_option) {
      parse_option_alias(long_option, short_option);
    }
    KphpRawOption &raw_option = option;
    raw_option.init(long_option, short_option, env, default_value, std::move(choices));
    options_[option_id] = &raw_option;
  }

  int32_t parse(int32_t option_id, const char *optarg_value) noexcept {
    auto it = options_.find(option_id);
    if (it == options_.end()) {
      return -1;
    }
    it->second->set_option_arg_value(optarg_value);
    return 0;
  }

  void finalize() {
    for (auto &raw_option : options_) {
      for (auto &option_for_substitute : options_) {
        raw_option.second->substitute_depends(*option_for_substitute.second);
      }
      raw_option.second->verify_arg_value();
      raw_option.second->parse_arg_value();
    }
    options_.clear();
    envs_.clear();
  }

private:
  OptionParser() = default;

  std::unordered_map<int32_t, KphpRawOption *> options_;
  std::unordered_set<vk::string_view> envs_;

  int32_t next_option_id_{2001};
};

} // namespace

int32_t parse_args_f(int32_t i) {
  switch (i) {
    case 'h':
      usage_and_exit();
    case 2000:
      printf("%s\n", get_version_string());
      exit(0);
    default:
      return OptionParser::get().parse(i, optarg);
  }
}

int main(int argc, char *argv[]) {
  init_version_string("kphp2cpp");
  usage_set_other_args_desc("<main-files-list>");
  set_debug_handlers();

  const uint32_t system_threads = std::max(std::thread::hardware_concurrency(), 1U);
  const std::string home_dir = CompilerSettings::get_home();

  auto settings = std::make_unique<CompilerSettings>();
  auto &parser = OptionParser::get();

  remove_all_options();
  parse_option("help", no_argument, 'h', "Print help and exit.");
  parse_option("version", no_argument, 2000, "Print version and exit.");

  parser.add(
    "Verbosity", settings->verbosity,
    'v', "verbosity", "KPHP_VERBOSITY", "0", {"0", "1", "2", "3"});
  parser.add(
    "Path to kphp source", settings->kphp_src_path,
    's', "source-path", "KPHP_PATH", home_dir + "kphp/");
  parser.add(
    "Internal file with the list of supported PHP functions", settings->functions_file,
    'f', "functions-file", "KPHP_FUNCTIONS", "${KPHP_PATH}/functions.txt");
  parser.add(
    "File with kphp runtime sha256 hash", settings->runtime_sha256_file,
    "runtime-sha256", "KPHP_RUNTIME_SHA256", "${KPHP_PATH}/objs/php_lib_version.sha256");
  parser.add(
    "The output binary type: server, cli or lib", settings->mode,
    'M', "mode", "KPHP_MODE", "server", {"server", "cli", "lib"});
  parser.add(
    "A runtime library for building the output binary", settings->link_file,
    'l', "link-with", "KPHP_LINK_FILE", "${KPHP_PATH}/objs/libphp-main-${KPHP_MODE}.a");
  parser.add(
    "Directory where php files will be searched", settings->includes,
    'I', "include-dir", "KPHP_INCLUDE_DIR");
  parser.add(
    "Destination directory", settings->dest_dir,
    'd', "destination-directory", "KPHP_DEST_DIR", "${KPHP_PATH}/tests/kphp_tmp/default/");
  parser.add(
    "Path for the output binary", settings->user_binary_path,
    'o', "output-file", "KPHP_USER_BINARY_PATH");
  parser.add(
    "Directory for placing out static lib and header. Compatible only with lib mode", settings->static_lib_out_dir,
    'O', "output-lib-dir", "KPHP_OUT_LIB_DIR");
  parser.add(
    "Force make. Old object files and binary will be removed", settings->force_make,
    'F', "force-make", "KPHP_FORCE_MAKE");
  parser.add(
    "Make the output binary", settings->use_make,
    'm', "make", "KPHP_USE_MAKE");
  parser.add(
    "Processes number for the compilation", settings->jobs_count,
    'j', "jobs-num", "KPHP_JOBS_COUNT", std::to_string(system_threads));
  parser.add(
    "Threads number for the transpilation", settings->threads_count,
    't', "threads-count", "KPHP_THREADS_COUNT", std::to_string(system_threads * 2));
  parser.add(
    "Builtin tl schema. Incompatible with lib mode", settings->tl_schema_file,
    'T', "tl-schema", "KPHP_TL_SCHEMA");
  parser.add(
    "Generate storers and fetchers for internal tl functions", settings->gen_tl_internals,
    "gen-tl-internals", "KPHP_GEN_TL_INTERNALS");
  parser.add(
    "All compile time warnings will be errors", settings->error_on_warns,
    'W', "Werror", "KPHP_ERROR_ON_WARNINGS");
  parser.add(
    "Print all warnings to file, otherwise warnings are printed to stderr", settings->warnings_file,
    "warnings-file", "KPHP_WARNINGS_FILE");
  parser.add(
    "Warnings level: prints more warnings, according to level set", settings->warnings_level,
    "warnings-level", "KPHP_WARNINGS_LEVEL", "0", {"0", "1", "2"});
  parser.add(
    "Show all type errors", settings->show_all_type_errors,
    "show-all-type-errors", "KPHP_SHOW_ALL_TYPE_ERRORS"
  );
  parser.add(
    "Colorize warnings output: yes, no, auto", settings->colorize,
    "colorize", "KPHP_COLORS", "auto", {"auto", "yes", "no"});
  parser.add(
    "Save C++ compiler statistics to file", settings->stats_file,
    "stats-file", "KPHP_STATS_FILE");
  parser.add(
    "Save transpilation metrics to file", settings->compilation_metrics_file,
    "compilation-metrics-file", "KPHP_COMPILATION_METRICS_FILE");
  parser.add(
    "Override kphp version string", settings->override_kphp_version,
    "kphp-version-override", "KPHP_VERSION_OVERRIDE");
  parser.add(
    "Specify the compiled php code version", settings->php_code_version,
    "php-code-version", "KPHP_PHP_CODE_VERSION", "unknown");
  parser.add(
    "C++ compiler for building the output binary", settings->cxx,
    "cxx", "KPHP_CXX", "g++");
  parser.add(
    "Extra C++ compiler flags for building the output binary", settings->extra_cxx_flags,
    "extra-cxx-flags", "KPHP_EXTRA_CXXFLAGS", "-Os -ggdb -march=core2 -mfpmath=sse -mssse3");
  parser.add(
    "Extra linker flags for building the output binary", settings->extra_ld_flags,
    "extra-linker-flags", "KPHP_EXTRA_LDFLAGS", "-ggdb");
  parser.add(
    "C++ compiler debug level for building the output binary", settings->debug_level,
    "debug-level", "KPHP_DEBUG_LEVEL");
  parser.add(
    "Archive creator for building the output binary", settings->archive_creator,
    "archive-creator", "KPHP_ARCHIVE_CREATOR", "ar");
  parser.add(
    "Use dynamic incremental linkage for building the output binary", settings->dynamic_incremental_linkage,
    "dynamic-incremental-linkage", "KPHP_DYNAMIC_INCREMENTAL_LINKAGE");
  parser.add(
    "Profile functions: 0 - disabled, 1 - enabled for marked functions, 2 - enabled for all", settings->profiler_level,
    'g', "profiler", "KPHP_PROFILER", "0", {"0", "1", "2"});
  parser.add(
    "Enable an ability to get global vars memory stats", settings->enable_global_vars_memory_stats,
    "enable-global-vars-memory-stats", "KPHP_ENABLE_GLOBAL_VARS_MEMORY_STATS");
  parser.add(
    "Print graph of resumable calls to stderr", settings->print_resumable_graph,
    'p', "print-graph", "KPHP_PRINT_RESUMABLE_GRAPH");
  parser.add(
    "Forbid to use the precompile header", settings->no_pch,
    "no-pch", "KPHP_NO_PCH");
  parser.add(
    "Forbid to use the index file", settings->no_index_file,
    "no-index-file", "KPHP_NO_INDEX_FILE");
  parser.add(
    "Hide transpilation progress", settings->hide_progress,
    "hide-progress", "KPHP_HIDE_PROGRESS");

  parse_engine_options_long(argc, argv, parse_args_f);

  if (optind >= argc) {
    usage_and_exit();
    return 2;
  }

  for (; optind < argc; ++optind) {
    settings->add_main_file(argv[optind]);
  }

  try {
    parser.finalize();
    settings->init();
  } catch (const std::exception &ex) {
    printf("%s\n", ex.what());
    return 1;
  }

  if (settings->verbosity.get() >= 3) {
    settings->debug();
  }

  if (!compiler_execute(settings.release())) {
    return 1;
  }

  return 0;
}
