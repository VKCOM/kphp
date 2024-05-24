@ok
<?php

// Test case 1: Basic test with ASCII characters
function test_mb_strtolower_basic_ascii() {
  var_dump(mb_strtolower("HELLO WORLD", "ASCII"));
}

// Test Case 2: Basic test with non-ASCII characters
function test_mb_strtolower_basic_utf_8() {
  var_dump(mb_strtolower("HÉLLØ WØRLD", "UTF-8"));
}

// Test Case 3: Test with an empty string
function test_mb_strtolower_empty() {
  var_dump(mb_strtolower("", "UTF-8"));
}

// Test Case 4: Test with numbers and special characters
function test_mb_strtolower_numbers() {
  var_dump(mb_strtolower("123!@#", "UTF-8"));
}

// Test Case 5: Test with a mix of upper and lower case characters
function test_mb_strtolower_mixed() {
  var_dump(mb_strtolower("Hello WoRlD", "UTF-8"));
}

test_mb_strtolower_basic_ascii();
test_mb_strtolower_basic_utf_8();
test_mb_strtolower_empty();
test_mb_strtolower_numbers();
test_mb_strtolower_mixed();
