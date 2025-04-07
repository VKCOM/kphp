// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/instance-copy-processor.h"
#include "common/dl-utils-lite.h"

InstanceDeepCopyVisitor::InstanceDeepCopyVisitor(memory_resource::unsynchronized_pool_resource &memory_pool,
                                                 ExtraRefCnt memory_ref_cnt, ResourceCallbackOOM oom_callback) noexcept:
  Basic(*this, memory_ref_cnt),
  memory_pool_(memory_pool),
  oom_callback_(oom_callback) {
}

InstanceDeepDestroyVisitor::InstanceDeepDestroyVisitor(ExtraRefCnt memory_ref_cnt) noexcept:
  Basic(*this, memory_ref_cnt) {
}

bool InstanceDeepCopyVisitor::process(string &str) noexcept {
  bool _found = false;
  if (strcmp(str.c_str(),"http://10.248.0.113:8099") == 0) {
    _found = true;
    fprintf(stderr, "########################## %p %ld %s\n", str.p, str.get_reference_counter(), str.c_str());
    std::array<void*, 16> buf;
    fast_backtrace(buf.data(), buf.size());
    dl_print_backtrace(buf.data(), buf.size());
  }

  if (str.is_reference_counter(ExtraRefCnt::for_global_const)) {
    return true;
  }

  // TODO: it seems we can use `str.estimate_memory_usage(str.size())` here,
  //  because we don't need to take capacity into the account, only size.
  if (unlikely(!is_enough_memory_for(str.estimate_memory_usage()))) {
    if ( _found ) {
      fprintf(stderr, "#######  not enough mem\n");
    }
    str = string();
    memory_limit_exceeded_ = true;
    return false;
  }

  str.make_not_shared();
  if ( _found ) {
    fprintf(stderr, "####### %p %ld %s\n", str.p, str.get_reference_counter(), str.c_str());
  }
  // make_not_shared may make str constant again (e.g. const empty or single char str), therefore check again
  if (str.is_reference_counter(ExtraRefCnt::for_global_const)) {
    php_assert(str.size() < 2);
    if ( _found ) {
      fprintf(stderr, "#######  ExtraRefCnt::for_global_const)\n");
    }
  } else {
    php_assert(str.get_reference_counter() == 1);
    if ( _found ) {
      fprintf(stderr, "####### not ExtraRefCnt::for_global_const\n");
    }
    if (const auto extra_ref_cnt = get_memory_ref_cnt()) {
      if ( _found ) {
        fprintf(stderr, "#######  extra_ref_cnt: %d\n", static_cast<int>(extra_ref_cnt));
      }
      str.set_reference_counter_to(extra_ref_cnt);
    }
  }
  return true;
}

bool InstanceDeepDestroyVisitor::process(string &str) noexcept {
  // if string is constant, skip it, otherwise element was cached and should be destroyed
  if (!str.is_reference_counter(ExtraRefCnt::for_global_const)) {
    str.force_destroy(get_memory_ref_cnt());
  }
  str = string{};
  return true;
}

template<class T>
bool InstanceDeepCopyVisitor::PrimitiveArrayProcessor<T>::process(InstanceDeepCopyVisitor &self, array<T> &arr) noexcept {
  return self.process_array_impl(arr);
}

template struct InstanceDeepCopyVisitor::PrimitiveArrayProcessor<int64_t>;
template struct InstanceDeepCopyVisitor::PrimitiveArrayProcessor<double>;
template struct InstanceDeepCopyVisitor::PrimitiveArrayProcessor<bool>;
template struct InstanceDeepCopyVisitor::PrimitiveArrayProcessor<Optional<int64_t>>;
template struct InstanceDeepCopyVisitor::PrimitiveArrayProcessor<Optional<double>>;
template struct InstanceDeepCopyVisitor::PrimitiveArrayProcessor<Optional<bool>>;
