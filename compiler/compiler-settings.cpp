#include "compiler/compiler-settings.h"

#include <fstream>
#include <openssl/sha.h>
#include <sstream>
#include <thread>
#include <unistd.h>

#include "common/algorithms/contains.h"
#include "common/algorithms/find.h"
#include "common/version-string.h"
#include "common/wrappers/fmt_format.h"
#include "common/wrappers/to_array.h"

#include "compiler/stage.h"
#include "compiler/utils/string-utils.h"

void KphpRawOption::init(const char *env, std::string default_value, std::vector<std::string> choices) noexcept {
  if (char *val = getenv(env)) {
    raw_option_arg_ = val;
  } else {
    raw_option_arg_ = std::move(default_value);
  }
  env_var_ = env;
  choices_ = std::move(choices);
}

void KphpRawOption::substitute_depends(const KphpRawOption &other) noexcept {
  raw_option_arg_ = vk::replace_all(raw_option_arg_, "${" + other.get_env_var() + "}", other.raw_option_arg_);
}

void KphpRawOption::verify_arg_value() const {
  if (!choices_.empty() && !vk::contains(choices_, raw_option_arg_)) {
    throw_param_exception("choose from " + vk::join(choices_, ", "));
  }
}

void KphpRawOption::throw_param_exception(const std::string &reason) const {
  throw std::runtime_error{"Can't parse " + get_env_var() + " option: " + reason};
}

template<>
void KphpOption<std::string>::dump_option(std::ostream &out) const noexcept {
  out << value_;
}

template<>
void KphpOption<uint64_t>::dump_option(std::ostream &out) const noexcept {
  out << value_;
}

template<>
void KphpOption<bool>::dump_option(std::ostream &out) const noexcept {
  out << (value_ ? "true" : "false");
}

template<>
void KphpOption<std::vector<std::string>>::dump_option(std::ostream &out) const noexcept {
  out << vk::join(value_, ", ");
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

void as_dir(std::string &path) noexcept {
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

bool contains_lib(vk::string_view ld_flags, vk::string_view libname) noexcept {
  const std::string static_lib_name = "lib" + libname + ".a";
  if (vk::contains(ld_flags, static_lib_name)) {
    return true;
  }

  std::string shared_lib_name = "-l" + libname + " ";
  if (vk::contains(ld_flags, shared_lib_name)) {
    return true;
  }
  shared_lib_name.pop_back();
  return ld_flags.ends_with(shared_lib_name);
}

template<class T>
void append_if_doesnt_contain(std::string &ld_flags, const T &libs, const char *prefix, const char *suffix = "") noexcept {
  for (const char *lib : libs) {
    if (!contains_lib(ld_flags, lib)) {
      ld_flags.append(" ").append(prefix).append(lib).append(suffix);
    }
  }
}

} // namespace

void CompilerSettings::option_as_dir(KphpOption<std::string> &path_option) noexcept {
  as_dir(path_option.value_);
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

  auto cxx_flags_full = cxx.get() + cxx_flags.get() + debug_level.get();
  SHA256_Update(&sha256, cxx_flags_full.c_str(), cxx_flags_full.size());

  unsigned char hash[SHA256_DIGEST_LENGTH] = {0};
  SHA256_Final(hash, &sha256);

  char hash_str[SHA256_DIGEST_LENGTH * 2 + 1] = {0};
  for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    fmt_format_to(hash_str + (i * 2), "{:02x}", hash[i]);
  }
  cxx_flags_sha256.value_.assign(hash_str, SHA256_DIGEST_LENGTH * 2);
}

void CompilerSettings::init() {
  option_as_dir(kphp_src_path);
  functions_file.value_ = get_full_path(functions_file.get());
  runtime_sha256_file.value_ = get_full_path(runtime_sha256_file.get());
  link_file.value_ = get_full_path(link_file.get());

  if (is_static_lib_mode()) {
    if (main_files.get().size() > 1) {
      throw std::runtime_error{"Multiple main directories are forbidden for static lib mode"};
    }
    if (!tl_schema_file.get().empty()) {
      throw std::runtime_error{"Option " + tl_schema_file.get_env_var() + " is forbidden for static lib mode"};
    }
    std::string lib_dir = get_full_path(main_files.get().back());
    std::size_t last_slash = lib_dir.rfind('/');
    if (last_slash == std::string::npos) {
      throw std::runtime_error{"Bad lib directory"};
    }
    static_lib_name.value_ = lib_dir.substr(last_slash + 1);
    if (static_lib_name.get().empty()) {
      throw std::runtime_error{"Empty static lib name"};
    }
    as_dir(lib_dir);
    includes.value_.emplace_back(lib_dir + "php/");
    if (static_lib_out_dir.get().empty()) {
      static_lib_out_dir.value_ = lib_dir + "lib/";
    }

    option_as_dir(static_lib_out_dir);
    main_files.value_.back().assign(lib_dir).append("/php/index.php");
  } else if (!static_lib_out_dir.get().empty()) {
    throw std::runtime_error{"Option " + static_lib_out_dir.get_env_var() + " is allowed only for static lib mode"};
  }

  if (!jobs_count.get()) {
    jobs_count.value_ = std::max(std::thread::hardware_concurrency(), 1U);
  }
  if (!threads_count.get()) {
    threads_count.value_ = std::max(std::thread::hardware_concurrency(), 1U);
  }

  for (std::string &include : includes.value_) {
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

  // TODO REMOVE IT!
  if (const char *deprecated_cxx = getenv("CXX")) {
    cxx.value_ = deprecated_cxx;
  }

  // TODO REMOVE IT!
  if (const char *deprecated_cxx_flags = getenv("CXXFLAGS")) {
    extra_cxx_flags.value_ = deprecated_cxx_flags;
  }

  // TODO REMOVE IT!
  if (const char *deprecated_ld_flags = getenv("LDFLAGS")) {
    extra_ld_flags.value_ = deprecated_ld_flags;
  }

  remove_extra_spaces(extra_cxx_flags.value_);
  std::stringstream ss;
  ss << extra_cxx_flags.get();
  ss << " -iquote" << kphp_src_path.get()
     << " -iquote" << kphp_src_path.get() << ""
     << " -iquote " << kphp_src_path.get() << "objs/generated";
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

  cxx_flags.value_ = ss.str();

  update_cxx_flags_sha256();
  runtime_sha256.value_ = read_runtime_sha256_file(runtime_sha256_file.get());

  incremental_linker.value_ = dynamic_incremental_linkage.get() ? cxx.get() : "ld";
  incremental_linker_flags.value_ = dynamic_incremental_linkage.get() ? "-shared" : "-r";

  remove_extra_spaces(extra_ld_flags.value_);

  auto external_shared_libs = {"pthread", "rt", "crypto", "m", "dl", "z", "pcre", "re2", "yaml-cpp", "h3", "ssl", "zstd", "lzma", "curl"};
  auto external_static_libs = {"vk-flex-data"};
  ld_flags.value_ = extra_ld_flags.get();
  append_if_doesnt_contain(ld_flags.value_, external_shared_libs, "-l");
  append_if_doesnt_contain(ld_flags.value_, external_static_libs, "-l:lib", ".a");

  ld_flags.value_ += " -rdynamic";

  option_as_dir(dest_dir);
  dest_cpp_dir.value_ = dest_dir.get() + "kphp/";
  dest_objs_dir.value_ = dest_dir.get() + "objs/";
  binary_path.value_ = dest_dir.get() + mode.get();
  cxx_flags.value_ += " -iquote" + dest_cpp_dir.get();

  tl_namespace_prefix.value_ = "VK\\TL\\";
  tl_classname_prefix.value_ = "C$VK$TL$";
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
