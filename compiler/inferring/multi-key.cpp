#include "compiler/inferring/multi-key.h"

#include <functional>

#include "common/algorithms/string-algorithms.h"

std::string MultiKey::to_string() const {
  return "[" + vk::join(*this, ",", std::mem_fn(&Key::to_string))+ "]";
}

std::vector<MultiKey> MultiKey::any_key_vec;
