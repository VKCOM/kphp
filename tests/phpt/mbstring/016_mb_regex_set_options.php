@ok
<?php

// Test case 1: Set and get options
function test_mb_regex_set_options_basic() {
    $original_options = mb_regex_set_options();
    var_dump(mb_regex_set_options("is"));
    var_dump(mb_regex_set_options());
    mb_regex_set_options($original_options);
}

// Test case 2: Set multiple options
function test_mb_regex_set_options_multiple() {
    $original_options = mb_regex_set_options();
    var_dump(mb_regex_set_options("ixm"));
    var_dump(mb_regex_set_options());
    mb_regex_set_options($original_options);
}

// Test case 3: Set mode option
function test_mb_regex_set_options_mode() {
    $original_options = mb_regex_set_options();
    var_dump(mb_regex_set_options("j"));
    var_dump(mb_regex_set_options());
    mb_regex_set_options($original_options);
}

// Test case 4: Set invalid option
function test_mb_regex_set_options_invalid() {
    $original_options = mb_regex_set_options();
    var_dump(mb_regex_set_options("z"));
    var_dump(mb_regex_set_options());
    mb_regex_set_options($original_options);
}

// Test case 5: Set empty string
function test_mb_regex_set_options_empty_string() {
    $original_options = mb_regex_set_options();
    var_dump(mb_regex_set_options(""));
    var_dump(mb_regex_set_options());
    mb_regex_set_options($original_options);
}

// Test case 6: Set null
function test_mb_regex_set_options_null() {
    $original_options = mb_regex_set_options();
    var_dump(mb_regex_set_options(null));
    var_dump(mb_regex_set_options());
    mb_regex_set_options($original_options);
}

// Test case 7: Set deprecated 'e' option (for PHP versions < 7.1.0)
function test_mb_regex_set_options_deprecated_e() {
    $original_options = mb_regex_set_options();
    var_dump(mb_regex_set_options("e"));
    var_dump(mb_regex_set_options());
    mb_regex_set_options($original_options);
}

// Run the tests
test_mb_regex_set_options_basic();
test_mb_regex_set_options_multiple();
test_mb_regex_set_options_mode();
test_mb_regex_set_options_invalid();
test_mb_regex_set_options_empty_string();
test_mb_regex_set_options_null();
test_mb_regex_set_options_deprecated_e();
