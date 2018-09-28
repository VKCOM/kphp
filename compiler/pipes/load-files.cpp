#include "compiler/pipes/load-files.h"

#include "compiler/io.h"

void LoadFileF::execute(SrcFilePtr file, DataStream<SrcFilePtr> &os) {
  AUTO_PROF (load_files);
  stage::set_name("Load file");
  stage::set_file(file);

  kphp_assert (!file->loaded);
  file->load();

  if (stage::has_error()) {
    return;
  }

  os << file;
}
