@ok
<?php

require_once 'kphp_tester_include.php';

function test_array_reserve_map_string_keys_empty() {
  $a = [];
  array_reserve_map_int_keys($a, 10);
  var_dump($a);
}

function test_array_reserve_map_string_with_values() {
  $a = ['key' => 'value'];
  array_reserve_map_string_keys($a, 10);
  var_dump($a);
}

test_array_reserve_map_string_keys_empty();
test_array_reserve_map_string_with_values();
