// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/inferring/multi-key.h"

#include <functional>

#include "common/algorithms/string-algorithms.h"

std::string MultiKey::to_string() const {
  if (keys_.empty()) {
    return "(empty)";
  }
  return "[" + vk::join(*this, ",", std::mem_fn(&Key::to_string))+ "]";
}

std::vector<MultiKey> MultiKey::any_key_vec;
