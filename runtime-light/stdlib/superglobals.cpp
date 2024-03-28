#include "runtime-light/stdlib/superglobals.h"

#include "runtime-light/component/component.h"
#include "runtime-light/allocator/allocator.h"
#include "runtime-light/streams/streams.h"

extern mixed v$_POST;

void init_superglobals() {
  // ComponentState & ctx = *get_component_context();
  // string str = read_all_from_stream(ctx.standard_stream);
  //
  // v$_POST.assign(buffer, size);
}

