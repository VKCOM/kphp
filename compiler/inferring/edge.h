#pragma once

#include "compiler/inferring/type-data.h"

namespace tinf {

class Node;

class Edge {
public :
  Node *from;
  Node *to;

  const MultiKey *from_at;
};


} // namespace tinf
