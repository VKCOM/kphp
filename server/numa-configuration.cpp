// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/numa-configuration.h"

#include <algorithm>
#include <cassert>

#include "common/dl-utils-lite.h"
#include "common/kprintf.h"

bool NumaConfiguration::add_numa_node([[maybe_unused]] int numa_node_id, [[maybe_unused]] const bitmask *cpu_mask) {
#if defined(__APPLE__)
  return false;
#else
  assert(numa_available() >= 0);

  if (!inited) {
    total_cpus = numa_num_configured_cpus();
    total_numa_nodes = numa_num_configured_nodes();
    numa_node_masks.resize(total_numa_nodes);
    for (auto &mask : numa_node_masks) {
      CPU_ZERO(&mask);
    }
    inited = true;
  }

  for (int cpu = 0; cpu < total_cpus; ++cpu) {
    if (numa_bitmask_isbitset(cpu_mask, cpu)) {
      int actual_numa_node = numa_node_of_cpu(cpu);
      if (actual_numa_node != numa_node_id) {
        kprintf("CPU #%d belongs to %d NUMA node, but %d is given\n", cpu, actual_numa_node, numa_node_id);
        return false;
      }
      CPU_SET(cpu, &numa_node_masks[numa_node_id]); // prepare cpu bitmask
    }
  }

  numa_nodes.emplace_back(numa_node_id);
  // remove duplicates:
  std::sort(numa_nodes.begin(), numa_nodes.end());
  numa_nodes.erase(std::unique(numa_nodes.begin(), numa_nodes.end()), numa_nodes.end());
  return true;
#endif
}

void NumaConfiguration::distribute_process([[maybe_unused]] int numa_node_id, [[maybe_unused]] const cpu_set_t &cpu_mask) const {
#if !defined(__APPLE__)
  assert(numa_available() >= 0);

  int res = sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);
  dl_passert(res != -1, "Can't bind worker to cpu");

  switch (memory_policy) {
    case MemoryPolicy::bind: {
      auto *node_mask = numa_allocate_nodemask();
      numa_bitmask_setbit(node_mask, numa_node_id);
      numa_set_membind(node_mask);
      numa_free_nodemask(node_mask);
      break;
    }
    case MemoryPolicy::local:
      numa_set_localalloc(); // this is set by default, but let's set it here just in case
      break;
  }
#endif
}

void NumaConfiguration::distribute_worker(int worker_index) const {
  int numa_node_to_bind = get_worker_numa_node(worker_index);
  const auto &cpu_mask_to_bind = numa_node_masks.at(numa_node_to_bind);

  distribute_process(numa_node_to_bind, cpu_mask_to_bind);
}

void NumaConfiguration::set_memory_policy(NumaConfiguration::MemoryPolicy policy) {
  memory_policy = policy;
}

bool NumaConfiguration::enabled() const {
  return inited;
}

int NumaConfiguration::get_worker_numa_node(int worker_index) const {
  return numa_nodes[worker_index % numa_nodes.size()];
}
