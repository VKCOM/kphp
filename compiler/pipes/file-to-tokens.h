#pragma once

#include "compiler/bicycle.h"
#include "compiler/data_ptr.h"
#include "compiler/pipes/file-and-token.h"

class FileToTokensF {
public:
  void execute(SrcFilePtr file, DataStream<FileAndTokens> &os);
};
