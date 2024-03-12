@ok
<?php

// Test case 1: Basic test with single occurrence
function test_mb_substr_count_basic() {
  var_dump(mb_substr_count("hello world", "world", "UTF-8"));
}

// Test case 2: Test with multiple occurrences
function test_mb_substr_count_basic_multiple() {
  var_dump(mb_substr_count("hello world, hello universe", "hello", "UTF-8"));
}

// Test case 3: Test with empty haystack
function test_mb_substr_count_empty_haystack() {
  var_dump(mb_substr_count("", "hello", "UTF-8"));
}

// Test case 4: Test with ru letters
function test_mb_substr_count_basic_ru() {
  var_dump(mb_substr_count("привет мир", "мир", "UTF-8"));
}

// Test case 5: Test with non-UTF-8 encoding
function test_mb_substr_count_non_utf_8() {
  var_dump(mb_substr_count("пこんにちは世界", "世界", "SJIS"));
}

// Test case 6: Test with haystack and needle being identical
function test_mb_substr_count_haystack_equals_needle() {
  var_dump(mb_substr_count("hello", "hello", "UTF-8"));
}

// Test case 7: Test with haystack containing repeating needle
function test_mb_substr_count_haystack_is_repeated_needle() {
  var_dump(mb_substr_count("aaaaaa", "aa", "UTF-8"));
}

// Test case 8: Test with Unicode characters
function test_mb_substr_count_unicode() {
  var_dump(mb_substr_count("🎉🎉🎉", "🎉", "Unicode"));
}

// Test case 9: Test with needle not found
function test_mb_substr_basic_not_found() {
  var_dump(mb_substr_count("hello world", "foo", "UTF-8"));
}

test_mb_substr_count_basic();
test_mb_substr_count_basic_multiple();
test_mb_substr_count_empty_haystack();
test_mb_substr_count_basic_ru();
test_mb_substr_count_non_utf_8();
test_mb_substr_count_haystack_equals_needle();
test_mb_substr_count_haystack_is_repeated_needle();
test_mb_substr_count_unicode();
test_mb_substr_basic_not_found();
