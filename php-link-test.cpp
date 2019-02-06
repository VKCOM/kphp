#include <cassert>

extern "C" {

void init_php_scripts(void) {
  assert(0 && "this code shouldn't be executed and only for linkage test");
}
void global_init_php_scripts(void) {
  assert(0 && "this code shouldn't be executed and only for linkage test");
}

}
