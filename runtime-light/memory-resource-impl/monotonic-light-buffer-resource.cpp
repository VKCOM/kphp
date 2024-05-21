#include "runtime-core/memory-resource/monotonic_buffer_resource.h"

namespace memory_resource {
//todo:k2 How it should work in k2?
void monotonic_buffer_resource::critical_dump(void *, size_t ) const noexcept {

}

void monotonic_buffer_resource::raise_oom(size_t ) const noexcept {

}
}