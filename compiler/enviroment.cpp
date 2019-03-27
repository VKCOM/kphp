#include "compiler/enviroment.h"

#include <fstream>
#include <openssl/sha.h>

#include "common/version-string.h"

#include "compiler/stage.h"
#include "compiler/threading/format.h"
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

static void as_file(string *s __attribute__((unused))) {
  return;
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

void env_str2bool(bool *dest, const string &src) {
  *dest = static_cast <bool> (atoi(src.c_str()));
}

const string &KphpEnviroment::get_home() const {
  return home_;
}

void KphpEnviroment::set_use_safe_integer_arithmetic(const string &flag) {
  use_safe_integer_arithmetic_ = flag;
}

bool KphpEnviroment::get_use_safe_integer_arithmetic() const {
  return use_safe_integer_arithmetic_bool_;
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

void KphpEnviroment::set_use_auto_dest(const string &use_auto_dest) {
  use_auto_dest_ = use_auto_dest;
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

void KphpEnviroment::set_index(const string &index) {
  index_ = index;
}

const string &KphpEnviroment::get_index() const {
  return index_;
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

void KphpEnviroment::set_use_make(const string &use_make) {
  use_make_ = use_make;
}

bool KphpEnviroment::get_use_make() const {
  return use_make_bool_;
}

void KphpEnviroment::set_make_force(const string &make_force) {
  make_force_ = make_force;
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

bool KphpEnviroment::get_stop_on_type_error() const {
  return stop_on_type_error_;
}

void KphpEnviroment::add_main_file(const string &main_file) {
  main_files_.push_back(main_file);
}

const vector<string> &KphpEnviroment::get_main_files() const {
  return main_files_;
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

const string &KphpEnviroment::get_ld() const {
  return ld_;
}

const string &KphpEnviroment::get_ld_flags() const {
  return ld_flags_;
}

const string &KphpEnviroment::get_ar() const {
  return ar_;
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

void KphpEnviroment::set_stats_filename(const string &path) {
  stats_filename_ = path;
}

const string &KphpEnviroment::get_warnings_filename() const {
  return warnings_filename_;
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
    sprintf(hash_str + (i * 2), "%02x", hash[i]);
  }
  cxx_flags_sha256_.assign(hash_str, SHA256_DIGEST_LENGTH);
}

bool KphpEnviroment::init() {
  char tmp[PATH_MAX];
  char *cur_dir = getcwd(tmp, PATH_MAX);
  if (cur_dir == nullptr) {
    printf("Failed to get current directory");
    return false;
  }
  cur_dir_ = cur_dir;
  as_dir(&cur_dir_);
  home_ = getenv("HOME");
  as_dir(&home_);
  init_env_var(&path_, "KPHP_PATH", get_home() + "engine/src/");
  as_dir(&path_);
  init_env_var(&functions_, "KPHP_FUNCTIONS", get_path() + "PHP/functions.txt");
  as_file(&functions_);
  init_env_var(&mode_, "KPHP_MODE", "server");
  if (mode_ == "net") {
    mode_ = "server";
  }
  string link_file_name;
  if (mode_ == "server") {
    link_file_name = "php-server.a";
  } else if (mode_ == "cli") {
    link_file_name = "php-cli.a";
  } else if (!is_static_lib_mode()) {
    printf("Unknown $KPHP_MODE=%s\n", mode_.c_str());
    return false;
  }

  if (is_static_lib_mode()) {
    if (main_files_.size() > 1) {
      printf("Multiple main directories are forbidden for static lib mode");
      return false;
    }
    if (!tl_schema_file_.empty()) {
      printf("tl-schema is forbidden for static lib mode");
      return false;
    }
    std::string lib_dir = get_full_path(main_files_.back());
    std::size_t last_slash = lib_dir.rfind('/');
    if (last_slash == std::string::npos) {
      printf("Bad lib directory");
      return false;
    }
    static_lib_name_ = lib_dir.substr(last_slash + 1);
    if (static_lib_name_.empty()) {
      printf("Got empty static lib name");
      return false;
    }
    as_dir(&lib_dir);
    add_include(lib_dir + "php/");
    init_env_var(&static_lib_out_dir_, "KPHP_OUT_LIB_DIR", lib_dir + "lib/");
    as_dir(&static_lib_out_dir_);
    as_file(&main_files_.back().assign(lib_dir).append("/php/index.php"));
  }
  else {
    if (!static_lib_out_dir_.empty()) {
      printf("Output dir is allowed only for static lib mode");
      return false;
    }
    init_env_var(&link_file_, "KPHP_LINK_FILE", get_path() + "objs/PHP/" + link_file_name);
    as_file(&link_file_);
  }

  init_env_var(&runtime_sha256_filename_, "KPHP_RUNTIME_SHA256", get_path() + "objs/PHP/php_lib_version.sha256");
  as_file(&runtime_sha256_filename_);

  init_env_var(&use_safe_integer_arithmetic_, "KPHP_USE_SAFE_INTEGER_ARITHMETIC", "0");
  env_str2bool(&use_safe_integer_arithmetic_bool_, use_safe_integer_arithmetic_);

  init_env_var(&base_dir_, "", "");
  as_dir(&base_dir_);

  init_env_var(&index_, "", "");
  as_file(&index_);

  if (!no_pch_) {
    std::string no_pch;
    init_env_var(&no_pch, "KPHP_NO_PCH", "0");
    env_str2bool(&no_pch_, no_pch);
  }

  init_env_var(&jobs_count_, "KPHP_JOBS_COUNT", "100");
  env_str2int(&jobs_count_int_, jobs_count_);
  if (jobs_count_int_ <= 0) {
    printf("Incorrect jobs_count=%s\n", jobs_count_.c_str());
    return false;
  }

  init_env_var(&use_make_, "KPHP_USE_MAKE", "0");
  env_str2bool(&use_make_bool_, use_make_);

  init_env_var(&make_force_, "", "0");
  env_str2bool(&make_force_bool_, make_force_);

  init_env_var(&threads_count_, "KPHP_THREADS_COUNT", "16");
  env_str2int(&threads_count_int_, threads_count_);
  if (threads_count_int_ <= 0 || threads_count_int_ > 100) {
    printf("Incorrect threads_count=%s\n", threads_count_.c_str());
    return false;
  }

  std::string stop_on_type_error;
  init_env_var(&stop_on_type_error, "KPHP_STOP_ON_TYPE_ERROR", "1");
  env_str2bool(&stop_on_type_error_, stop_on_type_error);

  for (string &include : includes_) {
    as_dir(&include);
  }

  for (string &main_file : main_files_) {
    as_file(&main_file);
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
    fprintf(stderr, "Unknown color settings %s, fallback to auto\n", color_settings_str.c_str());
    color_ = auto_colored;
  }

  string user_cxx_flags;
  init_env_var(&user_cxx_flags, "CXXFLAGS", "-Os -ggdb -march=core2 -mfpmath=sse -mssse3");
  init_env_var(&cxx_, "CXX", "g++");
  stringstream ss;
  ss << user_cxx_flags;
  ss << " -iquote" << get_path() << " -iquote" << get_path() << "PHP/";
  ss << " -Wall -fwrapv -Wno-parentheses -Wno-trigraphs";
  ss << " -fno-exceptions -fno-strict-aliasing -fno-omit-frame-pointer";
  if (!no_pch_) {
    ss << " -Winvalid-pch -fpch-preprocess";
  }

  cxx_flags_ = ss.str();

  update_cxx_flags_sha256();
  runtime_sha256_ = read_runtime_sha256_file(get_runtime_sha256_file());

  init_env_var(&ld_, "LD", "ld");
  string user_ld_flags;
  init_env_var(&user_ld_flags, "LDFLAGS", "-ggdb");
  ld_flags_ = user_ld_flags + " -lm -lz -lpthread -lrt -lcrypto -lpcre -lre2 -rdynamic";

  init_env_var(&ar_, "AR", "ar");

  //todo: use some hash???
  init_env_var(&dest_dir_, "KPHP_DEST_DIR", get_path() + "PHP/tests/kphp_tmp/default/");
  as_dir(&dest_dir_);
  init_env_var(&use_auto_dest_, "KPHP_AUTO_DEST", "0");
  init_env_var(&version_, "KPHP_VERSION_OVERRIDE", get_version_string());
  env_str2bool(&use_auto_dest_bool_, use_auto_dest_);

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
  as_file(&binary_path_);

  as_file(&user_binary_path_);

  cxx_flags_ += " -iquote" + get_dest_cpp_dir();
}

void KphpEnviroment::debug() const {
  std::cerr << "HOME=[" << get_home() << "]\n" <<
            "KPHP_USE_SAFE_INTEGER_ARITHMETIC=[" << get_use_safe_integer_arithmetic() << "]\n" <<
            "KPHP_BASE_DIR=[" << get_base_dir() << "]\n" <<
            "KPHP_DEST_DIR=[" << get_dest_dir() << "]\n" <<
            "KPHP_FUNCTIONS=[" << get_functions() << "]\n" <<
            "KPHP_INDEX=[" << get_index() << "]\n" <<
            "KPHP_JOBS_COUNT=[" << get_jobs_count() << "]\n" <<
            "KPHP_MODE=[" << get_mode() << "]\n" <<
            "KPHP_LINK_FILE=[" << get_link_file() << "]\n" <<
            "KPHP_USE_MAKE=[" << get_use_make() << "]\n" <<
            "KPHP_THREADS_COUNT=[" << get_threads_count() << "]\n" <<
            "KPHP_PATH=[" << get_path() << "]\n" <<
            "KPHP_USER_BINARY_PATH=[" << get_user_binary_path() << "]\n" <<
            "KPHP_RUNTIME_SHA256_FILE=[" << get_runtime_sha256_file() << "]\n" <<
            "KPHP_RUNTIME_SHA256=[" << get_runtime_sha256() << "]\n" <<
            "KPHP_VERBOSITY=[" << get_verbosity() << "]\n" <<
            "KPHP_NO_PCH=[" << get_no_pch() << "]\n" <<
            "KPHP_STOP_ON_TYPE_ERROR=[" << get_stop_on_type_error() << "]\n" <<

            "KPHP_AUTO_DEST=[" << get_use_auto_dest() << "]\n" <<
            "KPHP_BINARY_PATH=[" << get_binary_path() << "]\n" <<
            "KPHP_DEST_CPP_DIR=[" << get_dest_cpp_dir() << "]\n" <<
            "KPHP_DEST_OBJS_DIR=[" << get_dest_objs_dir() << "]\n";

  if (is_static_lib_mode()) {
    std::cerr << "KPHP_OUT_LIB_DIR=[" << get_static_lib_out_dir() << "]\n";
  }

  std::cerr << "CXX=[" << get_cxx() << "]\n" <<
            "CXX_FLAGS=[" << get_cxx_flags() << "]\n" <<
            "LD=[" << get_ld() << "]\n" <<
            "LD_FLAGS=[" << get_ld_flags() << "]\n" <<
            "AR=[" << get_ar() << "]\n";
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
             format("Can't open runtime sha256 file '%s'", filename.c_str()));

  constexpr std::streamsize SHA256_LEN = 64;
  char runtime_sha256[SHA256_LEN] = {0};
  runtime_sha256_file.read(runtime_sha256, SHA256_LEN);
  kphp_error(runtime_sha256_file.gcount() == SHA256_LEN,
             format("Can't read runtime sha256 from file '%s'", filename.c_str()));
  return std::string(runtime_sha256, runtime_sha256 + SHA256_LEN);
}

KphpEnviroment::color_settings KphpEnviroment::get_color_settings() const {
  return color_;
}
