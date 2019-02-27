#include "drinkless/dl-utils-lite.h"

#include "php_functions.h"
#include "PHP/php_script.h"

static char memory_buffer[1 << 29];

extern "C" {
void init_php_scripts(void);
void global_init_php_scripts(void);
}

int main(int argc, char **argv) {
  int arg_index;
  for (arg_index = 0; arg_index != argc; arg_index++) {
    arg_add(argv[arg_index]);
  }

  dl_set_default_handlers();
  global_init_runtime_libs();
  global_init_php_scripts();
  global_init_script_allocator();
  init_php_scripts();
  init_runtime_environment(nullptr, memory_buffer, 1 << 29);
  script_t *script = get_script("#0");
  script->run();
  if (CurException) {
    Exception e = *CurException;
    fprintf(stderr, "Unhandled Exception caught in file %s at line %d. Error %d: %s.\n", e.file.c_str(), e.line, e.code, e.message.c_str());
    fprintf(stderr, "Backtrace:\n%s", f$Exception$$getTraceAsString(e).c_str());
  }
  script->clear();
  free_runtime_environment();
  return 0;
}
