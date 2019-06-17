#pragma once
#include "compiler/inferring/restriction-less.h"

class RestrictionGreater final : public RestrictionLess {
public:
  using RestrictionLess::RestrictionLess;

protected:
  bool is_greater_restriction() final {
    return true;
  }
};
