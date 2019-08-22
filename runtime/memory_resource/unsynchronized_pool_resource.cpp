#include "runtime/memory_resource/unsynchronized_pool_resource.h"


namespace memory_resource {

constexpr size_type unsynchronized_pool_resource::MAX_CHUNK_BLOCK_SIZE_;

void unsynchronized_pool_resource::init(void *buffer, size_type buffer_size) {
  monotonic_buffer_resource::init(buffer, buffer_size);

  new(&huge_pieces_storage_) map_type{map_allocator{*this}};
  huge_pieces_ = reinterpret_cast <map_type *> (&huge_pieces_storage_);
  free_chunks_.fill(details::unsynchronized_memory_chunk_list{});
}

} // namespace memory_resource
