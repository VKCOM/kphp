#include <gtest/gtest.h>

#include <zstd.h>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/string/string-context.h"
#include "runtime/zstd.h"

TEST(zstd_test, test_bounds) {
  ASSERT_LE(ZSTD_CStreamOutSize(), StringLibContext::STATIC_BUFFER_LENGTH);
  ASSERT_LE(ZSTD_CStreamInSize(), StringLibContext::STATIC_BUFFER_LENGTH);

  ASSERT_LE(ZSTD_DStreamOutSize(), StringLibContext::STATIC_BUFFER_LENGTH);
  ASSERT_LE(ZSTD_DStreamInSize(), StringLibContext::STATIC_BUFFER_LENGTH);
}

TEST(zstd_test, test_compress_decompress) {
  const char *encoded_string = f$zstd_uncompress(f$zstd_compress(string("sample string"), 3).val()).val().c_str();
  ASSERT_STREQ("sample string", encoded_string);
}

TEST(zstd_test, test_stream_compression) {
  class_instance<C$ZstdCtx> ctx = f$zstd_stream_init();
  string first_chunk = string("consequat omittam consequat suavitate vocibus contentiones dolores causae appetere eius dicit vis penatibus postulant egestas sanctus mollis verear faucibus salutatus");
  string second_chunk = string("tacimates imperdiet augue equidem oporteat mentitum sapien aliquam primis assueverit pretium maiorum constituto novum taciti expetenda mauris an euripidis quam");
  string first_chunk_compressed = f$zstd_stream_add(ctx, first_chunk, false).val();
  string second_chunk_compressed = f$zstd_stream_add(ctx, second_chunk, true).val();

  string whole = first_chunk_compressed.copy_and_make_not_shared().append(second_chunk_compressed);
  ASSERT_STREQ(f$zstd_uncompress(first_chunk_compressed).val().c_str(), first_chunk.c_str());
  ASSERT_STREQ(f$zstd_uncompress(whole).val().c_str(), first_chunk.append(second_chunk).c_str());
}
