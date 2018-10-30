#pragma once

#include <vector>

#include "compiler/data/data_ptr.h"

struct FileAndTokens {
  SrcFilePtr file;
  vector<Token *> *tokens;

  FileAndTokens() :
    file(),
    tokens(nullptr) {
  }
};
