#include <gtest/gtest.h>
#include "runtime/streams.h"
#include "runtime/critical_section.h"

TEST(stream_select, udp_writable) {
  mixed err_no;
  mixed err_desc;
  Stream udp_stream = f$stream_socket_client(
      CONST_STRING("udp://127.0.0.1:9"), err_no, err_desc, 1.0);

  ASSERT_TRUE(f$boolval(udp_stream));

  mixed read_arr = array<Stream>();
  mixed write_arr = array<Stream>::create(udp_stream);
  mixed except_arr = array<Stream>();

  Optional<int64_t> res = f$stream_select(read_arr, write_arr, except_arr, 0, 0);
  ASSERT_TRUE(res.has_value());
  ASSERT_EQ(res.val(), 1);

  f$fclose(udp_stream);
}
