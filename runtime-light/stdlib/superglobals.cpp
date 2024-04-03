#include "runtime-light/stdlib/superglobals.h"

#include "runtime-light/component/component.h"
#include "runtime-light/allocator/allocator.h"
#include "runtime-light/streams/streams.h"

void init_superglobals(const char * buffer, int size) {
   ComponentState & ctx = *get_component_context();
   get_component_context()->superglobals.v$_POST.assign(buffer, size);
}

