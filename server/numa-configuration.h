// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <numa.h>
#include <vector>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"


class NumaConfiguration : vk::not_copyable {
public:
  enum class MemoryPolicy {
    local,
    bind
  };

  bool add_numa_node(int numa_node_id, const bitmask *cpu_mask);
  bool enabled() const;
  void distribute_worker(int worker_index) const;
  void set_memory_policy(MemoryPolicy policy);
private:
  std::vector<int> allowed_cpus;
  MemoryPolicy memory_policy;

  friend class vk::singleton<NumaConfiguration>;
};
