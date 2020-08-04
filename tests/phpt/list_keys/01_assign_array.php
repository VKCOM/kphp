@ok
<?php

class Pair {
  public $a = 0;
  public $b = 0;
}

define('ZERO', 0);

function test_assign_int_index(array $x) {
  list(ZERO => $first) = $x;
  list(1 => $second,) = $x; // Trailing comma is OK

  var_dump($first, $second);
}

function test_assign_str_index(array $x) {
  $first = -1;
  list('b' => $second, 'a' => $first) = $x;

  var_dump($first, $second);
}

function test_assign_var_index(array $x) {
  $key2 = 'b';
  $keys = ['a', $key2];
  [$keys[0] => $first] = $x;
  [$key2 => $second] = $x;

  var_dump($first, $second);
}

function test_assign_field_lvalue(array $props) {
  $p = new Pair();
  list('a' => $p->a, 'b' => $p->b) = $props;
  var_dump($p->a);
  var_dump($p->b);
}

function test_assign_repeated() {
  // This tests a rewrite from the ConvertListAssignmentsPass.
  $create_array = function() {
    return [[42]];
  };
  list(0 => $v) = $create_array();
  list(0 => $v) = $v;
  var_dump($v);
}

test_assign_int_index(['a', 'b']);
test_assign_str_index(['a' => 10, 'b' => 20]);
test_assign_var_index(['a' => 3.5, 'b' => 4.7]);
test_assign_field_lvalue(['a' => 5, 'b' => 50]);
test_assign_repeated();
