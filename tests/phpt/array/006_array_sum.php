@ok
<?php

/**
 * @param float $x
 * @return float
 */
function fix_php($x) {
#ifndef KPHP
  return $x * 1.0;
#endif
  return $x;
}

function test_array_sum_empty() {
  var_dump(fix_php(array_sum([])));
}

function test_array_sum_int() {
  var_dump(array_sum([1, 2, 3, 4, 5]));
  var_dump(fix_php(array_sum([1, 2, 3, 4, 5, false, null])));
}

function test_array_sum_double() {
  var_dump(array_sum([1.1, 2.2, 3.3, 4.4, 5.5]));
  var_dump(array_sum([1.1, 2.2, 3.3, 4.4, 5.5, 6, false, null]));
}

function test_array_sum_string() {
  var_dump(fix_php(array_sum(["ss", "xx", "gg", "pp"])));
  var_dump(fix_php(array_sum(["1", "2", "3", "4"])));
  var_dump(array_sum(["1.2", "2.1", "3.5", "4.2"]));
  var_dump(array_sum(["1.2", "2.1", "3.5", "4.2", false, null]));
}

function test_array_sum_bool() {
  var_dump(fix_php(array_sum([true, false, true, false])));
  var_dump(fix_php(array_sum([false, false])));
  var_dump(fix_php(array_sum([null, null])));
  var_dump(fix_php(array_sum([false, false, null])));
  var_dump(fix_php(array_sum([true, false, true, false, null])));
}

function test_array_sum_var() {
  var_dump(array_sum([true, 2.3, "das", "23.1", null]));
}

test_array_sum_empty();
test_array_sum_int();
test_array_sum_double();
test_array_sum_string();
test_array_sum_bool();
test_array_sum_var();
