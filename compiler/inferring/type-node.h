// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/inferring/node.h"
#include "compiler/location.h"

namespace tinf {

class TypeNode : public Node {
  Location location_;

public:
  explicit TypeNode(const TypeData *type, Location location)
    : location_(std::move(location)) {
    set_type(type);
  }

  void recalc(TypeInferer *inferer __attribute__((unused))) {}

  std::string get_description() final;
  const Location &get_location() const final;
};

} // namespace tinf
