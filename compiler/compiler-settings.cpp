// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/compiler-settings.h"

#include <fstream>
#include <openssl/sha.h>
#include <sstream>
#include <unistd.h>

#include "common/algorithms/contains.h"
#include "common/algorithms/find.h"
#include "common/version-string.h"
#include "common/wrappers/fmt_format.h"
#include "common/wrappers/mkdir_recursive.h"
#include "common/wrappers/to_array.h"

#include "compiler/stage.h"
#include "compiler/threading/tls.h"
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
void KphpOption<std::string>::parse_arg_value() {
  // Don't move it, it can be used later
  value_ = raw_option_arg_;
}

template<>
void KphpOption<uint64_t>::parse_arg_value() {
  if (raw_option_arg_.empty()) {
    value_ = 0;
  } else {
    try {
      if (raw_option_arg_[0] == '-') {
        throw std::out_of_range{""};
      }
      value_ = std::stoul(raw_option_arg_);
    } catch (...) {
      throw_param_exception(fmt_format("unsigned integer is expected but {} found", raw_option_arg_));
    }
  }
}

template<>
void KphpOption<bool>::parse_arg_value() {
  if (vk::none_of_equal(raw_option_arg_, "1", "0", "")) {
    throw_param_exception("'0' or '1' are expected");
  }
  value_ = raw_option_arg_ == "1";
}

template<>
void KphpOption<std::vector<std::string>>::parse_arg_value() {
  value_ = split(raw_option_arg_, ':');
}

namespace {

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

void append_curl(std::string &cxx_flags, std::string &ld_flags) noexcept {
  if (!contains_lib(ld_flags, "curl")) {
    // TODO make it as an option?
    const std::string curl_dir = "/opt/curl7600";
    cxx_flags += " -I" + curl_dir + "/include/";
    ld_flags += " " + curl_dir + "/lib/libcurl.a";
  }
}

} // namespace

void CompilerSettings::option_as_dir(KphpOption<std::string> &path_option) noexcept {
  path_option.value_ = as_dir(path_option.value_);
}

bool CompilerSettings::is_static_lib_mode() const {
  return mode.get() == "lib";
}

bool CompilerSettings::is_server_mode() const {
  return mode.get() == "server";
}

bool CompilerSettings::is_cli_mode() const {
  return mode.get() == "cli";
}

bool CompilerSettings::is_composer_enabled() const {
  return !composer_root.get().empty();
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

  std::string hash_str;
  hash_str.reserve(SHA256_DIGEST_LENGTH * 2);
  for (auto hash_symb : hash) {
    fmt_format_to(std::back_inserter(hash_str), "{:02x}", hash_symb);
  }
  cxx_flags_sha256.value_ = std::move(hash_str);
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
    lib_dir = as_dir(lib_dir);
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
    jobs_count.value_ = get_default_threads_count();
  }
  if (!threads_count.get()) {
    threads_count.value_ = get_default_threads_count();
  } else if (threads_count.get() > MAX_THREADS_COUNT) {
    throw std::runtime_error{"Option " + threads_count.get_env_var() + " is expected to be <= " + std::to_string(MAX_THREADS_COUNT)};
  }

  if (globals_split_count.get() == 0) {
    throw std::runtime_error{"globals-split-count may not be equal to zero"};
  }

  for (std::string &include : includes.value_) {
    include = as_dir(include);
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

  remove_extra_spaces(extra_cxx_flags.value_);
  std::stringstream ss;
  ss << extra_cxx_flags.get();
  ss << " -iquote" << kphp_src_path.get()
     << " -iquote " << kphp_src_path.get() << "objs/generated/auto/runtime";
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
  #if __cplusplus <= 201402L
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

  ld_flags.value_ = extra_ld_flags.get();
  append_curl(cxx_flags.value_, ld_flags.value_);

  auto external_libs = {"pthread", "rt", "crypto", "m"};
  auto external_static_libs = {"vk-flex-data", "pcre", "re2", "yaml-cpp", "h3", "ssl", "z", "zstd", "nghttp2"};
  append_if_doesnt_contain(ld_flags.value_, external_libs, "-l");
  append_if_doesnt_contain(ld_flags.value_, external_static_libs, "-l:lib", ".a");

  ld_flags.value_ += " -rdynamic";

  for (auto &main_file : main_files.value_) {
    auto full_path = get_full_path(main_file);
    if (full_path.empty()) {
      // get_full_path() calls realpath() which will set the errno in case of failure
      throw std::runtime_error{fmt_format("Failed to open file [{}] : {}", main_file, strerror(errno))};
    }
    main_file = full_path;
  }

  const auto &first_main_file = main_files.get().front();
  const size_t dir_pos = first_main_file.rfind('/');
  kphp_assert (dir_pos != std::string::npos);
  base_dir.value_ = first_main_file.substr(0, dir_pos + 1);

  mkdir_recursive(dest_dir.get().c_str(), 0777);
  option_as_dir(dest_dir);
  dest_cpp_dir.value_ = dest_dir.get() + "kphp/";
  dest_objs_dir.value_ = dest_dir.get() + "objs/";
  binary_path.value_ = dest_dir.get() + mode.get();
  generated_runtime_path.value_ = kphp_src_path.get() + "objs/generated/auto/runtime/";
  cxx_flags.value_ += " -iquote" + dest_cpp_dir.get();

  tl_namespace_prefix.value_ = "VK\\TL\\";
  tl_classname_prefix.value_ = "C$VK$TL$";

  option_as_dir(composer_root);
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
