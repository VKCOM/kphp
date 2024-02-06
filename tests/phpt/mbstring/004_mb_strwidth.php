@ok
<?php

// Test case 1: Basic test with ASCII characters
function test_mb_strwidth_basic() {
  var_dump(mb_strwidth('Hello', "ASCII"));
}

// Test case 2: Test with halfwidth characters
function test_mb_strwidth_halfwidth() {
  var_dump(mb_strwidth('こんにちは', "UTF-8"));
}

// Test case 3: Test with fullwidth characters
function test_mb_strwidth_fullwidth() {
  var_dump(mb_strwidth('Ｈｅｌｌｏ', "UTF-8"));
}

// Test case 4: Test with mixed halfwidth and fullwidth characters
function test_mb_strwidth_mixed() {
  var_dump(mb_strwidth('Ｈｅｌｌｏ, こんにちは', "UTF-8"));
}

// Test case 5: Test with mixed halfwidth and fullwidth characters
function test_mb_strwidth_special_characters() {
  var_dump(mb_strwidth('🙂👍', "UTF-8"));
}

// Test case 6: Test with specified encoding (Shift_JIS)
function test_mb_strwidth_encoding() {
  var_dump(mb_strwidth('こんにちは', "Shift_JIS"));
}

// Test case 7: Test with string containing whitespace
function test_mb_strwidth_whitespaces() {
  var_dump(mb_strwidth('  ', "UTF-8"));
}

// Test case 8: Test with string containing newlines
function test_mb_strwidth_newlines() {
  var_dump(mb_strwidth("Hello\nWorld", "UTF-8"));
}

// Test case 9: Test with string containing tabs
function test_mb_strwidth_tabs() {
  var_dump(mb_strwidth("Hello\tWorld", "UTF-8"));
}

// Test case 10: Test with string control characters
function test_mb_strwidth_control_characters() {
  var_dump(mb_strwidth("\x00\x01\x02", "UTF-8"));
}

test_mb_strwidth_basic();
test_mb_strwidth_halfwidth();
test_mb_strwidth_fullwidth();
test_mb_strwidth_mixed();
test_mb_strwidth_special_characters();
test_mb_strwidth_encoding();
test_mb_strwidth_whitespaces();
test_mb_strwidth_newlines();
test_mb_strwidth_tabs();
test_mb_strwidth_control_characters();
