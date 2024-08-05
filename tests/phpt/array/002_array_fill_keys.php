@ok k2_skip
<?php

function test_array_fill_keys_empty() {
  var_dump(array_fill_keys([], "foo"));
}

function test_array_fill_keys_ints() {
  var_dump(array_fill_keys([1, 2, 2, 3, 1], "foo"));
  var_dump(array_fill_keys([1, false, 2, null, 3, 1, false, null], "foo"));
}

function test_array_fill_keys_strings() {
  var_dump(array_fill_keys(["hello", "foo", "bar", "baz", "foo", ""], 123));
  var_dump(array_fill_keys([null, "hello", false, "foo", false, "", "bar", "baz", null, "foo"], 123));
}

function test_array_fill_keys_doubles() {
  var_dump(array_fill_keys([1.2, 2.3, 1.2, 42.1], "foo"));
  var_dump(array_fill_keys([false, 1.2, null, false, 2.3, 1.2, null, 42.1], "foo"));
}

function test_array_fill_keys_bools() {
  var_dump(array_fill_keys([true, false, true, true, false], "foo"));
  var_dump(array_fill_keys([null, false, null, null, false], "foo"));
  var_dump(array_fill_keys([null, false, null, true, false], "foo"));
}

function test_array_fill_keys_arrays() {
  var_dump(array_fill_keys([[1], [1], [2]], "foo"));
  var_dump(array_fill_keys([null, [1], false, [1], [2], false, null], "foo"));
}

function test_array_fill_keys_vars() {
  var_dump(array_fill_keys([1, false, 2.0, null, 3, 1, 0, "xxx", [1], "Array", ""], "foo"));
}

test_array_fill_keys_empty();
test_array_fill_keys_ints();
test_array_fill_keys_strings();
test_array_fill_keys_doubles();
test_array_fill_keys_bools();
test_array_fill_keys_arrays();
test_array_fill_keys_vars();
