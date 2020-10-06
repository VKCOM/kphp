#include "compiler/compiler-settings.h"

#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
#include <sstream>
#include <thread>
#include <unistd.h>

#include "common/algorithms/contains.h"
#include "common/algorithms/find.h"
#include "common/version-string.h"
#include "common/wrappers/fmt_format.h"

#include "compiler/stage.h"
#include "compiler/utils/string-utils.h"

void KphpRawOption::init(vk::string_view long_option, char short_option, vk::string_view env,
                         vk::string_view default_value, std::vector<std::string> choices) noexcept {
  env_var_.assign(env.begin(), env.end());
  if (char *val = getenv(env_var_.c_str())) {
    raw_option_arg_ = val;
  } else {
    raw_option_arg_.assign(default_value.begin(), default_value.end());
  }

  cmd_option_full_name_.assign("--").append(long_option.begin(), long_option.end());
  if (short_option) {
    cmd_option_full_name_.append("/-").append(1, short_option);
  }
  cmd_option_full_name_.append(" [").append(env_var_).append("]");
  choices_ = std::move(choices);
}

void KphpRawOption::substitute_depends(const KphpRawOption &other) noexcept {
  std::string pattern_to_replace = "${" + other.env_var_ + "}";
  raw_option_arg_ = vk::replace_all(raw_option_arg_, pattern_to_replace, other.raw_option_arg_);
}

void KphpRawOption::verify_arg_value() const {
  if (!choices_.empty() && !vk::contains(choices_, raw_option_arg_)) {
    throw_param_exception("choose from " + vk::join(choices_, ", "));
  }
}

void KphpRawOption::throw_param_exception(const std::string &reason) const {
  throw std::runtime_error{"Can't parse " + cmd_option_full_name_ + " option: " + reason};
}

template<>
void KphpOption<std::string>::parse_arg_value() noexcept {
  // Don't move it, it can be used later
  value_ = raw_option_arg_;
}

template<>
void KphpOption<uint64_t>::parse_arg_value() noexcept {
  if (raw_option_arg_.empty()) {
    value_ = 0;
  } else {
    try {
      value_ = std::stoul(raw_option_arg_);
    } catch (...) {
      throw_param_exception("unsigned integer is expected");
    }
  }
}

template<>
void KphpOption<bool>::parse_arg_value() noexcept {
  if (vk::none_of_equal(raw_option_arg_, "1", "0", "")) {
    throw_param_exception("'0' or '1' are expected");
  }
  value_ = raw_option_arg_ == "1";
}

template<>
void KphpOption<std::vector<std::string>>::parse_arg_value() noexcept {
  value_ = split(raw_option_arg_, ':');
}

namespace {
static void as_dir(std::string &path) {
  if (path.empty()) {
    return;
  }
  auto full_path = get_full_path(path);
  if (!full_path.empty()) {
    path = std::move(full_path);
  }
  if (path.back() != '/') {
    path += "/";
  }
  if (path.front() != '/') {
    path.insert(0, "./");
  }
}
} // namespace

std::string CompilerSettings::get_home() noexcept {
  const char *home = getenv("HOME");
  kphp_assert(home);
  std::string home_str = home;
  as_dir(home_str);
  return home_str;
}

void CompilerSettings::option_as_dir(KphpOption<std::string> &path_option) noexcept {
  as_dir(path_option.value_);
}

const string &CompilerSettings::get_binary_path() const {
  return binary_path_;
}

const std::string &CompilerSettings::get_static_lib_name() const {
  return static_lib_name_;
}

const string &CompilerSettings::get_runtime_sha256() const {
  return runtime_sha256_;
}

const string &CompilerSettings::get_cxx_flags_sha256() const {
  return cxx_flags_sha256_;
}

void CompilerSettings::add_main_file(const string &main_file) {
  main_files_.push_back(main_file);
}

const vector<string> &CompilerSettings::get_main_files() const {
  return main_files_;
}

const string &CompilerSettings::get_dest_cpp_dir() const {
  return dest_cpp_dir_;
}

const string &CompilerSettings::get_dest_objs_dir() const {
  return dest_objs_dir_;
}

const string &CompilerSettings::get_cxx_flags() const {
  return cxx_flags_;
}

const string &CompilerSettings::get_ld_flags() const {
  return ld_flags_;
}

const string &CompilerSettings::get_incremental_linker() const {
  return incremental_linker_;
}

const string &CompilerSettings::get_incremental_linker_flags() const {
  return incremental_linker_flags_;
}

bool CompilerSettings::is_static_lib_mode() const {
  return mode.get() == "lib";
}

std::string CompilerSettings::get_version() const {
  return override_kphp_version.get().empty() ? get_version_string() : override_kphp_version.get();
}

void CompilerSettings::update_cxx_flags_sha256() {
  SHA256_CTX sha256;
  SHA256_Init(&sha256);

  auto cxx_flags_full = cxx.get() + cxx_flags_ + debug_level.get();
  SHA256_Update(&sha256, cxx_flags_full.c_str(), cxx_flags_full.size());

  unsigned char hash[SHA256_DIGEST_LENGTH] = {0};
  SHA256_Final(hash, &sha256);

  char hash_str[SHA256_DIGEST_LENGTH * 2 + 1] = {0};
  for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    fmt_format_to(hash_str + (i * 2), "{:02x}", hash[i]);
  }
  cxx_flags_sha256_.assign(hash_str, SHA256_DIGEST_LENGTH * 2);
}

void CompilerSettings::init() {
  option_as_dir(kphp_src_path);

  if (is_static_lib_mode()) {
    if (main_files_.size() > 1) {
      throw std::runtime_error{"Multiple main directories are forbidden for static lib mode"};
    }
    if (!tl_schema_file.get().empty()) {
      throw std::runtime_error{"Option " + tl_schema_file.get_option_full_name() + " is forbidden for static lib mode"};
    }
    std::string lib_dir = get_full_path(main_files_.back());
    std::size_t last_slash = lib_dir.rfind('/');
    if (last_slash == std::string::npos) {
      throw std::runtime_error{"Bad lib directory"};
    }
    static_lib_name_ = lib_dir.substr(last_slash + 1);
    if (static_lib_name_.empty()) {
      throw std::runtime_error{"Empty static lib name"};
    }
    as_dir(lib_dir);
    includes.value_.emplace_back(lib_dir + "php/");
    if (static_lib_out_dir.get().empty()) {
      static_lib_out_dir.value_ = lib_dir + "lib/";
    }

    option_as_dir(static_lib_out_dir);
    main_files_.back().assign(lib_dir).append("/php/index.php");
  } else if (!static_lib_out_dir.get().empty()) {
    throw std::runtime_error{"Option " + static_lib_out_dir.get_option_full_name() + " is allowed only for static lib mode"};
  }

  if (!jobs_count.get()) {
    jobs_count.value_ = std::max(std::thread::hardware_concurrency(), 1U);
  }
  if (!threads_count.get()) {
    threads_count.value_ = std::max(std::thread::hardware_concurrency(), 1U);
  }

  for (string &include : includes.value_) {
    as_dir(include);
  }

  if (colorize.get() == "auto") {
    color_ = auto_colored;
  } else if (colorize.get() == "no") {
    color_ = not_colored;
  } else if (colorize.get() == "yes") {
    color_ = colored;
  } else {
    kphp_assert(0);
  }

  std::stringstream ss;
  ss << extra_cxx_flags.get();
  ss << " -iquote" << kphp_src_path.get() << " -iquote" << kphp_src_path.get() << "PHP/";
  ss << " -Wall -fwrapv -Wno-parentheses -Wno-trigraphs";
  ss << " -fno-strict-aliasing -fno-omit-frame-pointer";
  if (!no_pch.get()) {
    ss << " -Winvalid-pch -fpch-preprocess";
  }
  if (dynamic_incremental_linkage.get()) {
    ss << " -fPIC";
  }
  if (vk::contains(cxx.get(), "clang")) {
    ss << " -Wno-invalid-source-encoding";
  }
  #if __cplusplus <= 201103L
  ss << " -std=gnu++11";
  #elif __cplusplus <= 201402L
  ss << " -std=gnu++14";
  #elif __cplusplus <= 201703L
  ss << " -std=gnu++17";
  #elif __cplusplus <= 202002L
  ss << " -std=gnu++20";
  #else
    #error unsupported __cplusplus value
  #endif

  cxx_flags_ = ss.str();

  update_cxx_flags_sha256();
  runtime_sha256_ = read_runtime_sha256_file(runtime_sha256_file.get());

  incremental_linker_ = dynamic_incremental_linkage.get() ? cxx.get() : "ld";
  incremental_linker_flags_ = dynamic_incremental_linkage.get() ? "-shared" : "-r";
  ld_flags_ = extra_ld_flags.get() + " -lm -lz -lpthread -lrt -lcrypto -lpcre -lre2 -lyaml-cpp -lh3 -rdynamic";

  option_as_dir(dest_dir);

  dest_cpp_dir_ = dest_dir.get() + "kphp/";
  dest_objs_dir_ = dest_dir.get() + "objs/";
  binary_path_ = dest_dir.get() + mode.get();
  cxx_flags_ += " -iquote" + get_dest_cpp_dir();
}

void CompilerSettings::debug() const {
  //std::cerr <<
  //          "HOME=[" << get_home() << "]\n" <<
  //          "KPHP_DEST_DIR=[" << get_dest_dir() << "]\n" <<
  //          "KPHP_FUNCTIONS=[" << get_functions() << "]\n" <<
  //          "KPHP_JOBS_COUNT=[" << get_jobs_count() << "]\n" <<
  //          "KPHP_MODE=[" << get_mode() << "]\n" <<
  //          "KPHP_LINK_FILE=[" << get_link_file() << "]\n" <<
  //          "KPHP_USE_MAKE=[" << get_use_make() << "]\n" <<
  //          "KPHP_THREADS_COUNT=[" << get_threads_count() << "]\n" <<
  //          "KPHP_PATH=[" << get_path() << "]\n" <<
  //          "KPHP_USER_BINARY_PATH=[" << get_user_binary_path() << "]\n" <<
  //          "KPHP_RUNTIME_SHA256_FILE=[" << get_runtime_sha256_file() << "]\n" <<
  //          "KPHP_RUNTIME_SHA256=[" << get_runtime_sha256() << "]\n" <<
  //          "KPHP_TL_SCHEMA=[" << get_tl_schema_file() << "]\n" <<
  //          "KPHP_VERBOSITY=[" << get_verbosity() << "]\n" <<
  //          "KPHP_PROFILER=[" << get_profiler_level() << "]\n" <<
  //          "KPHP_NO_PCH=[" << get_no_pch() << "]\n" <<
  //          "KPHP_NO_INDEX_FILE=[" << get_no_index_file() << "]\n" <<
  //          "KPHP_CONTINUE_ON_TYPE_ERROR=[" << get_continue_on_type_error() << "]\n" <<
  //          "KPHP_ENABLE_GLOBAL_VARS_MEMORY_STATS=[" << get_enable_global_vars_memory_stats() << "]\n" <<
  //          "KPHP_GEN_TL_INTERNALS=[" << get_gen_tl_internals() << "]\n" <<
  //          "KPHP_COMPILATION_METRICS_FILE=[" << get_compilation_metrics_filename() << "]\n" <<
  //          "KPHP_DYNAMIC_INCREMENTAL_LINKAGE = [" << get_dynamic_incremental_linkage() << "]\n" <<
  //          "KPHP_PHP_CODE_VERSION=[" << get_php_code_version() << "]\n" <<
  //
  //          "KPHP_AUTO_DEST=[" << get_use_auto_dest() << "]\n" <<
  //          "KPHP_BINARY_PATH=[" << get_binary_path() << "]\n" <<
  //          "KPHP_DEST_CPP_DIR=[" << get_dest_cpp_dir() << "]\n" <<
  //          "KPHP_DEST_OBJS_DIR=[" << get_dest_objs_dir() << "]\n";
  //
  //if (is_static_lib_mode()) {
  //  std::cerr << "KPHP_OUT_LIB_DIR=[" << get_static_lib_out_dir() << "]\n";
  //}
  //
  //std::cerr << "CXX=[" << get_cxx() << "]\n" <<
  //          "CXXFLAGS=[" << get_cxx_flags() << "]\n" <<
  //          "LDFLAGS=[" << get_ld_flags() << "]\n" <<
  //          "AR=[" << get_ar() << "]\n" <<
  //          "KPHP_INCREMENTAL_LINKER=[" << get_incremental_linker() << "]\n" <<
  //          "KPHP_INCREMENTAL_LINKER_FLAGS=[" << get_incremental_linker_flags() << "]\n";
  //std::cerr << "KPHP_INCLUDES=[";
  //bool is_first = true;
  //for (const auto &include : get_includes()) {
  //  if (is_first) {
  //    is_first = false;
  //  } else {
  //    std::cerr << ";";
  //  }
  //  std::cerr << include;
  //}
  //std::cerr << "]\n";
  //std::cerr << "KPHP_MAIN_FILES=[";
  //is_first = true;
  //for (const auto &main_file : get_main_files()) {
  //  if (is_first) {
  //    is_first = false;
  //  } else {
  //    std::cerr << ";";
  //  }
  //  std::cerr << main_file;
  //}
  //std::cerr << "]\n";
}

std::string CompilerSettings::read_runtime_sha256_file(const std::string &filename) {
  std::ifstream runtime_sha256_file(filename.c_str());
  kphp_error(runtime_sha256_file,
             fmt_format("Can't open runtime sha256 file '{}'", filename));

  constexpr std::streamsize SHA256_LEN = 64;
  char runtime_sha256[SHA256_LEN] = {0};
  runtime_sha256_file.read(runtime_sha256, SHA256_LEN);
  kphp_error(runtime_sha256_file.gcount() == SHA256_LEN,
             fmt_format("Can't read runtime sha256 from file '{}'", filename));
  return std::string(runtime_sha256, runtime_sha256 + SHA256_LEN);
}

CompilerSettings::color_settings CompilerSettings::get_color_settings() const {
  return color_;
}

const std::string &CompilerSettings::get_tl_namespace_prefix() const {
  static std::string tl_namespace_name("VK\\TL\\");
  return tl_namespace_name;
}

const std::string &CompilerSettings::get_tl_classname_prefix() const {
  static std::string tl_classname_prefix("C$VK$TL$");
  return tl_classname_prefix;
}
