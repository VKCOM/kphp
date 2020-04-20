#pragma once
#include "compiler/inferring/restriction-less.h"

class RestrictionGreater final : public RestrictionLess {
public:
  using RestrictionLess::RestrictionLess;

protected:
  bool is_less_virt(const TypeData *given, const TypeData *expected, const MultiKey *from_at) final {
    return RestrictionLess::is_less_virt(expected, given, from_at);
  }
};
