#pragma once

#include "compiler/types.h"

namespace tinf {

class Node;

class Edge {
public :
  Node *from;
  Node *to;

  const MultiKey *from_at;
};


}
