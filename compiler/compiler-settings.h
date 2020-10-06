#pragma once

#include "compiler/common.h"
#include "common/mixin/not_copyable.h"
#include "common/wrappers/string_view.h"

struct KphpRawOption : vk::not_copyable {
public:
  const std::string &get_option_full_name() const noexcept {
    return cmd_option_full_name_;
  }

  void init(vk::string_view long_option, char short_option, vk::string_view env,
            vk::string_view default_value, std::vector<std::string> choices) noexcept;

  void substitute_depends(const KphpRawOption &other) noexcept;
  void verify_arg_value() const;

  virtual void set_option_arg_value(const char *optarg_value) noexcept = 0;
  virtual void parse_arg_value() noexcept = 0;
  virtual ~KphpRawOption() = default;

protected:
  void throw_param_exception(const std::string &reason) const;

  std::string env_var_;
  std::string cmd_option_full_name_;
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
  using KphpRawOption::get_option_full_name;
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

  void parse_arg_value() noexcept final;

  T value_{};
};

class CompilerSettings : vk::not_copyable {
public:
  enum color_settings {
    auto_colored,
    not_colored,
    colored
  };

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

  KphpOption<bool> force_make;
  KphpOption<bool> use_make;
  KphpOption<uint64_t> jobs_count;
  KphpOption<uint64_t> threads_count;

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
  KphpOption<std::string> extra_cxx_flags;
  KphpOption<std::string> extra_ld_flags;
  KphpOption<std::string> debug_level;
  KphpOption<std::string> archive_creator;
  KphpOption<bool> dynamic_incremental_linkage;

  KphpOption<uint64_t> profiler_level;
  KphpOption<bool> enable_global_vars_memory_stats;
  KphpOption<bool> print_resumable_graph;

  KphpOption<bool> no_pch;
  KphpOption<bool> no_index_file;
  KphpOption<bool> hide_progress;

  static std::string get_home() noexcept;

private:
  static void option_as_dir(KphpOption<std::string> &path) noexcept;

  std::vector<std::string> main_files_;

  std::string cxx_flags_;
  std::string ld_flags_;
  std::string incremental_linker_;
  std::string incremental_linker_flags_;

  std::string dest_cpp_dir_;
  std::string dest_objs_dir_;
  std::string binary_path_;
  std::string static_lib_name_;

  std::string runtime_sha256_;
  std::string cxx_flags_sha256_;

  color_settings color_{auto_colored};

  void update_cxx_flags_sha256();

public:
  const std::string &get_binary_path() const;
  const std::string &get_static_lib_name() const;
  const std::string &get_runtime_sha256() const;
  const std::string &get_cxx_flags_sha256() const;

  void add_main_file(const std::string &main_file);
  const std::vector<std::string> &get_main_files() const;

  void set_dest_dir_subdir(const std::string &s);
  void init_dest_dirs();

  const std::string &get_dest_cpp_dir() const;
  const std::string &get_dest_objs_dir() const;

  const std::string &get_cxx_flags() const;
  const std::string &get_ld_flags() const;
  const std::string &get_incremental_linker() const;
  const std::string &get_incremental_linker_flags() const;

  std::string get_version() const;
  bool is_static_lib_mode() const;
  color_settings get_color_settings() const;

  const std::string &get_tl_namespace_prefix() const;
  const std::string &get_tl_classname_prefix() const;

  void init();
  void debug() const;

  static std::string read_runtime_sha256_file(const std::string &filename);

};

