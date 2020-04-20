#pragma once

#include "compiler/inferring/node.h"
#include "compiler/inferring/restriction-base.h"

class RestrictionNonVoid : public tinf::RestrictionBase {
  tinf::Node *node;
  std::string desc;
public:

  explicit RestrictionNonVoid(tinf::Node *node);
  const char *get_description() override;

protected:
  bool check_broken_restriction_impl() override;
};
