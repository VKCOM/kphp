// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/cpp-dest-dir-initializer.h"

#include "compiler/compiler-core.h"

void CppDestDirInitializer::initialize_sync() noexcept {
  const char *description = "Init cpp dest dir";
  stage::set_name(description);
  stage::set_file(SrcFilePtr{});
  static CachedProfiler cache{description};
  AutoProfiler prof{*cache};
  G->init_dest_dir();
  G->load_index();

  if (!G->settings().force_link_runtime.get()) {
    G->init_runtime_and_common_srcs_dir();
  }
}

void CppDestDirInitializer::initialize_async(int32_t thread_id) noexcept {
  kphp_assert(!initialization_thread_);

  initialization_thread_ = std::make_unique<std::thread>([thread_id] {
    set_thread_id(thread_id);
    initialize_sync();
  });
}

void CppDestDirInitializer::wait() noexcept {
  if (initialization_thread_) {
    kphp_assert(initialization_thread_->joinable());
    initialization_thread_->join();
    initialization_thread_.reset();
  }
}

CppDestDirInitializer::~CppDestDirInitializer() noexcept {
  if (initialization_thread_) {
    initialization_thread_->detach();
  }
}
