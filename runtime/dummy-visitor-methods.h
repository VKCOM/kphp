// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/stdlib/visitors/common-visitors-methods.h"
#include "runtime/instance-copy-processor.h"
#include "runtime/memory_usage.h"

struct DummyVisitorMethods : CommonDefaultVisitorMethods {
  using CommonDefaultVisitorMethods::accept;
  // for f$estimate_memory_usage()
  // set at compiler at deeply_require_instance_memory_estimate_visitor()
  //  void accept(InstanceMemoryEstimateVisitor &) noexcept {}
  void accept(CommonMemoryEstimateVisitor &) noexcept {}
  // for f$instance_cache_*() and f$kphp_job_worker_*()
  // set at compiler at deeply_require_instance_cache_visitor()
  void accept(InstanceReferencesCountingVisitor &) noexcept {}
  void accept(InstanceDeepCopyVisitor &) noexcept {}
  void accept(InstanceDeepDestroyVisitor &) noexcept {}
};
