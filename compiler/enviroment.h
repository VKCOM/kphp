#pragma once

#include "compiler/common.h"

class KphpEnviroment {
public:
  enum color_settings { auto_colored, not_colored, colored};
private:
  string cur_dir_;
  string home_;
  string path_;
  string functions_;
  string mode_;
  string link_file_;
  string runtime_sha256_filename_;
  string runtime_sha256_;
  string static_lib_out_dir_;
  string static_lib_name_;

  bool no_index_file_{false};
  vector<string> includes_;
  string jobs_count_;
  int jobs_count_int_{0};
  bool use_make_bool_{false};
  bool make_force_bool_{false};
  string threads_count_;
  int threads_count_int_{0};
  std::string verbosity_str_{0};
  int verbosity_int_{0};
  vector<string> main_files_;
  int print_resumable_graph_{0};
  std::string profiler_level_str_;
  int profiler_level_{0};
  string tl_schema_file_;
  bool gen_tl_internals_{false};
  bool no_pch_{false};
  bool stop_on_type_error_{true};
  bool show_progress_{true};
  bool enable_global_vars_memory_stats_{false};
  bool dynamic_incremental_linkage_{false};

  string cxx_;
  string cxx_flags_;
  string ld_flags_;
  string incremental_linker_;
  string incremental_linker_flags_;

  string ar_;

  string dest_dir_;
  bool use_auto_dest_bool_{false};
  string dest_cpp_dir_;
  string dest_objs_dir_;
  string binary_path_;
  string user_binary_path_;
  bool error_on_warns_{false};

  string warnings_filename_;

  string compilation_metrics_file_;
  string stats_filename_;
  std::string warnings_level_str_;
  int warnings_level_{0};
  string debug_level_;
  string version_;

  string cxx_flags_sha256_;
  color_settings color_{auto_colored};

  std::string php_code_version_;

  void update_cxx_flags_sha256();

public:
  const string &get_home() const;
  void set_dest_dir(const string &dest_dir);
  const string &get_dest_dir() const;
  void set_use_auto_dest();
  bool get_use_auto_dest() const;
  void set_functions(const string &functions);
  const string &get_functions() const;
  void add_include(const string &include);
  const vector<string> &get_includes() const;
  void set_jobs_count(const string &jobs_count);
  int get_jobs_count() const;
  void set_mode(const string &mode);
  const string &get_mode() const;
  void set_link_file(const string &link_file);
  const string &get_link_file() const;
  void set_use_make();
  bool get_use_make() const;
  void set_make_force();
  bool get_make_force() const;
  const string &get_binary_path() const;
  void set_static_lib_out_dir(string &&lib_dir);
  const string &get_static_lib_out_dir() const;
  const string &get_static_lib_name() const;
  void set_user_binary_path(const string &user_binary_path);
  const string &get_user_binary_path() const;
  void set_threads_count(const string &threads_count);
  int get_threads_count() const;
  void set_path(const string &path);
  const string &get_path() const;
  void set_runtime_sha256_file(string &&file_name);
  const string &get_runtime_sha256_file() const;
  const string &get_runtime_sha256() const;
  const string &get_cxx_flags_sha256() const;
  void set_verbosity(std::string &&verbosity);
  int get_verbosity() const;
  void set_print_resumable_graph();
  int get_print_resumable_graph() const;
  void set_profiler_level(string &&level);
  int get_profiler_level() const;
  void set_error_on_warns();
  bool get_error_on_warns() const;
  void set_tl_schema_file(const string &tl_schema_file);
  string get_tl_schema_file() const;
  void set_gen_tl_internals();
  bool get_gen_tl_internals() const;
  void set_no_pch();
  bool get_no_pch() const;
  void set_no_index_file();
  bool get_no_index_file() const;
  bool get_stop_on_type_error() const;
  bool get_show_progress() const;
  void set_enable_global_vars_memory_stats();
  bool get_enable_global_vars_memory_stats() const;
  void set_dynamic_incremental_linkage();
  bool get_dynamic_incremental_linkage() const;

  void add_main_file(const string &main_file);
  const vector<string> &get_main_files() const;

  void set_dest_dir_subdir(const string &s);
  void init_dest_dirs();

  void set_warnings_filename(const string &path);
  void set_compilation_metrics_filename(string &&path);
  void set_stats_filename(const string &path);
  void set_warnings_level(std::string &&level);
  void set_debug_level(const string &level);

  const string &get_warnings_filename() const;
  const string &get_compilation_metrics_filename() const;
  const string &get_stats_filename() const;
  int get_warnings_level() const;
  const string &get_debug_level() const;

  const string &get_dest_cpp_dir() const;
  const string &get_dest_objs_dir() const;

  const string &get_cxx() const;
  const string &get_cxx_flags() const;
  const string &get_ld_flags() const;
  const string &get_ar() const;
  const string &get_incremental_linker() const;
  const string &get_incremental_linker_flags() const;

  const string &get_version() const;
  bool is_static_lib_mode() const;
  color_settings get_color_settings() const;
  void set_php_code_version(std::string &&version);
  const std::string &get_php_code_version() const;

  const string &get_tl_namespace_prefix() const;
  const string &get_tl_classname_prefix() const;

  bool init();
  void debug() const;

  static std::string read_runtime_sha256_file(const std::string &filename);

};

