// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <iostream>
#include <memory>
#include <unordered_set>

#include "common/algorithms/string-algorithms.h"
#include "common/options.h"
#include "common/sanitizer.h"
#include "common/server/limits.h"
#include "common/server/signals.h"
#include "common/version-string.h"
#include "common/wrappers/string_view.h"

#include "compiler/compiler.h"
#include "compiler/compiler-settings.h"
#include "compiler/threading/tls.h"
#include "compiler/utils/string-utils.h"

namespace {

class OptionParser : vk::not_copyable {
public:
  static OptionParser &get_instance() noexcept {
    static OptionParser parser;
    return parser;
  }

  static void add_default_options() noexcept {
    remove_all_options();
    parse_option("help", no_argument, 'h', "Print help and exit.");
    parse_option("version", no_argument, version_and_first_option_id_, "Print version and exit.");
  }

  void add_other_args(const char *description, KphpRawOption &raw_option) {
    usage_set_other_args_desc(description);
    other_options_ = &raw_option;
    other_options_description_ = description;
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

    const auto option_id = static_cast<int32_t>(version_and_first_option_id_ + options_.size() + 1);
    parse_option(long_option, std::is_same<T, bool>{} ? no_argument : required_argument, option_id,
                 "[%s] %s%s%s.", env, description, choices_str.c_str(), default_str.c_str());
    if (short_option) {
      parse_option_alias(long_option, short_option);
    }
    KphpRawOption &raw_option = option;
    raw_option.init(env, std::move(default_value), std::move(choices));
    options_.emplace_back(&raw_option);
  }

  void add_implicit_option(vk::string_view description, KphpImplicitOption &implicit_option) {
    implicit_options_.emplace_back(description, &implicit_option);
  }

  void process_args(int32_t argc, char **argv) {
    parse_engine_options_long(argc, argv, [](int32_t option_id, const char *) {
      if (option_id == 'h') {
        usage_and_exit();
      }
      if (option_id == version_and_first_option_id_) {
        printf("%s\n", get_version_string());
        exit(0);
      }
      option_id -= version_and_first_option_id_ + 1;
      auto &this_ = get_instance();
      if (option_id < 0 || option_id >= this_.options_.size()) {
        return -1;
      }
      this_.options_[option_id]->set_option_arg_value(optarg);
      return 0;
    });

    if (optind >= argc) {
      usage_and_exit();
    }

    int num_optargs = argc - optind;
    if (num_optargs != 1) {
      printf("error: expected exactly 1 positional argument: main PHP file\n");
      usage_and_exit();
    }
    other_options_->set_option_arg_value(argv[optind]);

    finalize();
  }

  void dump_options(std::ostream &out) const noexcept {
    out << other_options_description_ << ": ";
    other_options_->dump_option(out);
    out << std::endl << std::endl;
    for (const auto &raw_option : options_) {
      out << raw_option->get_env_var() << ": [";
      raw_option->dump_option(out);
      out << "]" << std::endl;
    }
    out << std::endl;
    for (const auto &implicit_option : implicit_options_) {
      out << implicit_option.first << ": [" << implicit_option.second->get() << "]" << std::endl;
    }
    out << std::endl;
  }

private:
  void finalize() {
    other_options_->parse_arg_value();

    for (auto &raw_option : options_) {
      for (auto &option_for_substitute : options_) {
        raw_option->substitute_depends(*option_for_substitute);
      }
      raw_option->verify_arg_value();
      raw_option->parse_arg_value();
    }
  }

  OptionParser() = default;

  KphpRawOption *other_options_{nullptr};
  vk::string_view other_options_description_;

  std::vector<KphpRawOption *> options_;
  std::unordered_set<vk::string_view> envs_;

  std::vector<std::pair<vk::string_view, KphpImplicitOption *>> implicit_options_;

  static constexpr int32_t version_and_first_option_id_{2000};
};

std::string get_default_kphp_path() noexcept {
#ifdef DEFAULT_KPHP_PATH
  return as_dir(DEFAULT_KPHP_PATH);
#endif
  auto *home = getenv("HOME");
  assert(home);
  return std::string{home} + "/kphp/";
}

std::string get_default_cxx() noexcept {
#ifdef __clang__
  return "clang++";
#else
  return "g++";
#endif
}

std::string with_extra_flag(std::string flags) {
#if ASAN_ENABLED
  flags.append(" -fsanitize=address");
#endif
#if USAN_ENABLED
  flags.append(" -fsanitize=undefined -fno-sanitize-recover=undefined");
#endif
  return flags;
}

std::string get_default_extra_cxxflags() noexcept {
  return with_extra_flag("-O2 -ggdb -fsigned-char");
}

std::string get_default_extra_ldflags() noexcept {
  std::string flags{"-L${KPHP_PATH}/objs/flex -ggdb"};
#ifdef KPHP_HAS_NO_PIE
  flags += " " KPHP_HAS_NO_PIE;
#endif
  return with_extra_flag(std::move(flags));
}

} // namespace

int main(int argc, char *argv[]) {
#if ASAN_ENABLED
  // asan adds redzones to the stack variables, which increases the footprint,
  // therefore increase the system limit on the main thread stack
  constexpr int asan_stack_limit = 81920;
  raise_stack_rlimit(asan_stack_limit);
#endif

  init_version_string("kphp2cpp");
  set_debug_handlers();

  auto settings = std::make_unique<CompilerSettings>();

  OptionParser::add_default_options();
  auto &parser = OptionParser::get_instance();
  parser.add_other_args("<main-file>", settings->main_file);
  parser.add("Verbosity", settings->verbosity,
             'v', "verbosity", "KPHP_VERBOSITY", "0", {"0", "1", "2", "3"});
  parser.add("Path to kphp source", settings->kphp_src_path,
             's', "source-path", "KPHP_PATH", get_default_kphp_path());
  parser.add("Internal file with the list of supported PHP functions", settings->functions_file,
             'f', "functions-file", "KPHP_FUNCTIONS", "${KPHP_PATH}/builtin-functions/_functions.txt");
  parser.add("File with kphp runtime sha256 hash", settings->runtime_sha256_file,
             "runtime-sha256", "KPHP_RUNTIME_SHA256", "${KPHP_PATH}/objs/php_lib_version.sha256");
  parser.add("The output binary type: server, cli or lib", settings->mode,
             'M', "mode", "KPHP_MODE", "server", {"server", "cli", "lib"});
  parser.add("A runtime library for building the output binary", settings->link_file,
             'l', "link-with", "KPHP_LINK_FILE", "${KPHP_PATH}/objs/libkphp-full-runtime.a");
  parser.add("Directory where php files will be searched", settings->includes,
             'I', "include-dir", "KPHP_INCLUDE_DIR");
  parser.add("Destination directory", settings->dest_dir,
             'd', "destination-directory", "KPHP_DEST_DIR", "./kphp_out/");
  parser.add("Path for the output binary", settings->user_binary_path,
             'o', "output-file", "KPHP_USER_BINARY_PATH");
  parser.add("Directory for placing out static lib and header. Compatible only with lib mode", settings->static_lib_out_dir,
             'O', "output-lib-dir", "KPHP_OUT_LIB_DIR");
  parser.add("Force make. Old object files and binary will be removed", settings->force_make,
             'F', "force-make", "KPHP_FORCE_MAKE");
  parser.add("Code generation only, without making an output binary", settings->no_make,
             "no-make", "KPHP_NO_MAKE");
  parser.add("Processes number for the compilation", settings->jobs_count,
             'j', "jobs-num", "KPHP_JOBS_COUNT", std::to_string(get_default_threads_count()));
  parser.add("Threads number for the transpilation", settings->threads_count,
             't', "threads-count", "KPHP_THREADS_COUNT", std::to_string(get_default_threads_count()));
  parser.add("Count of global variables per dedicated .cpp file. Lowering it could decrease compilation time", settings->globals_split_count,
             "globals-split-count", "KPHP_GLOBALS_SPLIT_COUNT", "1024");
  parser.add("Builtin tl schema. Incompatible with lib mode", settings->tl_schema_file,
             'T', "tl-schema", "KPHP_TL_SCHEMA");
  parser.add("Generate storers and fetchers for internal tl functions", settings->gen_tl_internals,
             "gen-tl-internals", "KPHP_GEN_TL_INTERNALS");
  parser.add("All compile time warnings will be errors", settings->error_on_warns,
             'W', "Werror", "KPHP_ERROR_ON_WARNINGS");
  parser.add("Print all warnings to file, otherwise warnings are printed to stderr", settings->warnings_file,
             "warnings-file", "KPHP_WARNINGS_FILE");
  parser.add("Warnings level: prints more warnings, according to level set", settings->warnings_level,
             "warnings-level", "KPHP_WARNINGS_LEVEL", "0", {"0", "1", "2"});
  parser.add("Show all type errors", settings->show_all_type_errors,
             "show-all-type-errors", "KPHP_SHOW_ALL_TYPE_ERRORS");
  parser.add("Colorize warnings output: yes, no, auto", settings->colorize,
             "colorize", "KPHP_COLORS", "auto", {"auto", "yes", "no"});
  parser.add("Save C++ compiler statistics to file", settings->stats_file,
             "stats-file", "KPHP_STATS_FILE");
  parser.add("Save transpilation metrics to file", settings->compilation_metrics_file,
             "compilation-metrics-file", "KPHP_COMPILATION_METRICS_FILE");
  parser.add("Override kphp version string", settings->override_kphp_version,
             "kphp-version-override", "KPHP_VERSION_OVERRIDE");
  parser.add("Specify the compiled php code version", settings->php_code_version,
             "php-code-version", "KPHP_PHP_CODE_VERSION", "unknown");
  parser.add("C++ compiler for building the output binary", settings->cxx,
             "cxx", "KPHP_CXX", get_default_cxx());
  parser.add("Directory for c++ compiler toolchain. Will be passed to cxx via -B option", settings->cxx_toolchain_dir,
             "cxx-toolchain-dir", "KPHP_CXX_TOOLCHAIN_DIR");
  parser.add("Extra C++ compiler flags for building the output binary", settings->extra_cxx_flags,
             "extra-cxx-flags", "KPHP_EXTRA_CXXFLAGS", get_default_extra_cxxflags());
  parser.add("Extra linker flags for building the output binary", settings->extra_ld_flags,
             "extra-linker-flags", "KPHP_EXTRA_LDFLAGS", get_default_extra_ldflags());
  parser.add("C++ compiler debug level for building the output binary", settings->extra_cxx_debug_level,
             "debug-level", "KPHP_DEBUG_LEVEL");
  parser.add("Archive creator for building the output binary", settings->archive_creator,
             "archive-creator", "KPHP_ARCHIVE_CREATOR", "ar");
  parser.add("Use dynamic incremental linkage for building the output binary", settings->dynamic_incremental_linkage,
             "dynamic-incremental-linkage", "KPHP_DYNAMIC_INCREMENTAL_LINKAGE");
  parser.add("Profile functions: 0 - disabled, 1 - enabled for marked functions, 2 - enabled for all", settings->profiler_level,
             'g', "profiler", "KPHP_PROFILER", "0", {"0", "1", "2"});
  parser.add("Enable an ability to get global vars memory stats", settings->enable_global_vars_memory_stats,
             "enable-global-vars-memory-stats", "KPHP_ENABLE_GLOBAL_VARS_MEMORY_STATS");
  parser.add("Enable all inspections available for @kphp-analyze-performance for all reachable functions", settings->enable_full_performance_analyze,
             "enable-full-performance-analyze", "KPHP_ENABLE_FULL_PERFORMANCE_ANALYZE");
  parser.add("Print graph of resumable calls to stderr", settings->print_resumable_graph,
             'p', "print-graph", "KPHP_PRINT_RESUMABLE_GRAPH");
  parser.add("Forbid to use the precompile header", settings->no_pch,
             "no-pch", "KPHP_NO_PCH");
  parser.add("Forbid to use an index file which contains codegen hashes from previous compilation", settings->no_index_file,
             "no-index-file", "KPHP_NO_INDEX_FILE");
  parser.add("Show transpilation progress", settings->show_progress,
             "show-progress", "KPHP_SHOW_PROGRESS");
  parser.add("A folder that contains composer.json file", settings->composer_root,
             "composer-root", "KPHP_COMPOSER_ROOT");
  parser.add("Include autoload-dev section for the root composer file", settings->composer_autoload_dev,
             "composer-autoload-dev", "KPHP_COMPOSER_AUTOLOAD_DEV");
  parser.add("Require functions typing (1 - @param / type hint is mandatory, 0 - auto infer or check if exists)", settings->require_functions_typing,
             "require-functions-typing", "KPHP_REQUIRE_FUNCTIONS_TYPING");
  parser.add("Require class typing (1 - @var / default value is mandatory, 0 - auto infer or check if exists)", settings->require_class_typing,
             "require-class-typing", "KPHP_REQUIRE_CLASS_TYPING");

  parser.add_implicit_option("Linker flags", settings->ld_flags);
  parser.add_implicit_option("Incremental linker flags", settings->incremental_linker_flags);
  parser.add_implicit_option("Base directory", settings->base_dir);
  parser.add_implicit_option("CPP destination directory", settings->dest_cpp_dir);
  parser.add_implicit_option("Objs destination directory", settings->dest_objs_dir);
  parser.add_implicit_option("Binary path", settings->binary_path);
  parser.add_implicit_option("Static lib name", settings->static_lib_name);
  parser.add_implicit_option("Runtime SHA256", settings->runtime_sha256);
  parser.add_implicit_option("Runtime headers", settings->runtime_headers);
  parser.add_implicit_option("C++ compiler flags default", settings->cxx_flags_default.flags);
  parser.add_implicit_option("C++ compiler flags default SHA256", settings->cxx_flags_default.flags_sha256);
  parser.add_implicit_option("C++ compiler flags with debug", settings->cxx_flags_with_debug.flags);
  parser.add_implicit_option("C++ compiler flags with debug SHA256", settings->cxx_flags_with_debug.flags_sha256);
  parser.add_implicit_option("TL namespace prefix", settings->tl_namespace_prefix);
  parser.add_implicit_option("TL classname prefix", settings->tl_classname_prefix);
  parser.add_implicit_option("Generated runtime path", settings->generated_runtime_path);
  parser.add_implicit_option("Performance report path", settings->performance_analyze_report_path);
  parser.add_implicit_option("C++ compiler toolchain option", settings->cxx_toolchain_option);

  try {
    parser.process_args(argc, argv);
    settings->init();
  } catch (const std::exception &ex) {
    std::cout << ex.what() << std::endl;
    return 1;
  }

  if (settings->verbosity.get() >= 3) {
    parser.dump_options(std::cerr);
  }

  if (!compiler_execute(settings.release())) {
    return 1;
  }

  return 0;
}
