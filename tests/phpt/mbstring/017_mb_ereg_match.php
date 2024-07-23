@ok
<?php

// Test case 1: Basic match at the beginning of the string
function test_mb_ereg_match_basic() {
    var_dump(mb_ereg_match("abc", "abcdef"));
}

// Test case 2: Match fails when pattern is not at the beginning
function test_mb_ereg_match_not_beginning() {
    var_dump(mb_ereg_match("bcd", "abcdef"));
}

// Test case 3: Case-sensitive match
function test_mb_ereg_match_case_sensitive() {
    var_dump(mb_ereg_match("ABC", "abcdef"));
}

// Test case 4: Case-insensitive match using options
function test_mb_ereg_match_case_insensitive() {
    var_dump(mb_ereg_match("ABC", "abcdef", "i"));
}

// Test case 5: Match with multibyte characters
function test_mb_ereg_match_multibyte() {
    var_dump(mb_ereg_match("こん", "こんにちは", ""));
}

// Test case 6: Match with regex special characters
function test_mb_ereg_match_special_chars() {
    var_dump(mb_ereg_match("a.c", "abc"));
}

// Test case 7: Match with empty pattern
function test_mb_ereg_match_empty_pattern() {
    var_dump(mb_ereg_match("", "abcdef"));
}

// Test case 8: Match with empty string
function test_mb_ereg_match_empty_string() {
    var_dump(mb_ereg_match("abc", ""));
}

// Test case 9: Match with extended mode option
function test_mb_ereg_match_extended_mode() {
    var_dump(mb_ereg_match("a b c", "abc", "x"));
}

// Run the tests
test_mb_ereg_match_basic();
test_mb_ereg_match_not_beginning();
test_mb_ereg_match_case_sensitive();
test_mb_ereg_match_case_insensitive();
test_mb_ereg_match_multibyte();
test_mb_ereg_match_special_chars();
test_mb_ereg_match_empty_pattern();
test_mb_ereg_match_empty_string();
test_mb_ereg_match_extended_mode();
