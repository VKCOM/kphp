#pragma once

#include "compiler/io.h"
#include "compiler/threading/data-stream.h"

class WriteFilesF {
public:
  void execute(WriterData data, EmptyStream &);
};
