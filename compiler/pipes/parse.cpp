#include "compiler/pipes/parse.h"

#include "compiler/gentree.h"

void ParseF::execute(FileAndTokens file_and_tokens, DataStream<FunctionPtr> &os) {
  AUTO_PROF(gentree);
  stage::set_name("Parse file");
  stage::set_file(file_and_tokens.file);
  kphp_assert(file_and_tokens.file);

  GenTreeCallback callback(os);
  php_gen_tree(file_and_tokens.tokens, file_and_tokens.file->class_context, file_and_tokens.file->main_func_name, callback);
}
