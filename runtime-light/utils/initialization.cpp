#include "runtime-light/utils/initialization.h"

#include "runtime-light/stdlib/superglobals.h"
#include "runtime-light/streams/streams.h"
#include "runtime-light/component/component.h"

task_t<void> script_init() {
  ComponentState & ctx = *get_component_context();
  auto [buffer, size] = co_await read_all_from_stream(ctx.standard_stream);
//  init_superglobals();
  get_component_context()->superglobals.v$_POST.assign(buffer, size);
  get_platform_allocator()->free(buffer);
  co_return ;
}