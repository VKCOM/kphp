// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/stdlib/visitors/common-visitors-methods.h"

class CommonMemoryEstimateVisitor;
class InstanceDeepCopyVisitor;
class InstanceDeepDestroyVisitor;
class InstanceReferencesCountingVisitor;

struct DummyVisitorMethods : CommonDefaultVisitorMethods {
  using CommonDefaultVisitorMethods::accept;
  // for f$estimate_memory_usage()
  // set at compiler at deeply_require_instance_memory_estimate_visitor()
  //  void accept(InstanceMemoryEstimateVisitor &) noexcept {}
  void accept(CommonMemoryEstimateVisitor & /*unused*/) noexcept {}
  // for f$instance_cache_*() and f$kphp_job_worker_*()
  // set at compiler at deeply_require_instance_cache_visitor()
  void accept(InstanceReferencesCountingVisitor & /*unused*/) noexcept {}
  void accept(InstanceDeepCopyVisitor & /*unused*/) noexcept {}
  void accept(InstanceDeepDestroyVisitor & /*unused*/) noexcept {}
};
