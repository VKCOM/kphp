@ok
<?php

// Test Case 1: Basic test with a simple string
function test_mb_strripos_basic() {
  var_dump(mb_strripos("hello world", "o", 0, "UTF-8"));
}

// Test Case 2: Basic test with offset
function test_mb_strripos_with_offset() {
  var_dump(mb_strripos("hello world", "o", 5, "UTF-8"));
}

// Test Case 3: Test with negative offset
function test_mb_strripos_with_neg_offset() {
  var_dump(mb_strripos("hello world", "o", -5, "UTF-8"));
}

// Test Case 4: Test with multibyte characters
function test_mb_strripos_multibyte() {
  var_dump(mb_strripos("こんにちは世界", "世界", 5, "UTF-8"));
}

// Test Case 5: Test with empty needle
function test_mb_strripos_empty_needle() {
  var_dump(mb_strripos("hello world", "", 0, "UTF-8"));
}

// Test Case 6: Test with needle not found
function test_mb_strripos_needle_not_found() {
  var_dump(mb_strripos("hello world", "foo", 0, "UTF-8"));
}

// Test Case 7: Test with offset greater than haystack length
function test_mb_strripos_big_offset() {
  var_dump(mb_strripos("hello world", "world", 100, "UTF-8"));
}

// Test Case 8: Test with case-insensitive search
function test_mb_strripos_case_insensitive() {
  var_dump(mb_strripos("Hello World", "world", 0, "UTF-8"));
}

test_mb_strripos_basic();
test_mb_strripos_with_offset();
test_mb_strripos_with_neg_offset();
test_mb_strripos_multibyte();
test_mb_strripos_empty_needle();
test_mb_strripos_needle_not_found();
test_mb_strripos_big_offset();
test_mb_strripos_case_insensitive();
