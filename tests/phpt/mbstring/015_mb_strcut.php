@ok
<?php

// Test case 1: Extract all characters from start position to the end of the string
function test_mb_strcut_full_string() {
  var_dump(mb_strcut("abcdef", 0, null, "UTF-8"));
}

// Test case 2: Extract characters starting from the specified position
function test_mb_strcut_with_start() {
  var_dump(mb_strcut("abcdef", 2, null, "UTF-8"));
}

// Test case 3: Extract characters using negative start position
function test_mb_strcut_with_neg_start() {
  var_dump(mb_strcut("abcdef", -3, null, "UTF-8"));
}

// Test case 4: Extract a specific number of characters
function test_mb_strcut_with_length() {
  var_dump(mb_strcut("abcdef", 1, 3, "UTF-8"));
}

// Test case 5: Test with different encoding
function test_mb_strcut_different_encoding() {
  var_dump(mb_strcut("abcdef", 1, 3, "ISO-8859-1"));
}

// Test case 6: Test with non-ASCII characters
function test_mb_strcut_non_ascii_characters() {
  var_dump(mb_strcut("你好，世界！", 0, 3, "UTF-8"));
}

// Test case 7: Test with start position beyond string length
function test_mb_strcut_start_after_string_ends() {
  var_dump(mb_strcut("你好，世界！", 20, 3, "UTF-8"));
}

// Test case 8: Test with negative start position beyond string length
function test_mb_strcut_neg_start_after_string_ends() {
  var_dump(mb_strcut("你好，世界！", -20, 3, "UTF-8"));
}

// Test case 9: Test with empty string
function test_mb_strcut_empty_string() {
  var_dump(mb_strcut("你好，世界！", -20, 3, "UTF-8"));
}

test_mb_strcut_full_string();
test_mb_strcut_with_start();
test_mb_strcut_with_neg_start();
test_mb_strcut_with_length();
test_mb_strcut_different_encoding();
test_mb_strcut_non_ascii_characters();
test_mb_strcut_start_after_string_ends();
test_mb_strcut_neg_start_after_string_ends();
test_mb_strcut_empty_string();
