#include "runtime/allocator.h"
#include "runtime/kphp_core.h"

string_buffer static_SB;
string_buffer static_SB_spare;

dl::size_type string_buffer::MIN_BUFFER_LEN = 266175;
dl::size_type string_buffer::MAX_BUFFER_LEN = (1 << 24);

int string_buffer::string_buffer_error_flag = 0;

string_buffer::string_buffer(dl::size_type buffer_len) noexcept:
  buffer_end(static_cast<char *>(dl::heap_allocate(buffer_len))),
  buffer_begin(buffer_end),
  buffer_len(buffer_len) {
}

string_buffer::~string_buffer() noexcept {
  dl::heap_deallocate(buffer_begin, buffer_len);
}
