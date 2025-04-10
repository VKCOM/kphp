// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/inferring/type-node.h"

#include "compiler/inferring/type-data.h"
#include "compiler/stage.h"

std::string tinf::TypeNode::get_description() {
  return "TypeNode at " + location_.as_human_readable() + " : " + type_->as_human_readable();
}

const Location& tinf::TypeNode::get_location() const {
  return location_;
}
