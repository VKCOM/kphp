#include "compiler/pipes/file-to-tokens.h"

#include "compiler/data/src-file.h"
#include "compiler/lexer.h"
#include "compiler/stage.h"
#include "compiler/threading/profiler.h"

void FileToTokensF::execute(SrcFilePtr file, DataStream<std::pair<SrcFilePtr, std::vector<Token *>>> &os) {
  stage::set_name("Split file to tokens");
  stage::set_file(file);
  kphp_assert(file);

  kphp_assert(file->loaded);
  auto tokens = php_text_to_tokens(&file->text[0], (int)file->text.length());

  if (stage::has_error()) {
    return;
  }

  os << make_pair(file, std::move(tokens));
}
