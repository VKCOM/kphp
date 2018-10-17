#include <cassert>

extern "C" {

void init_scripts(void) {
  assert(0 && "this code shouldn't be executed and only for linkage test");
}
void static_init_scripts(void) {
  assert(0 && "this code shouldn't be executed and only for linkage test");
}

}
