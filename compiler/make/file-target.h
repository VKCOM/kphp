#pragma once

#include "compiler/make/target.h"

class FileTarget : public Target {
public:
  string get_cmd() final {
    assert (0);
    return "";
  }
};
