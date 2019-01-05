#include "compiler/pipes/file-to-tokens.h"

#include "compiler/data/src-file.h"
#include "compiler/lexer.h"
#include "compiler/stage.h"
#include "compiler/threading/profiler.h"

void FileToTokensF::execute(SrcFilePtr file, DataStream<FileAndTokens> &os) {
  stage::set_name("Split file to tokens");
  stage::set_file(file);
  kphp_assert(file);

  kphp_assert(file->loaded);
  FileAndTokens res;
  res.file = file;
  res.tokens = new vector<Token *>();
  php_text_to_tokens(&file->text[0], (int)file->text.length(), res.tokens);

  if (stage::has_error()) {
    return;
  }

  os << res;
}
