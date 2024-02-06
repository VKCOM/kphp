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

TEST(mbstring_test, test_mb_strlen) {
  const int predicted = 12;
  int real = 0;
  real = f$mb_strlen(string("Hello world!"), string("UTF-8"));
  ASSERT_TRUE(real == predicted);
}

TEST(mbstring_test, test_mb_substr) {
  ASSERT_STREQ(f$mb_substr(string("Hello world"), 2, 3, string("UTF-8")).c_str(), "llo");
}

TEST(mbstring_test, test_mb_substr_null_length) {
  ASSERT_STREQ(f$mb_substr(string("Hello world"), 3, false, string("UTF-8")).c_str(), "lo world");
}

TEST(mbstring_test, test_mb_substr_count) {
  ASSERT_TRUE(f$mb_substr_count(string("Hello world"), string("l"), string("UTF-8")) == 3);
}

TEST(mbstring_test, test_mb_strpos) {
  ASSERT_TRUE(val(f$mb_strpos(string("This is a test string"), string("test"), 0, string("UTF-8"))) == 10);
}

TEST(mbstring_test, test_mb_strrpos) {
  ASSERT_TRUE(val(f$mb_strrpos(string("españololol"), string("ol"), 0, string("UTF-8"))) == 9);
}

TEST(mbstring_test, test_mb_strtoupper) {
  ASSERT_STREQ(f$mb_strtoupper(string("españololol"), string("UTF-8")).c_str(), "ESPAÑOLOLOL");
}

TEST(mbstring_test, test_mb_strtolower) {
  ASSERT_STREQ(f$mb_strtolower(string("ESPAÑOLOLOL"), string("UTF-8")).c_str(), "españololol");
}

TEST(mbstring_test, test_mb_stripos) {
  ASSERT_TRUE(val(f$mb_stripos(string("This is a tEsT string"), string("TeSt"), 0, string("UTF-8"))) == 10);
}

TEST(mbstring_test, test_mb_strripos) {
  ASSERT_TRUE(val(f$mb_strripos(string("espaÑOLOlol"), string("oL"), 0, string("UTF-8"))) == 9);
}

TEST(mbstring_test, test_mb_strwidth) {
  ASSERT_TRUE(val(f$mb_strwidth(string("現現"), string("UTF-8"))) == 4);
}

TEST(mbstring_test, test_mb_strstr) {
  ASSERT_STREQ(f$mb_strstr(string("This is a test string"), string("test"), true, string("UTF-8")).val().c_str(), "This is a ");
  ASSERT_STREQ(f$mb_strstr(string("This is a test string"), string("test"), false, string("UTF-8")).val().c_str(), "test string");
}

TEST(mbstring_test, test_mb_stristr_before_needle) {
  ASSERT_STREQ(val(f$mb_stristr(string("This is a tEsT string"), string("TeSt"), true, string("UTF-8"))).c_str(), "This is a ");
  ASSERT_STREQ(val(f$mb_stristr(string("This is a tEsT string"), string("TeSt"), false, string("UTF-8"))).c_str(), "tEsT string");
}

TEST(mbstring_test, test_mb_strrchr) {
  ASSERT_STREQ(f$mb_strrchr(string("This is a test string"), string("test"), true, string("UTF-8")).val().c_str(), "This is a ");
  ASSERT_STREQ(f$mb_strrchr(string("This is a test string"), string("test"), false, string("UTF-8")).val().c_str(), "test string");
}

TEST(mbstring_test, test_mb_strrichr) {
  ASSERT_STREQ(f$mb_strrichr(string("This is a test string"), string("test"), true, string("UTF-8")).val().c_str(), "This is a ");
  ASSERT_STREQ(f$mb_strrichr(string("This is a test string"), string("test"), false, string("UTF-8")).val().c_str(), "test string");
}

#endif