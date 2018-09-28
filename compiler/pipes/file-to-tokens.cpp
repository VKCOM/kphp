#include "compiler/pipes/file-to-tokens.h"

#include "compiler/io.h"
#include "compiler/lexer.h"

void FileToTokensF::execute(SrcFilePtr file, DataStream<FileAndTokens> &os) {
  AUTO_PROF(lexer);
  stage::set_name("Split file to tokens");
  stage::set_file(file);
  kphp_assert(file);

  kphp_assert(file->loaded);
  FileAndTokens res;
  res.file = file;
  res.tokens = new vector<Token *>();
  php_text_to_tokens(
    &file->text[0], (int)file->text.length(),
    file->main_func_name, res.tokens
  );

  if (stage::has_error()) {
    return;
  }

  os << res;
}
