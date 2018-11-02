#pragma once

#include "compiler/inferring/node.h"

namespace tinf {

class TypeNode : public Node {
public:
  explicit TypeNode(const TypeData *type) {
    set_type(type);
  }

  void recalc(TypeInferer *inferer __attribute__((unused))) {
  }

  string get_description();
};

}
