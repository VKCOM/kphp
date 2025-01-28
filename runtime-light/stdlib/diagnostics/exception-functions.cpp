#include "runtime-light/stdlib/diagnostics/exception-functions.h"

Exception f$Exception$$__construct([[maybe_unused]] const Exception &v$this, const string &message, [[maybe_unused]] int64_t code) {
  php_critical_error("exception with message %s", message.c_str());
  return {};
}
