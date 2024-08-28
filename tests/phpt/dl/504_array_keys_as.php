@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

function test_empty() {
  var_dump(array_keys_as_strings([]));
  var_dump(array_keys_as_ints([]));
}

function test_vector() {
  $a = ["hello", "world", [1], 2, 3];
  var_dump(array_keys_as_strings($a));
  var_dump(array_keys_as_ints($a));
}

function test_int_keys() {
  $a = [2 => "hello", 7 => "world", 9 => [1],  -7 => 2, 0 => 3];
  var_dump(array_keys_as_strings($a));
  var_dump(array_keys_as_ints($a));
}

function test_string_keys() {
  $a = ["2" => "hello", "str_key" => "world", "9 and some" => [1], "key12" => true, "-7" => 2, "0" => 3];
  var_dump(array_keys_as_strings($a));
  var_dump(array_keys_as_ints($a));
}

function test_mixed_keys() {
  $a = [
    "str_key" => "value",
    42 => "world",
    "9 and some" => [1],
    -24 => true,
    "-7" => 2,
    0 => false
  ];
  var_dump(array_keys_as_strings($a));
  var_dump(array_keys_as_ints($a));
}

function test_strange_keys() {
  $a = [true => "value", false => "world", 2.2 => [1], null => "null"];
  var_dump(array_keys_as_strings($a));
  var_dump(array_keys_as_ints($a));
}

test_empty();
test_vector();
test_int_keys();
test_string_keys();
test_mixed_keys();
test_strange_keys();
