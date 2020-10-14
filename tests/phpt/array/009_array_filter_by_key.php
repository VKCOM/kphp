@ok
<?php

require_once 'kphp_tester_include.php';

/**
 * @param mixed[] $a
 */
function test_array_filter_by_key($a) {
  var_dump(array_filter_by_key($a, 'is_scalar'));
  var_dump(array_filter_by_key($a, 'is_numeric'));
  var_dump(array_filter_by_key($a, 'is_null'));
  var_dump(array_filter_by_key($a, 'is_bool'));
  var_dump(array_filter_by_key($a, 'is_int'));
  var_dump(array_filter_by_key($a, 'is_integer'));
  var_dump(array_filter_by_key($a, 'is_long'));
  var_dump(array_filter_by_key($a, 'is_float'));
  var_dump(array_filter_by_key($a, 'is_double'));
  var_dump(array_filter_by_key($a, 'is_real'));
  var_dump(array_filter_by_key($a, 'is_string'));
  var_dump(array_filter_by_key($a, 'is_array'));
  var_dump(array_filter_by_key($a, 'is_numeric'));
  var_dump(array_filter_by_key($a, function($x) { return $x < 4; }));
}


test_array_filter_by_key(["hello", 123, false, true, null, 0.2, "world", [123]]);
test_array_filter_by_key([
  "key1" => "hello",
  "key2" => 123,
  "3" => false,
  4 => true,
  "key5" => null,
  0.2,
  "key7" => "world",
  9999 => [123]]
);
