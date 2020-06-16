#include "compiler/enviroment.h"

#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
#include <sstream>
#include <unistd.h>

#include "common/algorithms/contains.h"
#include "common/version-string.h"
#include "common/wrappers/fmt_format.h"

#include "compiler/stage.h"
#include "compiler/utils/string-utils.h"

/*** Enviroment ***/

static void as_dir(string *s) {
  if (s->empty()) {
    return;
  }
  if ((*s)[(int)s->size() - 1] != '/') {
    *s += "/";
  }
  if ((*s)[0] != '/') {
    *s = "./" + *s;
  }
}

static void init_env_var(string *str, const string &var_name, const string &default_value) {
  if (!str->empty()) {
    return;
  }
  if (!var_name.empty()) {
    char *val = getenv(var_name.c_str());
    if (val != nullptr) {
      *str = val;
      return;
    }
  }
  *str = default_value;
}

void env_str2int(int *dest, const string &src) {
  *dest = atoi(src.c_str());
}

void get_bool_option_from_env(bool &option_value, const char *env, bool default_value) {
  if (option_value == default_value) {
    std::string env_value;
    init_env_var(&env_value, env, default_value ? "1" : "0");
    option_value = static_cast <bool> (atoi(env_value.c_str()));
  }
}

const string &KphpEnviroment::get_home() const {
  return home_;
}

void KphpEnviroment::set_base_dir(const string &base_dir) {
  base_dir_ = base_dir;
}

const string &KphpEnviroment::get_base_dir() const {
  return base_dir_;
}

void KphpEnviroment::set_dest_dir(const string &dest_dir) {
  dest_dir_ = dest_dir;
}

const string &KphpEnviroment::get_dest_dir() const {
  return dest_dir_;
}

void KphpEnviroment::set_use_auto_dest() {
  use_auto_dest_bool_ = true;
}

bool KphpEnviroment::get_use_auto_dest() const {
  return use_auto_dest_bool_;
}

void KphpEnviroment::set_functions(const string &functions) {
  functions_ = functions;
}

const string &KphpEnviroment::get_functions() const {
  return functions_;
}

void KphpEnviroment::add_include(const string &include) {
  includes_.push_back(include);
}

const vector<string> &KphpEnviroment::get_includes() const {
  return includes_;
}

void KphpEnviroment::set_jobs_count(const string &jobs_count) {
  jobs_count_ = jobs_count;
}

int KphpEnviroment::get_jobs_count() const {
  return jobs_count_int_;
}

void KphpEnviroment::set_mode(const string &mode) {
  mode_ = mode;
}

const string &KphpEnviroment::get_mode() const {
  return mode_;
}

void KphpEnviroment::set_link_file(const string &link_file) {
  link_file_ = link_file;
}

const string &KphpEnviroment::get_link_file() const {
  return link_file_;
}

void KphpEnviroment::set_use_make() {
  use_make_bool_ = true;
}

bool KphpEnviroment::get_use_make() const {
  return use_make_bool_;
}

void KphpEnviroment::set_make_force() {
  make_force_bool_ = true;
}

bool KphpEnviroment::get_make_force() const {
  return make_force_bool_;
}

const string &KphpEnviroment::get_binary_path() const {
  return binary_path_;
}

void KphpEnviroment::set_user_binary_path(const string &user_binary_path) {
  user_binary_path_ = user_binary_path;
}

const string &KphpEnviroment::get_user_binary_path() const {
  return user_binary_path_;
}

void KphpEnviroment::set_static_lib_out_dir(string &&lib_dir) {
  static_lib_out_dir_ = std::move(lib_dir);
}

const string &KphpEnviroment::get_static_lib_out_dir() const {
  return static_lib_out_dir_;
}

const string &KphpEnviroment::get_static_lib_name() const {
  return static_lib_name_;
}

void KphpEnviroment::set_threads_count(const string &threads_count) {
  threads_count_ = threads_count;
}

int KphpEnviroment::get_threads_count() const {
  return threads_count_int_;
}

void KphpEnviroment::set_tl_schema_file(const string &tl_schema_file) {
  tl_schema_file_ = tl_schema_file;
}

string KphpEnviroment::get_tl_schema_file() const {
  return tl_schema_file_;
}

void KphpEnviroment::set_path(const string &path) {
  path_ = path;
}

const string &KphpEnviroment::get_path() const {
  return path_;
}

void KphpEnviroment::set_runtime_sha256_file(string &&file_name) {
  runtime_sha256_filename_ = std::move(file_name);
}

const string &KphpEnviroment::get_runtime_sha256_file() const {
  return runtime_sha256_filename_;
}

const string &KphpEnviroment::get_runtime_sha256() const {
  return runtime_sha256_;
}

const string &KphpEnviroment::get_cxx_flags_sha256() const {
  return cxx_flags_sha256_;
}

void KphpEnviroment::inc_verbosity() {
  verbosity_int_++;
}

int KphpEnviroment::get_verbosity() const {
  return verbosity_int_;
}

void KphpEnviroment::set_print_resumable_graph() {
  print_resumable_graph_ = 1;
}

int KphpEnviroment::get_print_resumable_graph() const {
  return print_resumable_graph_;
}

void KphpEnviroment::set_enable_profiler() {
  enable_profiler_ = 1;
}

int KphpEnviroment::get_enable_profiler() const {
  return enable_profiler_;
}

void KphpEnviroment::set_no_pch() {
  no_pch_ = true;
}

bool KphpEnviroment::get_no_pch() const {
  return no_pch_;
}

void KphpEnviroment::set_no_index_file() {
  no_index_file_ = true;
}

bool KphpEnviroment::get_no_index_file() const {
  return no_index_file_;
}

bool KphpEnviroment::get_stop_on_type_error() const {
  return stop_on_type_error_;
}

bool KphpEnviroment::get_show_progress() const {
  return show_progress_;
}

void KphpEnviroment::add_main_file(const string &main_file) {
  main_files_.push_back(main_file);
}

const vector<string> &KphpEnviroment::get_main_files() const {
  return main_files_;
}

void KphpEnviroment::set_enable_global_vars_memory_stats() {
  enable_global_vars_memory_stats_ = true;
}

bool KphpEnviroment::get_enable_global_vars_memory_stats() const {
  return enable_global_vars_memory_stats_;
}

void KphpEnviroment::set_dynamic_incremental_linkage() {
  dynamic_incremental_linkage_ = true;
}

bool KphpEnviroment::get_dynamic_incremental_linkage() const {
  return dynamic_incremental_linkage_;
}

const string &KphpEnviroment::get_dest_cpp_dir() const {
  return dest_cpp_dir_;
}

const string &KphpEnviroment::get_dest_objs_dir() const {
  return dest_objs_dir_;
}

const string &KphpEnviroment::get_cxx() const {
  return cxx_;
}

const string &KphpEnviroment::get_cxx_flags() const {
  return cxx_flags_;
}

const string &KphpEnviroment::get_ld_flags() const {
  return ld_flags_;
}

const string &KphpEnviroment::get_ar() const {
  return ar_;
}

const string &KphpEnviroment::get_incremental_linker() const {
  return incremental_linker_;
}

const string &KphpEnviroment::get_incremental_linker_flags() const {
  return incremental_linker_flags_;
}

bool KphpEnviroment::is_static_lib_mode() const {
  return mode_ == "lib";
}

void KphpEnviroment::set_error_on_warns() {
  error_on_warns_ = true;
}

bool KphpEnviroment::get_error_on_warns() const {
  return error_on_warns_;
}

void KphpEnviroment::set_warnings_filename(const string &path) {
  warnings_filename_ = path;
}

void KphpEnviroment::set_compilation_metrics_filename(string &&path) {
  compilation_metrics_file_ = std::move(path);
}

void KphpEnviroment::set_stats_filename(const string &path) {
  stats_filename_ = path;
}

const string &KphpEnviroment::get_warnings_filename() const {
  return warnings_filename_;
}

const string &KphpEnviroment::get_compilation_metrics_filename() const {
  return compilation_metrics_file_;
}

const string &KphpEnviroment::get_stats_filename() const {
  return stats_filename_;
}

FILE *KphpEnviroment::get_stats_file() const {
  return stats_file_;
}

void KphpEnviroment::set_stats_file(FILE *file) {
  stats_file_ = file;
}

FILE *KphpEnviroment::get_warnings_file() const {
  return warnings_file_;
}

void KphpEnviroment::set_warnings_file(FILE *file) {
  warnings_file_ = file;
}

void KphpEnviroment::set_warnings_level(int level) {
  warnings_level_ = level;
}

int KphpEnviroment::get_warnings_level() const {
  return warnings_level_;
}

void KphpEnviroment::set_debug_level(const string &level) {
  debug_level_ = level;
}

const string &KphpEnviroment::get_debug_level() const {
  return debug_level_;
}

const string &KphpEnviroment::get_version() const {
  return version_;
}

void KphpEnviroment::update_cxx_flags_sha256() {
  SHA256_CTX sha256;
  SHA256_Init(&sha256);

  auto cxx_flags_full = cxx_ + cxx_flags_ + debug_level_;
  SHA256_Update(&sha256, cxx_flags_full.c_str(), cxx_flags_full.size());

  unsigned char hash[SHA256_DIGEST_LENGTH] = {0};
  SHA256_Final(hash, &sha256);

  char hash_str[SHA256_DIGEST_LENGTH * 2 + 1] = {0};
  for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    fmt_format_to(hash_str + (i * 2), "{:02x}", hash[i]);
  }
  cxx_flags_sha256_.assign(hash_str, SHA256_DIGEST_LENGTH * 2);
}

bool KphpEnviroment::init() {
  char tmp[PATH_MAX];
  char *cur_dir = getcwd(tmp, PATH_MAX);
  if (cur_dir == nullptr) {
    fmt_print("Failed to get current directory\n");
    return false;
  }
  cur_dir_ = cur_dir;
  as_dir(&cur_dir_);
  home_ = getenv("HOME");
  as_dir(&home_);
  init_env_var(&path_, "KPHP_PATH", get_home() + "engine/src/");
  as_dir(&path_);
  init_env_var(&functions_, "KPHP_FUNCTIONS", get_path() + "PHP/functions.txt");
  init_env_var(&mode_, "KPHP_MODE", "server");
  if (mode_ == "net") {
    mode_ = "server";
  }
  string link_file_name;
  if (mode_ == "server") {
    link_file_name = "php-main-server.a";
  } else if (mode_ == "cli") {
    link_file_name = "php-main-cli.a";
  } else if (!is_static_lib_mode()) {
    fmt_print("Unknown $KPHP_MODE={}\n", mode_);
    return false;
  }

  init_env_var(&tl_schema_file_, "KPHP_TL_SCHEMA", "");

  if (is_static_lib_mode()) {
    if (main_files_.size() > 1) {
      fmt_print("Multiple main directories are forbidden for static lib mode\n");
      return false;
    }
    if (!tl_schema_file_.empty()) {
      fmt_print("tl-schema is forbidden for static lib mode\n");
      return false;
    }
    std::string lib_dir = get_full_path(main_files_.back());
    std::size_t last_slash = lib_dir.rfind('/');
    if (last_slash == std::string::npos) {
      fmt_print("Bad lib directory\n");
      return false;
    }
    static_lib_name_ = lib_dir.substr(last_slash + 1);
    if (static_lib_name_.empty()) {
      fmt_print("Got empty static lib name\n");
      return false;
    }
    as_dir(&lib_dir);
    add_include(lib_dir + "php/");
    init_env_var(&static_lib_out_dir_, "KPHP_OUT_LIB_DIR", lib_dir + "lib/");
    as_dir(&static_lib_out_dir_);
    main_files_.back().assign(lib_dir).append("/php/index.php");
  }
  else {
    if (!static_lib_out_dir_.empty()) {
      fmt_print("Output dir is allowed only for static lib mode\n");
      return false;
    }
    init_env_var(&link_file_, "KPHP_LINK_FILE", get_path() + "objs/PHP/" + link_file_name);
  }

  init_env_var(&runtime_sha256_filename_, "KPHP_RUNTIME_SHA256", get_path() + "objs/PHP/php_lib_version.sha256");

  init_env_var(&base_dir_, "", "");
  as_dir(&base_dir_);

  get_bool_option_from_env(no_index_file_, "KPHP_NO_INDEX_FILE", false);
  get_bool_option_from_env(no_pch_, "KPHP_NO_PCH", false);
  get_bool_option_from_env(use_make_bool_, "KPHP_USE_MAKE", false);
  get_bool_option_from_env(stop_on_type_error_, "KPHP_STOP_ON_TYPE_ERROR", true);
  get_bool_option_from_env(show_progress_, "KPHP_SHOW_PROGRESS", true);
  get_bool_option_from_env(enable_global_vars_memory_stats_, "KPHP_ENABLE_GLOBAL_VARS_MEMORY_STATS", false);
  get_bool_option_from_env(gen_tl_internals_, "KPHP_GEN_TL_INTERNALS", false);
  get_bool_option_from_env(dynamic_incremental_linkage_, "KPHP_DYNAMIC_INCREMENTAL_LINKAGE", false);

  init_env_var(&jobs_count_, "KPHP_JOBS_COUNT", "100");
  env_str2int(&jobs_count_int_, jobs_count_);
  if (jobs_count_int_ <= 0) {
    fmt_print("Incorrect jobs_count={}\n", jobs_count_);
    return false;
  }

  init_env_var(&threads_count_, "KPHP_THREADS_COUNT", "16");
  env_str2int(&threads_count_int_, threads_count_);
  if (threads_count_int_ <= 0 || threads_count_int_ > 100) {
    fmt_print("Incorrect threads_count={}\n", threads_count_);
    return false;
  }

  for (string &include : includes_) {
    as_dir(&include);
  }

  string color_settings_str;
  init_env_var(&color_settings_str, "KPHP_COLORS", "auto");
  if (color_settings_str == "auto") {
    color_ = auto_colored;
  } else if (color_settings_str == "no") {
    color_ = not_colored;
  } else if (color_settings_str == "yes") {
    color_ = colored;
  } else {
    fmt_fprintf(stderr, "Unknown color settings {}, fallback to auto\n", color_settings_str);
    color_ = auto_colored;
  }

  init_env_var(&php_code_version_, "KPHP_PHP_CODE_VERSION", "unknown");
  init_env_var(&compilation_metrics_file_, "KPHP_COMPILATION_METRICS_FILE", "");

  string user_cxx_flags;
  init_env_var(&user_cxx_flags, "CXXFLAGS", "-Os -ggdb -march=core2 -mfpmath=sse -mssse3");
  init_env_var(&cxx_, "CXX", "g++");
  std::stringstream ss;
  ss << user_cxx_flags;
  ss << " -iquote" << get_path() << " -iquote" << get_path() << "PHP/";
  ss << " -Wall -fwrapv -Wno-parentheses -Wno-trigraphs";
  ss << " -fno-strict-aliasing -fno-omit-frame-pointer";
  if (!no_pch_) {
    ss << " -Winvalid-pch -fpch-preprocess";
  }
  if (dynamic_incremental_linkage_) {
    ss << " -fPIC";
  }
  if (vk::contains(cxx_, "clang")) {
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
  runtime_sha256_ = read_runtime_sha256_file(get_runtime_sha256_file());

  init_env_var(&incremental_linker_, "KPHP_INCREMENTAL_LINKER",
               dynamic_incremental_linkage_ ? cxx_ : "ld");
  init_env_var(&incremental_linker_flags_, "KPHP_INCREMENTAL_LINKER_FLAGS",
               dynamic_incremental_linkage_ ? "-shared" : "-r");
  string user_ld_flags;
  init_env_var(&user_ld_flags, "LDFLAGS", "-ggdb");
  ld_flags_ = user_ld_flags + " -lm -lz -lpthread -lrt -lcrypto -lpcre -lre2 -l:libyaml-cpp.a -rdynamic";

  init_env_var(&ar_, "AR", "ar");

  //todo: use some hash???
  get_bool_option_from_env(use_auto_dest_bool_, "KPHP_AUTO_DEST", false);
  init_env_var(&dest_dir_, "KPHP_DEST_DIR", get_path() + "PHP/tests/kphp_tmp/default/");
  as_dir(&dest_dir_);
  init_env_var(&version_, "KPHP_VERSION_OVERRIDE", get_version_string());

  init_env_var(&verbosity_, "KPHP_VERBOSITY", "0");
  if (!verbosity_int_) {
    env_str2int(&verbosity_int_, verbosity_);
  }

  return true;
}

void KphpEnviroment::set_dest_dir_subdir(const string &s) {
  dest_dir_ += s;
  as_dir(&dest_dir_);
}

void KphpEnviroment::init_dest_dirs() {
  init_env_var(&dest_cpp_dir_, "", get_dest_dir() + "kphp/");
  as_dir(&dest_cpp_dir_);
  init_env_var(&dest_objs_dir_, "", get_dest_dir() + "objs/");
  as_dir(&dest_objs_dir_);
  init_env_var(&binary_path_, "", get_dest_dir() + get_mode());

  cxx_flags_ += " -iquote" + get_dest_cpp_dir();
}

void KphpEnviroment::debug() const {
  std::cerr <<
            "HOME=[" << get_home() << "]\n" <<
            "KPHP_BASE_DIR=[" << get_base_dir() << "]\n" <<
            "KPHP_DEST_DIR=[" << get_dest_dir() << "]\n" <<
            "KPHP_FUNCTIONS=[" << get_functions() << "]\n" <<
            "KPHP_JOBS_COUNT=[" << get_jobs_count() << "]\n" <<
            "KPHP_MODE=[" << get_mode() << "]\n" <<
            "KPHP_LINK_FILE=[" << get_link_file() << "]\n" <<
            "KPHP_USE_MAKE=[" << get_use_make() << "]\n" <<
            "KPHP_THREADS_COUNT=[" << get_threads_count() << "]\n" <<
            "KPHP_PATH=[" << get_path() << "]\n" <<
            "KPHP_USER_BINARY_PATH=[" << get_user_binary_path() << "]\n" <<
            "KPHP_RUNTIME_SHA256_FILE=[" << get_runtime_sha256_file() << "]\n" <<
            "KPHP_RUNTIME_SHA256=[" << get_runtime_sha256() << "]\n" <<
            "KPHP_TL_SCHEMA=[" << get_tl_schema_file() << "]\n" <<
            "KPHP_VERBOSITY=[" << get_verbosity() << "]\n" <<
            "KPHP_NO_PCH=[" << get_no_pch() << "]\n" <<
            "KPHP_NO_INDEX_FILE=[" << get_no_index_file() << "]\n" <<
            "KPHP_STOP_ON_TYPE_ERROR=[" << get_stop_on_type_error() << "]\n" <<
            "KPHP_ENABLE_GLOBAL_VARS_MEMORY_STATS=[" << get_enable_global_vars_memory_stats() << "]\n" <<
            "KPHP_GEN_TL_INTERNALS=[" << get_gen_tl_internals() << "]\n" <<
            "KPHP_COMPILATION_METRICS_FILE=[" << get_compilation_metrics_filename() << "]\n" <<
            "KPHP_DYNAMIC_INCREMENTAL_LINKAGE = [" << get_dynamic_incremental_linkage() << "]\n" <<
            "KPHP_PHP_CODE_VERSION=[" << get_php_code_version() << "]\n" <<

            "KPHP_AUTO_DEST=[" << get_use_auto_dest() << "]\n" <<
            "KPHP_BINARY_PATH=[" << get_binary_path() << "]\n" <<
            "KPHP_DEST_CPP_DIR=[" << get_dest_cpp_dir() << "]\n" <<
            "KPHP_DEST_OBJS_DIR=[" << get_dest_objs_dir() << "]\n";

  if (is_static_lib_mode()) {
    std::cerr << "KPHP_OUT_LIB_DIR=[" << get_static_lib_out_dir() << "]\n";
  }

  std::cerr << "CXX=[" << get_cxx() << "]\n" <<
            "CXXFLAGS=[" << get_cxx_flags() << "]\n" <<
            "LDFLAGS=[" << get_ld_flags() << "]\n" <<
            "AR=[" << get_ar() << "]\n" <<
            "KPHP_INCREMENTAL_LINKER=[" << get_incremental_linker() << "]\n" <<
            "KPHP_INCREMENTAL_LINKER_FLAGS=[" << get_incremental_linker_flags() << "]\n";
  std::cerr << "KPHP_INCLUDES=[";
  bool is_first = true;
  for (const auto &include : get_includes()) {
    if (is_first) {
      is_first = false;
    } else {
      std::cerr << ";";
    }
    std::cerr << include;
  }
  std::cerr << "]\n";
  std::cerr << "KPHP_MAIN_FILES=[";
  is_first = true;
  for (const auto &main_file : get_main_files()) {
    if (is_first) {
      is_first = false;
    } else {
      std::cerr << ";";
    }
    std::cerr << main_file;
  }
  std::cerr << "]\n";
}

std::string KphpEnviroment::read_runtime_sha256_file(const std::string &filename) {
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

KphpEnviroment::color_settings KphpEnviroment::get_color_settings() const {
  return color_;
}

void KphpEnviroment::set_php_code_version(std::string &&version_tag) {
  php_code_version_ = std::move(version_tag);
}

const std::string &KphpEnviroment::get_php_code_version() const {
  return php_code_version_;
}

const std::string &KphpEnviroment::get_tl_namespace_prefix() const {
  static std::string tl_namespace_name("VK\\TL\\");
  return tl_namespace_name;
}

const std::string &KphpEnviroment::get_tl_classname_prefix() const {
  static std::string tl_classname_prefix("C$VK$TL$");
  return tl_classname_prefix;
}

void KphpEnviroment::set_gen_tl_internals() {
  gen_tl_internals_ = true;
}

bool KphpEnviroment::get_gen_tl_internals() const {
  return gen_tl_internals_;
}
