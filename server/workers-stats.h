// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

struct workers_stats_t {
  long tot_workers_started{0};
  long tot_workers_dead{0};
  long tot_workers_strange_dead{0};
  long workers_killed{0};
  long workers_hung{0};
  long workers_terminated{0};
  long workers_failed{0};
};
