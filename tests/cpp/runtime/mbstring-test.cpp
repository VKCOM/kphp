#include <gtest/gtest.h>
#include "runtime/mbstring/mbstring.h"

#ifdef MBFL
/* TODO: make fun strings for tests */

TEST(mbstring_test, test_mb_check_encoding) {
	ASSERT_TRUE(f$mb_check_encoding(string("sdf"), string("Windows-1251")));
	ASSERT_TRUE(f$mb_check_encoding(string("ыва"), string("Windows-1251")));
	ASSERT_TRUE(f$mb_check_encoding(string("Ä°nanÃ§ EsaslarÄ±"), string("UTF-8")));
	ASSERT_TRUE(f$mb_check_encoding(string("Ä°nanÃ§ EsaslarÄ±"), string("Windows-1251")));
	ASSERT_FALSE(f$mb_check_encoding(string("Ä°nanÃ§ EsaslarÄ±"), string("ASCII")));
}

TEST(mbstring_test, test_mb_convert_encoding) {
	ASSERT_STREQ(f$mb_convert_encoding(string("Hello"), string("UTF-8"), string("EUC-KR")).to_string().c_str(), "Hello");
	ASSERT_STREQ(f$mb_convert_encoding(string("ыавыа"), string("UTF-8"), string("Windows-1251")).to_string().c_str(), "С‹Р°РІС‹Р°");
	ASSERT_STREQ(f$mb_convert_encoding(string("ыва"), string("UTF-8"), string("ASCII")).to_string().c_str(), "??????");
}

#endif