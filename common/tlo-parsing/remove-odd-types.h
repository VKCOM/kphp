#include "common/tlo-parsing/tl-objects.h"

namespace vk {
namespace tl {
  void remove_cyclic_types(tl_scheme &scheme);
  void remove_exclamation_types(tl_scheme &scheme);
}
}
