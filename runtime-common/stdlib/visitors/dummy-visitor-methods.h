// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

class CommonMemoryEstimateVisitor;
class ToArrayVisitor;
class InstanceDeepCopyVisitor;
class InstanceDeepDestroyVisitor;
class InstanceReferencesCountingVisitor;

namespace kphp::visitors {
class instance_deep_copy_visitor;
class instance_deep_estimate_size_visitor;
} // namespace kphp::visitors

struct DummyVisitorMethods {
  // for f$estimate_memory_usage()
  // set at compiler at deeply_require_instance_memory_estimate_visitor()
  void accept(CommonMemoryEstimateVisitor& /*unused*/) noexcept {}
  // for f$instance_to_array(), f$to_array_debug()
  // set at compiler at deeply_require_to_array_debug_visitor()
  void accept(ToArrayVisitor& /*unused*/) noexcept {}
  // for f$instance_cache_*() and f$kphp_job_worker_*()
  // set at compiler at deeply_require_instance_cache_visitor()
  void accept(InstanceReferencesCountingVisitor& /*unused*/) noexcept {}
  void accept(InstanceDeepCopyVisitor& /*unused*/) noexcept {}
  void accept(InstanceDeepDestroyVisitor& /*unused*/) noexcept {}
  // K2 counterparts of the instance cache visitors
  void accept(kphp::visitors::instance_deep_copy_visitor& /*unused*/) noexcept {}
  void accept(kphp::visitors::instance_deep_estimate_size_visitor& /*unused*/) noexcept {}
};
