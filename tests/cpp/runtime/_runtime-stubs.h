#pragma once
#include <cassert>

#include "runtime/storage.h"
#include "runtime/interface.h"
#include "runtime/job-workers/job-interface.h"
#include "runtime/pdo/pdo_statement.h"
#include "runtime/tl/rpc_response.h"
#include "server/php-engine-vars.h"
#include "server/workers-control.h"

template<> int Storage::tagger<bool>::get_tag() noexcept { return 0; }
template<> int Storage::tagger<int64_t>::get_tag() noexcept { return 0; }
template<> int Storage::tagger<Optional<int64_t>>::get_tag() noexcept { return 0; }
template<> int Storage::tagger<void>::get_tag() noexcept { return 0; }
template<> int Storage::tagger<thrown_exception>::get_tag() noexcept { return 0; }
template<> int Storage::tagger<mixed>::get_tag() noexcept { return 0; }
template<> int Storage::tagger<array<mixed>>::get_tag() noexcept { return 0; }
template<> int Storage::tagger<Optional<string>>::get_tag() noexcept { return 0; }
template<> int Storage::tagger<Optional<array<mixed>>>::get_tag() noexcept { return 0; }
template<> int Storage::tagger<array<array<mixed>>>::get_tag() noexcept { return 0; }
template<> int Storage::tagger<class_instance<C$KphpJobWorkerResponse>>::get_tag() noexcept { return 0; }
template<> int Storage::tagger<class_instance<C$VK$TL$RpcResponse>>::get_tag() noexcept { return 0; }
template<> int Storage::tagger<array<class_instance<C$VK$TL$RpcResponse>>>::get_tag() noexcept { return 0; }
template<> int Storage::tagger<class_instance<C$PDOStatement>>::get_tag() noexcept { return 0; }
template<> Storage::loader<mixed>::loader_fun Storage::loader<mixed>::get_function(int) noexcept { return nullptr; }

void init_php_scripts() noexcept {
  assert(0 && "this code shouldn't be executed and only for linkage test");
}
void global_init_php_scripts() noexcept {
  assert(0 && "this code shouldn't be executed and only for linkage test");
}
const char *get_php_scripts_version() noexcept {
  assert(0 && "this code shouldn't be executed and only for linkage test");
}

char **get_runtime_options(int *) noexcept {
  assert(0 && "this code shouldn't be executed and only for linkage test");
  return nullptr;
}
