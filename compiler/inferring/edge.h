// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/inferring/multi-key.h"

namespace tinf {

class Node;

class Edge {
public:
  Node* from;
  Node* to;

  const MultiKey* from_at;
};

} // namespace tinf
