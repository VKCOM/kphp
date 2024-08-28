@ok k2_skip
<?php

function test_array_combine_empty() {
  var_dump(array_combine([], []));

#ifndef KPHP
  var_dump([]);
  var_dump([]);
  return;
#endif

  var_dump(array_combine([], [1, 2, 2, 3, 1]));
  var_dump(array_combine([1, 2, 2, 3, 1], []));
}

function test_array_combine_ints() {
  $a1 = [1, 2, 2, 3, 1];
  var_dump(array_combine($a1, $a1));
  $a2 = [1, false, 2, null, 3, 1, false, null];
  var_dump(array_combine($a2, $a2));
}

function test_array_combine_strings() {
  $a1 = ["hello", "foo", "bar", "baz", "foo", ""];
  var_dump(array_combine($a1, $a1));
  $a2 = [null, "hello", false, "foo", false, "", "bar", "baz", null, "foo"];
  var_dump(array_combine($a2, $a2));
}

function test_array_combine_doubles() {
  $a1 = [1.2, 2.3, 1.2, 42.1];
  var_dump(array_combine($a1, $a1));
  $a2 = [false, 1.2, null, false, 2.3, 1.2, null, 42.1];
  var_dump(array_combine($a2, $a2));
}

function test_array_combine_bools() {
  $a1 = [true, false, true, true, false];
  var_dump(array_combine($a1, $a1));
  $a2 = [null, false, null, null, false];
  var_dump(array_combine($a2, $a2));
  $a3 = [null, false, null, true, false];
  var_dump(array_combine($a3, $a3));
}

function test_array_combine_arrays() {
  $a1 = [[1], [1], [2]];
  var_dump(array_combine($a1, $a1));
  $a2 = [null, [1], false, [1], [2], false, null];
  var_dump(array_combine($a2, $a2));
}

function test_array_combine_vars() {
  $a1 = [1, false, 2.0, null, 3, 1, 0, "xxx", [1], "Array", ""];
  var_dump(array_combine($a1, $a1));
}

test_array_combine_empty();
test_array_combine_ints();
test_array_combine_strings();
test_array_combine_doubles();
test_array_combine_bools();
test_array_combine_arrays();
test_array_combine_vars();
