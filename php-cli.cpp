#include "php_functions.h"
#include "php_script.h"
#include "drinkless/dl-utils-lite.h"

static char memory_buffer[1 << 29];

extern "C" {
  void init_scripts (void);
  void static_init_scripts (void);
}

int main (int argc, char **argv) {
  int arg_index;
  for (arg_index = 0; arg_index != argc; arg_index++) {
    arg_add (argv[arg_index]);
  }

  dl_set_default_handlers();
  static_init_scripts();
  init_scripts();
  script_t *script = get_script ("#0");
#ifdef FAST_EXCEPTIONS
  script->run (NULL, memory_buffer, 1 << 29);
  if (CurException) {
    Exception e = *CurException;
#else
  try {
    script->run (NULL, memory_buffer, 1 << 29);
  } catch (Exception &e) {
#endif
    fprintf (stderr, "Unhandled Exception caught in file %s at line %d. Error %d: %s.\n", e.file.c_str(), e.line, e.code, e.message.c_str());
    fprintf (stderr, "Backtrace:\n%s", f$exception_getTraceAsString (e).c_str());
  }
  script->clear();
  return 0;
}
