// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/make/make.h"

#include <forward_list>
#include <queue>
#include <unordered_map>
#include <dirent.h>

#include "common/wrappers/mkdir_recursive.h"
#include "common/wrappers/pathname.h"

#include "compiler/compiler-core.h"
#include "compiler/data/lib-data.h"
#include "compiler/data/ffi-data.h"
#include "compiler/make/cpp-to-obj-target.h"
#include "compiler/make/file-target.h"
#include "compiler/make/h-to-pch-target.h"
#include "compiler/make/hardlink-or-copy.h"
#include "compiler/make/make-runner.h"
#include "compiler/make/objs-to-bin-target.h"
#include "compiler/make/objs-to-obj-target.h"
#include "compiler/make/objs-to-k2-component-target.h"
#include "compiler/make/objs-to-static-lib-target.h"
#include "compiler/stage.h"
#include "compiler/threading/profiler.h"

class MakeSetup {
private:
  MakeRunner make;
  const CompilerSettings &settings;

  void target_set_file(Target *target, File *file) {
    assert (file->target == nullptr);
    target->set_file(file);
    file->target = target;
  }

  void target_set_env(Target *target) {
    target->set_settings(&settings);
  }

  Target *to_target(File *file) {
    if (file->target != nullptr) {
      return file->target;
    }
    return create_cpp_target(file);
  }

  std::vector<Target *> to_targets(File *file) {
    return {to_target(file)};
  }

  std::vector<Target *> to_targets(std::vector<File *> files) {
    std::vector<Target *> res(files.size());
    for (int i = 0; i < (int)files.size(); i++) {
      res[i] = to_target(files[i]);
    }
    return res;
  }

  Target *create_target(Target *target, std::vector<Target *> &&deps, File *file) {
    target_set_file(target, file);
    target_set_env(target);
    make.register_target(target, std::move(deps));
    return target;
  }

public:
  MakeSetup(FILE *stats_file, const CompilerSettings &compiler_settings) noexcept:
    make(stats_file),
    settings(compiler_settings) {
  }

  Target *create_cpp_target(File *cpp) {
    return create_target(new FileTarget(), std::vector<Target *>(), cpp);
  }

  Target *create_cpp2obj_target(File *cpp, File *obj) {
    return create_target(new Cpp2ObjTarget(), to_targets(cpp), obj);
  }

  Target *create_h2pch_target(File *header_h, File *pch) {
    return create_target(new H2PchTarget(), to_targets(header_h), pch);
  }

  Target *create_objs2obj_target(std::vector<File *> objs, File *obj) {
    return create_target(new Objs2ObjTarget(), to_targets(std::move(objs)), obj);
  }

  Target *create_objs2bin_target(std::vector<File *> objs, File *bin) {
    bool need_libdl = !G->get_ffi_root().get_shared_libs().empty();
    return create_target(new Objs2BinTarget(need_libdl), to_targets(std::move(objs)), bin);
  }

  Target *create_objs2static_lib_target(std::vector<File *> objs, File *lib) {
    return create_target(new Objs2StaticLibTarget, to_targets(std::move(objs)), lib);
  }

  Target *create_objs2k2_component_target(std::vector<File *> objs, File *lib) {
    return create_target(new Objs2K2ComponentTarget, to_targets(std::move(objs)), lib);
  }

  bool make_target(File *bin, const std::string &build_message, int jobs_count) {
    return make.make_targets(to_targets(bin), build_message, jobs_count);
  }

  bool make_targets(const std::vector<File *> &bins, const std::string &build_message, int jobs_count) {
    return make.make_targets(to_targets(bins), build_message, jobs_count);
  }
};


static void copy_static_lib_to_out_dir(File &&static_archive) {
  Index out_dir;
  out_dir.set_dir(G->settings().static_lib_out_dir.get());
  out_dir.del_extra_files();    // todo seems that this invocation does nothing, as files are empty

  // copy static archive
  LibData out_lib(G->settings().static_lib_name.get(), out_dir.get_dir());
  hard_link_or_copy(static_archive.path, out_lib.static_archive_path());
  static_archive.unlink();

  // copy functions.txt of this static archive
  File functions_txt_tmp(G->settings().dest_cpp_dir.get() + LibData::functions_txt_tmp_file());
  hard_link_or_copy(functions_txt_tmp.path, out_lib.functions_txt_file());
  functions_txt_tmp.unlink();

  // copy runtime lib sha256 of this static archive
  File runtime_lib_sha256(G->settings().runtime_sha256_file.get());
  hard_link_or_copy(runtime_lib_sha256.path, out_lib.runtime_lib_sha256_file());

  Index headers_tmp_dir;
  headers_tmp_dir.sync_with_dir(G->settings().dest_cpp_dir.get() + LibData::headers_tmp_dir());
  Index out_headers_dir;
  out_headers_dir.set_dir(out_lib.headers_dir());
  // copy cpp header files of this static archive
  for (File *header_file: headers_tmp_dir.get_files()) {
    hard_link_or_copy(header_file->path, out_headers_dir.get_dir() + header_file->name);
    header_file->unlink();
  }
}

static std::forward_list<File *> collect_imported_libs() {
  const std::string &binary_runtime_sha256 = G->settings().runtime_sha256.get();
  stage::die_if_global_errors();

  std::forward_list<File *> imported_libs;
  imported_libs.emplace_front(new File{G->settings().link_file.get()});
  for (const auto &lib: G->get_libs()) {
    if (lib && !lib->is_raw_php()) {
      std::string lib_runtime_sha256 = CompilerSettings::read_runtime_sha256_file(lib->runtime_lib_sha256_file());

      kphp_error_act(binary_runtime_sha256 == lib_runtime_sha256,
                     fmt_format("Mismatching between sha256 of binary runtime '{}' and lib runtime '{}'",
                                G->settings().runtime_sha256_file.get(), lib->runtime_lib_sha256_file()),
                     continue);
      imported_libs.emplace_front(new File{lib->static_archive_path()});
    }
  }

  for (File *file: imported_libs) {
    kphp_error_act(file->read_stat() > 0, fmt_format("Can't read mtime of link_file [{}]", file->path), continue);
    if (G->settings().verbosity.get() >= 1) {
      fmt_fprintf(stderr, "Use static lib [{}]\n", file->path);
    }
  }

  stage::die_if_global_errors();
  return imported_libs;
}

static long long get_imported_header_mtime(const std::string &header_path, const std::forward_list<Index> &imported_headers) {
  for (const Index &lib_headers_dir: imported_headers) {
    if (File *header = lib_headers_dir.get_file(header_path)) {
      return header->mtime;
    }
  }
  kphp_error(false, fmt_format("Can't file lib header file '{}'", header_path));
  return 0;
}

// todo:k2 review pch-related things for
// k2-component mode

// prepare dir kphp_out/objs/pch_{flags} and make a target runtime-headers.h.gch inside it
// in production, there will be two pch_ folders: with debug symbols and without them
// later on, we'll copy this folder inside /tmp/kphp_pch — that's why if it exists, don't do anything
File *prepare_precompiled_header(Index *obj_dir, MakeSetup &make, File &runtime_headers_h, const CompilerSettings &settings, bool with_debug) {
  const auto &flags = with_debug ? settings.cxx_flags_with_debug : settings.cxx_flags_default;
  if (with_debug && flags == settings.cxx_flags_default) {
    return nullptr;
  }
  if (access(flags.pch_dir.get().c_str(), F_OK) != -1) {  // if a folder in /tmp exists, this pch was already made
    return nullptr;
  }

  std::string objs_pch_dir = settings.dest_objs_dir.get() + "pch_" + flags.flags_sha256.get() + "/";
  std::string pch_basename = std::string{kbasename(runtime_headers_h.path.c_str())} + ".gch";
  auto *runtime_header_h_pch = obj_dir->insert_file(objs_pch_dir + pch_basename);
  runtime_header_h_pch->compile_with_debug_info_flag = with_debug;
  make.create_h2pch_target(&runtime_headers_h, runtime_header_h_pch);
  return runtime_header_h_pch;
}

// copy dir kphp_out/objs/pch_{flags} inside /tmp/kphp_pch and unlink original
// finally, a folder inside tmp will contain runtime-headers.h and pch folder contents
// note, that a .gch file way even not exist, it's okay: just copy full folder
// (for instance, nocc emits a .nocc-pch file instead of requested .gch)
// todo it's better to use H2PchTarget::after_run_success() for this
// todo why do we need /tmp/kphp_pch at all? why not to just leave pch inside kphp_out/objs?
bool finalize_precompiled_header(File &runtime_headers_h, File &runtime_headers_h_pch, const std::string &dst_pch_dir) {
  const mode_t old_mask = umask(0);
  const bool dir_created = mkdir_recursive(dst_pch_dir.c_str(), 0777);
  umask(old_mask);
  kphp_error_act(dir_created, fmt_format("Can't create tmp dir '{}': {}", dst_pch_dir, strerror(errno)), return false);

  hard_link_or_copy(runtime_headers_h.path, dst_pch_dir + kbasename(runtime_headers_h.path.c_str()), false);
  std::string objs_pch_dir = runtime_headers_h_pch.path.substr(0, runtime_headers_h_pch.path.rfind('/') + 1);
  DIR *objs_pch_dirptr = opendir(objs_pch_dir.c_str());
  while (const auto *entry = readdir(objs_pch_dirptr)) {
    if (entry->d_name[0] == '.') {
      continue;
    }
    std::string from_file_name = objs_pch_dir + entry->d_name;
    std::string to_file_name = dst_pch_dir + entry->d_name;
    hard_link_or_copy(from_file_name, to_file_name, false);
    unlink(from_file_name.c_str());
  }
  closedir(objs_pch_dirptr);
  runtime_headers_h_pch.unlink();
  rmdir(objs_pch_dir.c_str());
  return true;
}

static bool kphp_make_precompiled_headers(Index *obj_dir, const CompilerSettings &settings, FILE *stats_file) {
  MakeSetup make{stats_file, settings};
  File sha256_version_file(settings.runtime_sha256_file.get());
  kphp_assert(sha256_version_file.read_stat() > 0);

  File runtime_headers_h{settings.generated_runtime_path.get() + settings.runtime_headers.get()};
  kphp_assert(runtime_headers_h.read_stat() > 0);
  make.create_cpp_target(&runtime_headers_h);
  runtime_headers_h.target->force_changed(sha256_version_file.mtime);

  std::vector<File *> runtime_header_pch_files;
  std::vector<std::string> pch_dirs;
  for (auto with_debug : {false, true}) {
    if (File *runtime_header_gch_file = prepare_precompiled_header(obj_dir, make, runtime_headers_h, settings, with_debug)) {
      pch_dirs.emplace_back(with_debug ? settings.cxx_flags_with_debug.pch_dir.get() : settings.cxx_flags_default.pch_dir.get());
      runtime_header_pch_files.emplace_back(runtime_header_gch_file);
    }
  }
  if (runtime_header_pch_files.empty()) {
    return true;
  }
  if (!make.make_targets(runtime_header_pch_files, "Compiling pch", settings.jobs_count.get())) {
    return false;
  }

  bool is_ok = true;
  for (size_t i = 0; i != runtime_header_pch_files.size(); ++i) {
    is_ok = finalize_precompiled_header(runtime_headers_h, *runtime_header_pch_files[i], pch_dirs[i]) && is_ok;
  }
  return is_ok;
}

static std::unordered_map<File *, long long> create_dep_mtime(const Index &cpp_dir, const std::forward_list<Index> &imported_headers) {
  std::unordered_map<File *, long long> dep_mtime;
  std::priority_queue<std::pair<long long, File *>> mtime_queue;
  std::unordered_map<File *, std::vector<File *>> reverse_includes;

  const auto &files = cpp_dir.get_files();

  auto lib_version_it = std::find_if(files.begin(), files.end(), [](File *file) { return file->name == "_lib_version.h"; });
  kphp_assert(lib_version_it != files.end());
  File *lib_version = *lib_version_it;

  for (const auto &file : files) {
    for (const auto &include : file->includes) {
      File *header = cpp_dir.get_file(include);
      kphp_assert_msg(header != nullptr, fmt_format("Can't find header {} required by {}", include, file->name));
      kphp_assert(header->on_disk);
      reverse_includes[header].push_back(file);
    }

    long long max_mtime = std::max(file->mtime, lib_version->mtime);
    for (const auto &lib_include : file->lib_includes) {
      max_mtime = std::max(max_mtime, get_imported_header_mtime(lib_include, imported_headers));
    }

    dep_mtime[file] = max_mtime;
    mtime_queue.emplace(max_mtime, file);
  }

  while (!mtime_queue.empty()) {
    const auto [mtime, file] = mtime_queue.top();
    mtime_queue.pop();

    if (dep_mtime[file] != mtime) {
      continue;
    }

    auto file_includes_it = reverse_includes.find(file);
    if (file_includes_it == reverse_includes.end()) {
      continue;
    }

    for (auto *including_file: file_includes_it->second) {
      auto &including_file_mtime = dep_mtime[including_file];
      if (including_file_mtime < mtime) {
        including_file_mtime = mtime;
        mtime_queue.emplace(including_file_mtime, including_file);
      }
    }
  }
  return dep_mtime;
}

static std::vector<File *> create_obj_files(MakeSetup *make, Index &obj_dir, const Index &cpp_dir,
                                            const std::forward_list<Index> &imported_headers) {
  std::unordered_map<File *, long long> dep_mtime = create_dep_mtime(cpp_dir, imported_headers);
  std::vector<File *> objs;
  for (const auto &cpp_file : cpp_dir.get_files()) {
    if (cpp_file->ext == ".cpp") {
      File *obj_file = obj_dir.insert_file(static_cast<std::string>(cpp_file->name_without_ext) + ".o");
      obj_file->compile_with_debug_info_flag = cpp_file->compile_with_debug_info_flag;
      make->create_cpp2obj_target(cpp_file, obj_file);
      Target *cpp_target = cpp_file->target;
      cpp_target->force_changed(dep_mtime[cpp_file]);
      objs.push_back(obj_file);
    }
  }
  fmt_fprintf(stderr, "objs cnt = {}\n", objs.size());

  std::map<vk::string_view, std::vector<File *>> subdirs;
  std::vector<File *> tmp_objs;
  for (auto *obj_file : objs) {
    auto name = obj_file->subdir;
    if (!name.empty()) {
      name.remove_suffix(1);
    }
    if (name.empty()) {
      tmp_objs.push_back(obj_file);
      continue;
    }
    subdirs[name].push_back(obj_file);
  }

  objs = std::move(tmp_objs);
  for (auto &name_and_files : subdirs) {
    auto &deps = name_and_files.second;
    std::sort(deps.begin(), deps.end(), [](File *a, File *b) { return a->name < b->name; });
    size_t hash = 0;
    for (File *f : deps) {
      vk::hash_combine(hash, vk::std_hash(f->name));
    }
    auto intermediate_file_name = fmt_format("{}_{:x}.{}", name_and_files.first, hash,
                                             G->settings().dynamic_incremental_linkage.get() ? "so" : "o");
    File *obj_file = obj_dir.insert_file(std::move(intermediate_file_name));
    make->create_objs2obj_target(std::move(deps), obj_file);
    objs.push_back(obj_file);
  }
  fmt_fprintf(stderr, "objs cnt = {}\n", objs.size());
  return objs;
}

static std::vector<File *> kphp_make_target(Index &obj_dir, const Index &cpp_dir,
                      const std::forward_list<Index> &imported_headers, MakeSetup &make) {
  std::vector<File *> lib_objs;
  auto imported_libs = collect_imported_libs();
  for (File *link_file: imported_libs) {
    make.create_cpp_target(link_file);
    lib_objs.emplace_back(link_file);
  }
  std::vector<File *> objs = create_obj_files(&make, obj_dir, cpp_dir, imported_headers);
  std::copy(lib_objs.begin(), lib_objs.end(), std::back_inserter(objs));
  return objs;
}

static std::vector<File *> kphp_make_static_lib_target(Index &obj_dir, const Index &cpp_dir,
                                 const std::forward_list<Index> &imported_headers, MakeSetup &make) {
  return create_obj_files(&make, obj_dir, cpp_dir, imported_headers);
}

static std::forward_list<Index> collect_imported_headers() {
  std::forward_list<Index> imported_headers;
  for (const auto &lib: G->get_libs()) {
    if (lib && !lib->is_raw_php()) {
      imported_headers.emplace_front();
      imported_headers.front().sync_with_dir(lib->lib_dir());
    }
  }
  return imported_headers;
}

static std::vector<File *> run_pre_make(OutputMode output_mode, const CompilerSettings &settings, FILE *make_stats_file, MakeSetup &make, Index &obj_index, File &bin_file) {
  AutoProfiler profiler{get_profiler("Prepare Targets For Build")};

  G->del_extra_files();
  obj_index.sync_with_dir(settings.dest_objs_dir.get());

  if (settings.force_make.get()) {
    obj_index.del_extra_files();
    bin_file.unlink();
  }

  const bool pch_allowed = !settings.no_pch.get();
  if (pch_allowed) {
    kphp_error(kphp_make_precompiled_headers(&obj_index, settings, make_stats_file), "Make precompiled header failed");
  }

  auto lib_header_dirs = collect_imported_headers();
  return output_mode == OutputMode::lib ? kphp_make_static_lib_target(obj_index, G->get_index(), lib_header_dirs, make)
                                        : kphp_make_target(obj_index, G->get_index(), lib_header_dirs, make);
}

void run_make() {
  const auto &settings = G->settings();
  OutputMode output_mode = G->get_output_mode();
  FILE *make_stats_file = nullptr;
  if (!settings.stats_file.get().empty()) {
    make_stats_file = fopen(settings.stats_file.get().c_str(), "w");
    kphp_error(make_stats_file, fmt_format("Can't open stats-file {}", settings.stats_file.get()));
    stage::die_if_global_errors();
  }
  stage::set_name("Make");

  Index obj_index;

  File bin_file(settings.binary_path.get());
  kphp_assert(bin_file.read_stat() >= 0);

  MakeSetup make{make_stats_file, settings};
  auto objs = run_pre_make(output_mode, settings, make_stats_file, make, obj_index, bin_file);
  stage::die_if_global_errors();

  if (output_mode == OutputMode::lib) {
    make.create_objs2static_lib_target(objs, &bin_file);
  } else if (output_mode == OutputMode::k2_component) {
    make.create_objs2k2_component_target(objs, &bin_file);
  } else {
    const std::string build_stage{"Compiling"};
    AutoProfiler profiler{get_profiler(build_stage)};

    bool ok = make.make_targets(objs, build_stage, settings.jobs_count.get());
    kphp_error(ok, build_stage + " stage failure");
    make.create_objs2bin_target(objs, &bin_file);
  }
  stage::die_if_global_errors();

  const std::string build_stage{output_mode == OutputMode::lib ? "Compiling" : "Linking"};
  AutoProfiler profiler{get_profiler(build_stage)};

  bool ok = make.make_target(&bin_file, build_stage, settings.jobs_count.get());
  kphp_error(ok, build_stage + " stage failure");

  if (make_stats_file) {
    fclose(make_stats_file);
  }
  stage::die_if_global_errors();
  obj_index.del_extra_files();

  if (bin_file.read_stat() > 0) {
    G->stats.object_out_size = bin_file.file_size;
  }

  if (!settings.user_binary_path.get().empty()) {
    hard_link_or_copy(bin_file.path, settings.user_binary_path.get());
  }
  if (output_mode == OutputMode::lib) {
    copy_static_lib_to_out_dir(std::move(bin_file));
  }
}
