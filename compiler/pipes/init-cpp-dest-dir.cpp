// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <atomic>

#include "compiler/pipes/init-cpp-dest-dir.h"

#include "compiler/compiler-core.h"

void InitCppDestDirF::execute(FunctionPtr function, DataStream<FunctionPtr> &os) {
  os << function;

  static std::atomic_flag dest_dir_initiated = ATOMIC_FLAG_INIT;
  // We need init destination cpp directory only once
  // TODO: think about fully async initialization, start before LoadFileF and sync in CodeGenF::on_finish
  if (!dest_dir_initiated.test_and_set()) {
    const char *description = "Init cpp dest dir";
    stage::set_name(description);
    stage::set_file(SrcFilePtr{});
    static CachedProfiler cache{description};
    AutoProfiler prof{*cache};
    G->init_dest_dir();
    G->load_index();
  }
}
