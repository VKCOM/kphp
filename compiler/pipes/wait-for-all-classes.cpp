// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/wait-for-all-classes.h"
#include "compiler/pipes/sort-and-inherit-classes.h"

#include "compiler/compiler-core.h"
#include "compiler/data/composer-json-data.h"
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

void WaitForAllClassesF::on_finish(DataStream<FunctionPtr> &os) {
  stage::die_if_global_errors();
  SortAndInheritClassesF::check_on_finish(tmp_stream);
  stage::set_name("Register modulites");
  resolve_and_validate_modulites();
  Base::on_finish(os);
}

// after all classes have been loaded, we can resolve symbols in modulite configs
// (until this point, yaml files were parsed, but stored as strings)
void WaitForAllClassesF::resolve_and_validate_modulites() {
  // sort all modulites from short to long, so that a parent appears before a child when iterating
  // this fact is used to fill `child->exported_from_parent` by lookup, and some others
  std::vector<ModulitePtr> all_modulites = G->get_modulites();
  std::sort(all_modulites.begin(), all_modulites.end(), [](ModulitePtr m1, ModulitePtr m2) {
    return m1->modulite_name.size() < m2->modulite_name.size();
  });

  // resolve symbols like SomeClass and @msg in "export"/"require"/etc.
  // until now, they are stored just as string names, they can be resolved only after all classes have been loaded
  for (ModulitePtr modulite : all_modulites) {
    modulite->resolve_names_to_pointers();
  }
  stage::die_if_global_errors();

  // validate symbols in yaml config, after resolving all
  // see comments inside validators for what they actually do
  for (ModulitePtr modulite : all_modulites) {
    modulite->validate_yaml_requires();
    modulite->validate_yaml_exports();
    modulite->validate_yaml_force_internal();
  }
}
