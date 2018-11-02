#pragma once

#include "compiler/stage.h"

namespace tinf {

// TODO: merge it to one class
struct RestrictionBase {
  virtual ~RestrictionBase() = default;
  virtual const char *get_description() = 0;
  virtual bool check_broken_restriction() = 0;

protected:
  virtual bool is_broken_restriction_an_error() { return false; }

  virtual bool check_broken_restriction_impl() = 0;
};

class Restriction : public RestrictionBase {
public:
  Location location;

  Restriction() :
    location(stage::get_location()) {}

  bool check_broken_restriction() override;
};

}
