#include "runtime-light/stdlib/superglobals.h"

#include "runtime-light/component/component.h"
#include "runtime-light/allocator/allocator.h"
#include "runtime-light/streams/streams.h"

task_t<void> init_superglobals() {
   ComponentState & ctx = *get_component_context();
   auto [buffer, size] = co_await read_all_from_stream(ctx.standard_stream);

   get_component_context()->superglobals.v$POST.assign(buffer, size);
   get_platform_allocator()->free(buffer);
}

