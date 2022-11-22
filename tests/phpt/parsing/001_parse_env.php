@ok
<?php

require_once 'kphp_tester_include.php';

function test_parse_env_string_empty() {
    var_dump(parse_env_string(''));
}

function test_parse_env_string_one() {
    var_dump(parse_env_string('APP_NAME=Laravel'));
}

function test_parse_env_string_many() {
    var_dump(parse_env_string('APP_NAME=Laravel APP_DEBUG=true APP_ENV=local'));
}

function test_parse_env_file_empty() {
    var_dump(parse_env_file(''));
}

function test_parse_env_file_not_found_empty() {
    var_dump(parse_env_file('file not found'));
}

function test_parse_env_file_one() {
    $filename = tempnam("", "wt");
    $fp = fopen($filename, "a");
    fwrite($fp, "APP_NAME=Laravel");
    fclose($fp);

    var_dump(parse_env_file($filename));
}

function test_parse_env_file_many() {
    $filename = tempnam("", "wt");
    $fp = fopen($filename, "a");
    fwrite($fp, "APP_NAME=Laravel");
    fwrite($fp, "APP_DEBUG=true");
    fwrite($fp, "APP_ENV=local");
    fclose($fp);

    var_dump(parse_env_file($filename));
}

test_parse_env_string_empty();
test_parse_env_string_one();
test_parse_env_string_many();

test_parse_env_file_empty();
test_parse_env_file_not_found_empty();
test_parse_env_file_one();
test_parse_env_file_many();
