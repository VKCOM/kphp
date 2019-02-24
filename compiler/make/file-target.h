#pragma once

#include "compiler/make/target.h"

class FileTarget : public KphpTarget {
public:
  string get_cmd() final {
    assert (0);
    return "";
  }
};
