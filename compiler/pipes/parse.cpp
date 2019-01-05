#include "compiler/pipes/parse.h"

#include "compiler/gentree.h"

void ParseF::execute(FileAndTokens file_and_tokens, DataStream<FunctionPtr> &os) {
  stage::set_name("Parse file");
  stage::set_file(file_and_tokens.file);
  kphp_assert(file_and_tokens.file);

  php_gen_tree(file_and_tokens.tokens, file_and_tokens.file, os);
}
