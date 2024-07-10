@ok
<?php

// Test case 1: Basic test with ASCII string
function test_mb_strimwidth_basic_ascii() {
  var_dump(mb_strimwidth("Hello World", 0, 5, "..."));
}

// Test case 2: Basic test with multibyte characters (UTF-8)
function test_mb_strimwidth_basic_utf_8() {
  var_dump(mb_strimwidth("Ä°nanÃ§ EsaslarÄ±", 0, 5, "...", "UTF-8"));
}

// Test case 3: Testing with empty string
function test_mb_strimwidth_empty_string() {
  var_dump(mb_strimwidth("", 0, 5, "...", "UTF-8"));
}

// Test case 4: Testing with specific encoding (UTF-16)
function test_mb_strimwidth_utf_16_encoding() {
  var_dump(mb_strimwidth("안녕하세요", 0, 5, "...", "UTF-16"));
}

// Test case 5: Testing with trim marker
function test_mb_strimwidth_with_trim_marker() {
  var_dump(mb_strimwidth("This is a long string", 0, 10, "..."));
}

// Test case 6: Testing with negative width (deprecated)
function test_mb_strimwidth_negative_width() {
  var_dump(mb_strimwidth("This is a long string", 0, -5, "..."));
}

// Test case 7: Testing with negative start position
function test_mb_strimwidth_negative_start() {
  var_dump(mb_strimwidth("This is a long string", -10, 5, "..."));
}

// Test case 8: Testing with start position beyond string length
function test_mb_strimwidth_start_beyond_length() {
  var_dump(mb_strimwidth("This is a long string", 100, 5, "..."));
}

// Test case 9: Testing with width greater than string length
function test_mb_strimwidth_width_greater_than_length() {
  var_dump(mb_strimwidth("Hello", 0, 10, "..."));
}


test_mb_strimwidth_basic_ascii();
test_mb_strimwidth_basic_utf_8();
test_mb_strimwidth_empty_string();
test_mb_strimwidth_utf_16_encoding();
test_mb_strimwidth_with_trim_marker();
test_mb_strimwidth_negative_width();
test_mb_strimwidth_negative_start();
test_mb_strimwidth_start_beyond_length();
test_mb_strimwidth_width_greater_than_length();
