// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/inferring/node.h"
#include "compiler/inferring/restriction-base.h"

class RestrictionNonVoid : public tinf::RestrictionBase {
  tinf::Node* node;

public:
  explicit RestrictionNonVoid(tinf::Node* node);

  bool is_restriction_broken() final;
  std::string get_description() final;
};
