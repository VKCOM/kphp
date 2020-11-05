// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/tlo-parsing/tl-objects.h"

namespace vk {
namespace tl {
  void remove_cyclic_types(tl_scheme &scheme);
  void remove_exclamation_types(tl_scheme &scheme);
}
}
