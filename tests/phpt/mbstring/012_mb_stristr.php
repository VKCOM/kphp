@ok
<?php

// Test Case 1: Basic test with needle found
function test_mb_stristr_basic() {
  var_dump(mb_stristr("hello world", "WORLD", false, "UTF-8"));
}

// Test Case 2: Test with needle found and before_needle set to true
function test_mb_stristr_before_needle_true() {
  var_dump(mb_stristr("hello world", "WORLD", true, "UTF-8"));
}

// Test Case 3: Test with needle found and before_needle set to false
function test_mb_stristr_before_needle_false() {
  var_dump(mb_stristr("hello world", "WORLD", false, "UTF-8"));
}

// Test Case 4: Test with needle found and encoding specified
function test_mb_stristr_encoding_specified() {
  var_dump(mb_stristr("Résumé", "É", false, "UTF-8"));
}

// Test Case 5: Test with needle found, before_needle set to true, and encoding specified
function test_mb_stristr_before_needle_true_encoding_specified() {
  var_dump(mb_stristr("Résumé", "É", true, "UTF-8"));
}

// Test Case 6: Test with needle found, before_needle set to false, and encoding specified
function test_mb_stristr_before_needle_false_encoding_specified() {
  var_dump(mb_stristr("Résumé", "É", false, "UTF-8"));
}

// Test Case 7: Test with needle not found
function test_mb_stristr_needle_not_found() {
  var_dump(mb_stristr("hello world", "UNIVERSE", false, "UTF-8"));
}

// Test Case 8: Test with needle found at the beginning of haystack
function test_mb_stristr_needle_at_beginning() {
  var_dump(mb_stristr("hello world", "HELLO", false, "UTF-8"));
}

// Test Case 9: Test with empty needle
function test_mb_stristr_empty_needle() {
  var_dump(mb_stristr("hello world", "", false, "UTF-8"));
}

// Test Case 10: Test with empty haystack
function test_mb_stristr_empty_haystack() {
  var_dump(mb_stristr("", "world", false, "UTF-8"));
}

test_mb_stristr_basic();
test_mb_stristr_before_needle_true();
test_mb_stristr_before_needle_false();
test_mb_stristr_encoding_specified();
test_mb_stristr_before_needle_true_encoding_specified();
test_mb_stristr_before_needle_false_encoding_specified();
test_mb_stristr_needle_not_found();
test_mb_stristr_needle_at_beginning();
test_mb_stristr_empty_needle();
test_mb_stristr_empty_haystack();
