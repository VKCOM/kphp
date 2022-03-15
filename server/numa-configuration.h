// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#if defined(__APPLE__)
struct bitmask {};
#else
#include <numa.h>
#endif

#include <map>
#include <sched.h>
#include <vector>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"

class NumaConfiguration : vk::not_copyable {
public:
  enum class MemoryPolicy { local, bind };

  bool add_numa_node(int numa_node_id, const bitmask *cpu_mask);
  bool enabled() const;
  void distribute_worker(int worker_index) const;
  void set_memory_policy(MemoryPolicy policy);
private:
  std::vector<int> numa_nodes;
  std::vector<cpu_set_t> numa_node_masks;
  MemoryPolicy memory_policy;
  int total_cpus{0};
  int total_numa_nodes{0};
  bool inited{false};

  friend class vk::singleton<NumaConfiguration>;
};
