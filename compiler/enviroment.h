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

  string base_dir_;
  string index_;
  vector<string> includes_;
  string jobs_count_;
  int jobs_count_int_{0};
  string use_make_;
  bool use_make_bool_{false};
  string make_force_;
  bool make_force_bool_{false};
  string threads_count_;
  int threads_count_int_{0};
  string verbosity_;
  int verbosity_int_{0};
  vector<string> main_files_;
  int print_resumable_graph_{0};
  int enable_profiler_{0};
  string tl_schema_file_;
  bool no_pch_{false};
  bool stop_on_type_error_{true};
  bool show_progress_{true};

  string cxx_;
  string cxx_flags_;
  string ld_;
  string ld_flags_;
  string ar_;

  string dest_dir_;
  string use_auto_dest_;
  bool use_auto_dest_bool_{false};
  string dest_cpp_dir_;
  string dest_objs_dir_;
  string binary_path_;
  string user_binary_path_;
  bool error_on_warns_{false};

  string warnings_filename_;
  FILE *warnings_file_{nullptr};

  string stats_filename_;
  FILE *stats_file_{nullptr};
  int warnings_level_{0};
  string debug_level_;
  string version_;

  string cxx_flags_sha256_;
  color_settings color_{auto_colored};

  void update_cxx_flags_sha256();

public:
  const string &get_home() const;
  void set_base_dir(const string &base_dir);
  const string &get_base_dir() const;
  void set_dest_dir(const string &dest_dir);
  const string &get_dest_dir() const;
  void set_use_auto_dest(const string &use_auto_dest);
  bool get_use_auto_dest() const;
  void set_functions(const string &functions);
  const string &get_functions() const;
  void set_index(const string &index);
  const string &get_index() const;
  void add_include(const string &include);
  const vector<string> &get_includes() const;
  void set_jobs_count(const string &jobs_count);
  int get_jobs_count() const;
  void set_mode(const string &mode);
  const string &get_mode() const;
  void set_link_file(const string &link_file);
  const string &get_link_file() const;
  void set_use_make(const string &use_make);
  bool get_use_make() const;
  void set_make_force(const string &make_force);
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
  void inc_verbosity();
  int get_verbosity() const;
  void set_print_resumable_graph();
  int get_print_resumable_graph() const;
  void set_enable_profiler();
  int get_enable_profiler() const;
  void set_error_on_warns();
  bool get_error_on_warns() const;
  void set_tl_schema_file(const string &tl_schema_file);
  string get_tl_schema_file() const;
  void set_no_pch();
  bool get_no_pch() const;
  bool get_stop_on_type_error() const;
  bool get_show_progress() const;
  void add_main_file(const string &main_file);
  const vector<string> &get_main_files() const;

  void set_dest_dir_subdir(const string &s);
  void init_dest_dirs();

  void set_warnings_filename(const string &path);
  void set_stats_filename(const string &path);
  void set_stats_file(FILE *file);
  void set_warnings_file(FILE *file);
  void set_warnings_level(int level);
  void set_debug_level(const string &level);

  const string &get_warnings_filename() const;
  const string &get_stats_filename() const;
  FILE *get_stats_file() const;
  FILE *get_warnings_file() const;
  int get_warnings_level() const;
  const string &get_debug_level() const;

  const string &get_dest_cpp_dir() const;
  const string &get_dest_objs_dir() const;

  const string &get_cxx() const;
  const string &get_cxx_flags() const;
  const string &get_ld() const;
  const string &get_ld_flags() const;
  const string &get_ar() const;

  const string &get_version() const;
  bool is_static_lib_mode() const;
  color_settings get_color_settings() const;

  bool init();
  void debug() const;

  static std::string read_runtime_sha256_file(const std::string &filename);

};

