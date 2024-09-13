@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

function test_array_unset_vector() {
  $arr = ["foo", "bar", "baz"];

  $unexisted_elem = array_unset($arr, 3);
  #ifndef KPHP
  $unexisted_elem = "";
  #endif
  var_dump($unexisted_elem);
  var_dump($arr);

  $baz = array_unset($arr, 2);
  var_dump($baz);
  var_dump($arr);

  $foo = array_unset($arr, 0);
  var_dump($foo);
  var_dump($arr);

  $bar = array_unset($arr, 1);
  var_dump($bar);
  var_dump($arr);
}

function test_array_unset_map() {
  $arr = ["a" => "foo", "b" => "bar", "c" => "baz"];

  $unexisted_elem = array_unset($arr, "d");
  #ifndef KPHP
  $unexisted_elem = "";
  #endif
  var_dump($unexisted_elem);
  var_dump($arr);

  $baz = array_unset($arr, "c");
  var_dump($baz);
  var_dump($arr);

  $foo = array_unset($arr, "a");
  var_dump($foo);
  var_dump($arr);

  $bar = array_unset($arr, "b");
  var_dump($bar);
  var_dump($arr);
}

function test_unset_empty_array() {
  $arr = [];
  array_unset($arr, "something");
  var_dump($arr);
}

function test_unset_mixed_key() {
  $arr = ["foo", "bar"];

  /** @var mixed */
  $unexised_key = "qux";
  $unexisted_elem = array_unset($arr, $unexised_key);
  #ifndef KPHP
  $unexisted_elem = "";
  #endif
  var_dump($unexisted_elem);
  var_dump($arr);

  /** @var mixed */
  $key1 = "1";
  $bar = array_unset($arr, $key1);
  var_dump($bar);
  var_dump($arr);

  /** @var mixed */
  $key0 = 0;
  $foo = array_unset($arr, $key0);
  var_dump($foo);
  var_dump($arr);
}

function test_unset_mixed_array() {
  /** @var mixed */
  $arr = ["foo", "bar"];

  $unexisted_elem = array_unset($arr, "qux");
  var_dump($unexisted_elem);
  var_dump($arr);

  $bar = array_unset($arr, 1);
  var_dump($bar);
  var_dump($arr);

  $foo = array_unset($arr, 0);
  var_dump($foo);
  var_dump($arr);
}

test_array_unset_vector();
test_array_unset_map();
test_unset_empty_array();
test_unset_mixed_key();
test_unset_mixed_array();
