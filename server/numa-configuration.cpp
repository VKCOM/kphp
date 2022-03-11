// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <algorithm>
#include <cassert>
#include <sched.h>

#include "common/kprintf.h"
#include "server/numa-configuration.h"
#include "common/dl-utils-lite.h"


bool NumaConfiguration::add_numa_node(int numa_node_id, const bitmask *cpu_mask) {
  assert(numa_available() == 0);

  int total_cpus = numa_num_configured_cpus();
  allowed_cpus.reserve(total_cpus);
  for (int cpu = 0; cpu < total_cpus; ++cpu) {
    if (numa_bitmask_isbitset(cpu_mask, cpu)) {
      int actual_numa_node = numa_node_of_cpu(cpu);
      if (actual_numa_node != numa_node_id) {
        kprintf("CPU #%d belongs to %d NUMA node, but %d is given\n", cpu, actual_numa_node, numa_node_id);
        return false;
      }
      allowed_cpus.emplace_back(cpu);
    }
  }
  // remove duplicates:
  std::sort(allowed_cpus.begin(), allowed_cpus.end());
  allowed_cpus.erase(std::unique( allowed_cpus.begin(), allowed_cpus.end() ), allowed_cpus.end());
  return true;
}

void NumaConfiguration::distribute_worker(int worker_index) const {
  assert(numa_available() == 0);

  int cpu = allowed_cpus[worker_index % allowed_cpus.size()];

  cpu_set_t cpu_mask;
  CPU_ZERO(&cpu_mask);
  CPU_SET(cpu, &cpu_mask);
  int res = sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);
  dl_passert(res != -1, "Can't bind worker to cpu");

  switch (memory_policy) {
    case MemoryPolicy::bind: {
      int numa_node = numa_node_of_cpu(cpu);
      auto *node_mask = numa_allocate_nodemask();
      numa_bitmask_setbit(node_mask, numa_node);
      numa_set_membind(node_mask);
      numa_free_nodemask(node_mask);
      break;
    }
    case MemoryPolicy::local:
      numa_set_localalloc(); // this is set by default, but let's set it here just in case
      break;
  }
}

void NumaConfiguration::set_memory_policy(NumaConfiguration::MemoryPolicy policy) {
  memory_policy = policy;
}

bool NumaConfiguration::enabled() const {
  return !allowed_cpus.empty();
}
