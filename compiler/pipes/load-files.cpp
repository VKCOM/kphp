#include "compiler/pipes/load-files.h"

#include "compiler/data/src-file.h"
#include "compiler/stage.h"
#include "compiler/threading/profiler.h"

void LoadFileF::execute(SrcFilePtr file, DataStream<SrcFilePtr> &os) {
  stage::set_name("Load file");
  stage::set_file(file);

  kphp_assert (!file->loaded);
  file->load();

  if (stage::has_error()) {
    return;
  }

  os << file;
}
