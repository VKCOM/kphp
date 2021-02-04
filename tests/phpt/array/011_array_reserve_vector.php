@ok
<?php

require_once 'kphp_tester_include.php';

function test_array_reserve_vector_empty() {
  $a = [];
  array_reserve_vector($a, 10);
  var_dump($a);
}

function test_array_reserve_vector_with_int_value() {
  $a = ['test'];
  array_reserve_vector($a, 10);
  var_dump($a);
}

function test_array_reserve_vector_with_int_key() {
  $a = [5 => 'value'];
  array_reserve_vector($a, 10);
  var_dump($a);
}

function test_array_reserve_vector_with_string_value() {
  $a = ['test'];
  array_reserve_vector($a, 10);
  var_dump($a);
}

function test_array_reserve_vector_with_string_key() {
  $a = ['test' => 'value'];
  array_reserve_vector($a, 10);
  var_dump($a);
}

test_array_reserve_vector_empty();
test_array_reserve_vector_with_int_key();
test_array_reserve_vector_with_int_value();
test_array_reserve_vector_with_string_key();
test_array_reserve_vector_with_string_value();
