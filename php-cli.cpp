#include "drinkless/dl-utils-lite.h"

#include "php_functions.h"
#include "PHP/php_script.h"

static char memory_buffer[1 << 29];

void init_php_scripts();
void global_init_php_scripts();

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
  if (!CurException.is_null()) {
    fprintf(stderr, "Unhandled Exception caught in file %s at line %d. Error %d: %s.\n",
      CurException->file.c_str(), CurException->line,
      CurException->code, CurException->message.c_str());
    fprintf(stderr, "Backtrace:\n%s", f$Exception$$getTraceAsString(CurException).c_str());
  }
  script->clear();
  free_runtime_environment();
  return 0;
}
