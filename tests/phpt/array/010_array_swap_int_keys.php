@ok
<?php

require_once 'kphp_tester_include.php';


class A {
  /** @var int[] */
  var $arr = [0, 1, 2, 3, 4, 5];
}

function test_int_swap_1(array $a1) {
  $n = count($a1);

  array_swap_int_keys($a1, 0, 8);
  array_swap_int_keys($a1, 0, 8);
  array_swap_int_keys($a1, 0, 7);
  array_swap_int_keys($a1, 1, 6);
  array_swap_int_keys($a1, 2, $n - 1);

  array_swap_int_keys($a1, 1, 100);
  array_swap_int_keys($a1, 1, -100);
  array_swap_int_keys($a1, -1, 0);
  array_swap_int_keys($a1, -1, -100);

  array_swap_int_keys($a1, 0, 0);
  array_swap_int_keys($a1, 1, 1);

  var_dump($a1);
}

function test_int_swap_2(array $a1) {
  $n = count($a1);
  $a_copy = $a1;   // refcnt++

  array_swap_int_keys($a1, 0, 8);
  array_swap_int_keys($a1, 0, 8);
  array_swap_int_keys($a1, 0, 7);
  array_swap_int_keys($a1, 1, 6);
  array_swap_int_keys($a1, 2, $n - 1);

  array_swap_int_keys($a1, 1, 100);
  array_swap_int_keys($a1, 1, -100);
  array_swap_int_keys($a1, -1, 0);
  array_swap_int_keys($a1, -1, -100);

  array_swap_int_keys($a1, 0, 0);
  array_swap_int_keys($a1, 1, 1);

  var_dump($a1);
  var_dump($a_copy);
}

function swap_by_ref_01(array &$a) {
  $n = count($a);
  array_swap_int_keys($a, 0, 1);
  array_swap_int_keys($a, 0, $n - 1);
}

function test_int_swap_by_ref() {
  $a1 = [90,91,92];
  $a2 = $a1;
  swap_by_ref_01($a1);
  var_dump($a1);
  var_dump($a2);

  $o1 = new A;
  $o2 = $o1;
  swap_by_ref_01($o1->arr);
  var_dump($o1->arr);
  var_dump($o2->arr);
}

function test_string_swap() {
  $a = ["a", "b", "hello", "hello"];
  array_swap_int_keys($a, 0, 2);
  array_swap_int_keys($a, 0, 100);
  array_swap_int_keys($a, 1, 3);
  var_dump($a);
}

function test_tuple_swap() {
  $a = [tuple(1, "a"), tuple(2, "b"), tuple(3, "c")];
  $s0 = $a[0][0];
  $t0 = $a[0];
  $a2 = $a;
  array_swap_int_keys($a, 0, 2);
  foreach ($a as $v) {
    echo $v[0], ' ', $v[1], "\n";
  }
  foreach ($a2 as $v) {
    echo $v[0], ' ', $v[1], "\n";
  }
  echo $s0, ' ', $t0[0], "\n";
}

function test_instance_swap() {
  $a = [new A, new A, new A];
  $a2 = $a;
  $o0 = $a[0];
  $a[0]->arr[] = 100500;
  array_swap_int_keys($a, 0, 1);
  foreach ($a as $o) {
    var_dump(to_array_debug($o));
  }
  foreach ($a2 as $o) {
    var_dump(to_array_debug($o));
  }
  var_dump(to_array_debug($o0));
}

// vectors
test_int_swap_1([0,1,2,3,4,5,6,7,8,9,10]);
test_int_swap_1([0, 1]);
test_int_swap_1([]);
test_int_swap_2([0,1,2,3,4,5,6,7,8,9,10]);
test_int_swap_2([0, 1]);
test_int_swap_2([]);
// non vectors
test_int_swap_1([0=>0,1=>1,2=>2,3=>3,4=>4,5=>5,6=>6,7=>7,8=>8,9=>9,10=>10, 100=>100, -1=>1]);
test_int_swap_1([-100=>-100, -1=>1]);
test_int_swap_2([0=>0,1=>1,2=>2,3=>3,4=>4,5=>5,6=>6,7=>7,8=>8,9=>9,10=>10, 100=>100, -1=>1]);
test_int_swap_2([-100=>-100, -1=>1]);

test_int_swap_by_ref();

test_string_swap();
test_tuple_swap();
test_instance_swap();
