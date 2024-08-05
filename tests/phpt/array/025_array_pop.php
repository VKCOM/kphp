@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

function test_array_pop_vector() {
  $arr = ["foo", "bar", "baz"];
  while($arr) {
    $elem = array_pop($arr);
    var_dump($elem);
  }
}

function test_array_pop_map() {
  $arr = ["a" => "foo", "b" => "bar", "c" => "baz"];
  while($arr) {
    $elem = array_pop($arr);
    var_dump($elem);
  }
}

function test_array_pop_empty_array() {
  $arr = [];
  array_pop($arr);
  var_dump($arr);
}

test_array_pop_vector();
test_array_pop_map();
test_array_pop_empty_array();
