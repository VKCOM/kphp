#include "drinkless/dl-utils-lite.h"

#include "php_functions.h"
#include "PHP/php_script.h"

static char memory_buffer[1 << 29];

extern "C" {
void init_scripts(void);
void static_init_scripts(void);
}

int main(int argc, char **argv) {
  int arg_index;
  for (arg_index = 0; arg_index != argc; arg_index++) {
    arg_add(argv[arg_index]);
  }

  dl_set_default_handlers();
  static_init_scripts();
  init_scripts();
  script_t *script = get_script("#0");
  script->run(nullptr, memory_buffer, 1 << 29);
  if (CurException) {
    Exception e = *CurException;
    fprintf(stderr, "Unhandled Exception caught in file %s at line %d. Error %d: %s.\n", e.file.c_str(), e.line, e.code, e.message.c_str());
    fprintf(stderr, "Backtrace:\n%s", f$Exception$$getTraceAsString(e).c_str());
  }
  script->clear();
  return 0;
}
