@ok
<?php

// Test case 1: Basic test with ASCII string
function test_mb_strlen_basic_ascii() {
  var_dump(mb_strlen("Hello", "ASCII"));
}

// Test case 2: Basic test with multibyte characters (UTF-8)
function test_mb_strlen_basic_utf_8() {
  var_dump(mb_strlen("こんにちは", "UTF-8"));
}

// Test case 3: Testing with empty string
function test_mb_strlen_empty_string() {
  var_dump(mb_strlen("", "UTF-8"));
}

// Test case 4: Testing with null encoding parameter (should use internal encoding)
function test_mb_strlen_null_encoding() {
  var_dump(mb_strlen("你好"));
}

// Test case 5: Testing with specific encoding (UTF-16)
function test_mb_strlen_utf_16_encoding() {
  var_dump(mb_strlen("안녕하세요", "UTF-16"));
}

// Test case 6: Testing with HTML entities
function test_mb_strlen_html_entities() {
  var_dump(mb_strlen("&lt;p&gt;This is a test&lt;/p&gt;", "UTF-8"));
}

// Test case 7: Testing with whitespace characters
function test_mb_strlen_whitespaces() {
  var_dump(mb_strlen("   ", "UTF-8"));
}

// Test case 8: Testing with control characters
function test_mb_strlen_control_characters() {
  var_dump(mb_strlen("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F", "UTF-8"));
}

// Test case 9: Testing with a mixture of characters
function test_mb_strlen_mixed_characters() {
  var_dump(mb_strlen("Hello こんにちは 你好", "UTF-8"));
}

// Test case 10: Testing with a very large string
function test_mb_strlen_long_string() {
  $string = str_repeat("a", 100000);
  var_dump(mb_strlen($string, "UTF-8"));
}

test_mb_strlen_basic_ascii();
test_mb_strlen_basic_utf_8();
test_mb_strlen_empty_string();
test_mb_strlen_null_encoding(); // doesn't put null through for some reason
test_mb_strlen_utf_16_encoding();
test_mb_strlen_html_entities();
test_mb_strlen_whitespaces();
test_mb_strlen_control_characters();
test_mb_strlen_mixed_characters();
test_mb_strlen_long_string();
