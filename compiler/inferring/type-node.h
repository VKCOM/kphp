#pragma once

#include "compiler/inferring/node.h"
#include "compiler/location.h"

namespace tinf {

class TypeNode : public Node {
  Location location_;

public:
  explicit TypeNode(const TypeData *type, Location location) :
    location_(std::move(location)) {
    set_type(type);
  }

  void recalc(TypeInferer *inferer __attribute__((unused))) {
  }

  string get_location_text();
  string get_description();
};

}
