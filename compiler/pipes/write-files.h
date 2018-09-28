#pragma once

#include "compiler/bicycle.h"
#include "compiler/io.h"

class WriteFilesF {
public:
  void on_finish(EmptyStream &) {};

  void execute(WriterData *data, EmptyStream &);
};
