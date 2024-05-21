#include "runtime-light/stdlib/interface.h"

#include <cassert>
#include <cstdint>
#include <random>


#include "runtime-core/runtime-core.h"
#include "runtime-light/component/component.h"
#include "runtime-light/coroutine/awaitable.h"

int64_t f$rand() {
  std::random_device rd;
  int64_t dice_roll = rd();
  return dice_roll;
}
