// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <string>
#include <tuple>
#include <vector>

#include "common/mixin/not_copyable.h"
#include "common/wrappers/string_view.h"

struct KphpRawOption : vk::not_copyable {
public:
  const std::string &get_env_var() const noexcept {
    return env_var_;
  }

  void init(const char *env, std::string default_value, std::vector<std::string> choices) noexcept;

  void substitute_depends(const KphpRawOption &other) noexcept;
  void verify_arg_value() const;

  virtual void set_option_arg_value(const char *optarg_value) noexcept = 0;
  virtual void parse_arg_value() = 0;
  virtual void dump_option(std::ostream &out) const noexcept = 0;
  virtual ~KphpRawOption() = default;

protected:
  void throw_param_exception(const std::string &reason) const;

  std::string env_var_;
  std::string raw_option_arg_;
  std::vector<std::string> choices_;
};

template<class T>
class KphpOption final : public KphpRawOption {
public:
  friend class CompilerSettings;

  const T &get() const noexcept {
    return value_;
  }

private:
  using KphpRawOption::get_env_var;
  using KphpRawOption::init;
  using KphpRawOption::substitute_depends;
  using KphpRawOption::verify_arg_value;

  KphpOption &set_default(std::string &&default_opt) noexcept {
    if (raw_option_arg_.empty()) {
      raw_option_arg_ = std::move(default_opt);
    }
    return *this;
  }

  void set_option_arg_value(const char *optarg_value) noexcept final {
    if (std::is_same<T, bool>{}) {
      raw_option_arg_ = "1";
    } else if (std::is_same<T, std::vector<std::string>>{} && !raw_option_arg_.empty()) {
      raw_option_arg_.append(":").append(optarg_value);
    } else {
      raw_option_arg_ = optarg_value;
    }
  }

  void dump_option(std::ostream &out) const noexcept final;
  void parse_arg_value() final;

  T value_{};
};

class KphpImplicitOption : vk::not_copyable {
public:
  friend class CompilerSettings;
  friend class CxxFlags;

  const std::string &get() const noexcept {
    return value_;
  }

private:
  std::string value_;
};

class CxxFlags {
public:
  void init(const std::string &runtime_sha256, const std::string &cxx, std::string cxx_flags_line, const std::string &dest_cpp_dir, bool enable_pch) noexcept;

  KphpImplicitOption flags;
  KphpImplicitOption flags_sha256;
  KphpImplicitOption pch_dir;

  friend inline bool operator==(const CxxFlags &lhs, const CxxFlags &rhs) noexcept {
    return std::tie(lhs.flags.get(), lhs.flags_sha256.get(), lhs.pch_dir.get()) == std::tie(rhs.flags.get(), rhs.flags_sha256.get(), rhs.pch_dir.get());
  }
};

class CompilerSettings : vk::not_copyable {
public:
  enum color_settings {
    auto_colored,
    not_colored,
    colored
  };

  KphpOption<std::string> main_file;

  KphpOption<uint64_t> verbosity;

  KphpOption<std::string> kphp_src_path;
  KphpOption<std::string> functions_file;
  KphpOption<std::string> runtime_sha256_file;
  KphpOption<std::string> mode;
  KphpOption<std::string> link_file;
  KphpOption<std::vector<std::string>> includes;

  KphpOption<std::string> dest_dir;
  KphpOption<std::string> user_binary_path;
  KphpOption<std::string> static_lib_out_dir;

  KphpOption<std::string> composer_root;
  KphpOption<bool> composer_autoload_dev;

  KphpOption<bool> force_make;
  KphpOption<bool> no_make;
  KphpOption<uint64_t> jobs_count;
  KphpOption<uint64_t> threads_count;
  KphpOption<uint64_t> globals_split_count;

  KphpOption<bool> require_functions_typing;
  KphpOption<bool> require_class_typing;

  KphpOption<std::string> tl_schema_file;
  KphpOption<bool> gen_tl_internals;

  KphpOption<bool> error_on_warns;
  KphpOption<std::string> warnings_file;
  KphpOption<uint64_t> warnings_level;
  KphpOption<bool> show_all_type_errors;
  KphpOption<std::string> colorize;

  KphpOption<std::string> stats_file;
  KphpOption<std::string> compilation_metrics_file;
  KphpOption<std::string> override_kphp_version;
  KphpOption<std::string> php_code_version;

  KphpOption<std::string> cxx;
  KphpOption<std::string> cxx_toolchain_dir;
  KphpOption<std::string> extra_cxx_flags;
  KphpOption<std::string> extra_ld_flags;
  KphpOption<std::string> extra_cxx_debug_level;
  KphpOption<std::string> archive_creator;
  KphpOption<bool> dynamic_incremental_linkage;

  KphpOption<uint64_t> profiler_level;
  KphpOption<bool> enable_global_vars_memory_stats;
  KphpOption<bool> enable_full_performance_analyze;
  KphpOption<bool> print_resumable_graph;

  KphpOption<bool> no_pch;
  KphpOption<bool> no_index_file;
  KphpOption<bool> show_progress;

  CxxFlags cxx_flags_default;
  CxxFlags cxx_flags_with_debug;
  KphpImplicitOption ld_flags;
  KphpImplicitOption incremental_linker_flags;

  KphpImplicitOption base_dir;
  KphpImplicitOption dest_cpp_dir;
  KphpImplicitOption dest_objs_dir;
  KphpImplicitOption binary_path;
  KphpImplicitOption static_lib_name;
  KphpImplicitOption generated_runtime_path;
  KphpImplicitOption performance_analyze_report_path;
  KphpImplicitOption cxx_toolchain_option;

  KphpImplicitOption runtime_headers;
  KphpImplicitOption runtime_sha256;

  KphpImplicitOption tl_namespace_prefix;
  KphpImplicitOption tl_classname_prefix;

  std::string get_version() const;
  bool is_static_lib_mode() const;
  bool is_server_mode() const;
  bool is_cli_mode() const;
  bool is_composer_enabled() const; // reports whether composer compatibility mode is on
  color_settings get_color_settings() const;

  void init();

  static std::string read_runtime_sha256_file(const std::string &filename);

private:
  static void option_as_dir(KphpOption<std::string> &path) noexcept;

  color_settings color_{auto_colored};
};
