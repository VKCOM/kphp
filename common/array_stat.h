// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt
#pragma once

#include "common/smart_ptrs/singleton.h"
#include "common/mixin/not_copyable.h"

struct StatNode {
  double time{0};
  std::size_t count{0};
};

class MapStat : vk::not_copyable {
  friend class vk::singleton<MapStat>;
  MapStat() = default;

public:
  void reset() noexcept {
    mem_allocated = 0;
    mem_deallocated = 0;
    create = StatNode{};
    dispose = StatNode{};
    emplace = StatNode{};
    find = StatNode{};
  }

  std::size_t mem_allocated{0};
  std::size_t mem_deallocated{0};

  StatNode create{0};
  StatNode dispose{0};
  StatNode emplace{0};
  StatNode find{0};
  StatNode unset{0};
};
