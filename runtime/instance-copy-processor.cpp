// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/instance-copy-processor.h"

InstanceDeepCopyVisitor::InstanceDeepCopyVisitor(memory_resource::unsynchronized_pool_resource &memory_pool, ExtraRefCnt memory_ref_cnt) noexcept:
  Basic(*this, memory_ref_cnt),
  memory_pool_(memory_pool) {
}

InstanceDeepDestroyVisitor::InstanceDeepDestroyVisitor(ExtraRefCnt memory_ref_cnt) noexcept:
  Basic(*this, memory_ref_cnt) {
}

bool InstanceDeepCopyVisitor::process(string &str) noexcept {
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
    str.set_reference_counter_to(get_memory_ref_cnt());
  }
  return true;
}

bool InstanceDeepDestroyVisitor::process(string &str) noexcept {
  // if string is constant, skip it, otherwise element was cached and should be destroyed
  if (!str.is_reference_counter(ExtraRefCnt::for_global_const)) {
    str.force_destroy(get_memory_ref_cnt());
  }
  return true;
}
