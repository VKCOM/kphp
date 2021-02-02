// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/instance-copy-processor.h"

namespace impl_ {

DeepMoveFromScriptToCacheVisitor::DeepMoveFromScriptToCacheVisitor(memory_resource::unsynchronized_pool_resource &memory_pool) noexcept:
  Basic(*this),
  memory_pool_(memory_pool) {
}

DeepDestroyFromCacheVisitor::DeepDestroyFromCacheVisitor() :
  Basic(*this) {
}

bool DeepMoveFromScriptToCacheVisitor::process(string &str) {
  if (str.is_reference_counter(ExtraRefCnt::for_global_const)) {
    return true;
  }

  if (unlikely(!is_enough_memory_for(str.estimate_memory_usage()))) {
    str = string();
    memory_limit_exceeded_ = true;
    return false;
  }

  str.make_not_shared();
  // make_not_shared may make str constant again (e.g. const empty or single char str), therefore check again
  if (str.is_reference_counter(ExtraRefCnt::for_global_const)) {
    php_assert(str.size() < 2);
  } else {
    php_assert(str.get_reference_counter() == 1);
    str.set_reference_counter_to(ExtraRefCnt::for_instance_cache);
  }
  return true;
}

bool DeepDestroyFromCacheVisitor::process(string &str) {
  // if string is constant, skip it, otherwise element was cached and should be destroyed
  if (!str.is_reference_counter(ExtraRefCnt::for_global_const)) {
    str.force_destroy(ExtraRefCnt::for_instance_cache);
  }
  return true;
}

}
