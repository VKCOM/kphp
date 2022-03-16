// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#if defined(__APPLE__)
struct bitmask {};
struct cpu_set_t {};
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
  int get_worker_numa_node(int worker_index) const;
  void distribute_worker(int worker_index) const;
  void distribute_master_if_needed() const;
  void set_numa_node_to_bind_master(int numa_node_id);
  void set_memory_policy(MemoryPolicy policy);
private:
  std::vector<int> numa_nodes;
  std::vector<cpu_set_t> numa_node_masks;
  MemoryPolicy memory_policy;
  int total_cpus{0};
  int total_numa_nodes{0};
  int numa_node_to_bind_master{-1};
  bool inited{false};

  void distribute_process(int numa_node_id, const cpu_set_t &cpu_mask) const;

  friend class vk::singleton<NumaConfiguration>;
};
