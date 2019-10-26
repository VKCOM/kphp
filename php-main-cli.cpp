#include "PHP/worker/php-engine.h"

int main(int argc, char *argv[]) {
  return run_main(argc, argv, php_mode::cli);
}
