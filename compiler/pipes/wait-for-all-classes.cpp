// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/wait-for-all-classes.h"
#include "compiler/pipes/sort-and-inherit-classes.h"

#include "compiler/compiler-core.h"
#include "compiler/data/src-dir.h"
#include "compiler/data/src-file.h"
#include "compiler/data/modulite-data.h"

/*
 * This is a sync point. When `on_finish()` is called (once, in a single thread), then
 * all php files have been parsed, all classes have been loaded.
 * It means, that in `on_finish()` we can look up classes/functions by name.
 *
 * What we do after loading everything:
 * 1) Convert extends and implements from stringnames to ClassPtr, validate existence, check inheritance.
 * 2) Parse all .modulite.yaml emitting ModulitePtr. We can do it only now, as modulites config contain class/method names.
 *    Also, we assign dir->nested_files_modulite to every dir in a tree.
 *
 * On complete, pass all functions forward the compilation pipeline.
 */

void WaitForAllClassesAndLoadModulitesF::on_finish(DataStream<FunctionPtr> &os) {
  stage::die_if_global_errors();
  SortAndInheritClassesF::check_on_finish(tmp_stream);
  stage::set_name("Register modulites");
  register_all_modulites();
  Base::on_finish(os);
}

void WaitForAllClassesAndLoadModulitesF::register_all_modulites() {
  if (!G->settings().modulite_enabled.get()) {
    return;
  }

  SrcDirPtr dir_root = G->get_main_file()->dir;

  // Sort all dirs in a project from short to long, e.g.
  // - /root/of/project
  // - /root/of/project/VK/Messages
  // - /root/of/project/VK/Expressions
  // - /root/of/project/VK/Messages/Core
  // We'll iterate dirs in the same order, so we have a guarantee, that
  // on a child dir pointer, its parent dir has 100% been visited.
  std::vector<SrcDirPtr> all_dirs = G->get_dirs();
  std::sort(all_dirs.begin(), all_dirs.end(), [](SrcDirPtr d1, SrcDirPtr d2) {
    return d1->full_dir_name.size() < d2->full_dir_name.size();
  });

  // 1) parse all existing .modulite.yaml files
  // while parsing, cross-references like @another-m are not resolved, it will be done below
  for (SrcDirPtr dir : all_dirs) {
    if (dir->has_modulite_yaml) {
      // find the dir up the tree with .modulite.yaml; say, dir contains @msg/channels, with_parent expected to be @msg
      SrcDirPtr with_parent = dir;
      while (with_parent && with_parent != dir_root && !with_parent->nested_files_modulite) {
        with_parent = with_parent->parent_dir;
      }
      ModulitePtr parent = with_parent ? with_parent->nested_files_modulite : ModulitePtr{};

      ModulitePtr modulite = ModuliteData::create_from_modulite_yaml(dir->get_modulite_yaml_filename(), parent);
      if (!modulite->modulite_name.empty()) {   // no error while parsing yaml
        G->register_modulite(modulite);
        dir->nested_files_modulite = modulite;
      }
    }
  }
  stage::die_if_global_errors();  // on any parsing errors, they were printed using kphp_error, same below

  // 2) resolve cross-references like @msg in "export"/"requires"/etc.
  for (SrcDirPtr dir : all_dirs) {
    if (dir->nested_files_modulite) {
      ModulitePtr modulite = dir->nested_files_modulite;
      modulite->resolve_names_to_pointers();
    }
  }
  stage::die_if_global_errors();

  // 3) propagate dir->modulite to child dirs unless a child dir is a modulite itself
  // - VK/Messages/                               it's @messages now
  //   - .modulite.yaml (@messages)
  //   - Core/                                    <-- assign @messages here
  //     - Channels/                              it's @messages/channels now
  //       - .modulite.yaml (@messages/channels)
  for (SrcDirPtr dir : all_dirs) {
    if (!dir->nested_files_modulite && dir->parent_dir) {
      dir->nested_files_modulite = dir->parent_dir->nested_files_modulite;
    }
  }

  // 4) validate symbols in yaml config like "export", check that child modulites are placed in expected dirs, etc.
  // we do this after knowing all dirs and subdirs (=> all files inside them), which modulites every file belongs to
  for (SrcDirPtr dir : all_dirs) {
    if (dir->nested_files_modulite && dir->nested_files_modulite->yaml_file->dir == dir) {
      ModulitePtr modulite = dir->nested_files_modulite;
      modulite->validate_yaml_requires();
      modulite->validate_yaml_exports();
      modulite->validate_yaml_force_internal();
    }
  }
  stage::die_if_global_errors();
}
