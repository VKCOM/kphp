@ok k2_skip
<?php

function test_array_flip_empty() {
   var_dump(array_flip([]));
}

function test_array_flip_ints() {
  var_dump(array_flip([5, 2, 3, 1, 4]));
  var_dump(array_flip([5, 2, 3, 1, 4, 2, 5]));
  var_dump(array_flip([2 => 3, 7 => 2, 10 => 6, 1 => 1, 8 => 2, 4 => 1]));
  var_dump(array_flip(["foo" => 3, "bar" => 1, "baz" => 2, "gaz" => 3]));
  var_dump(array_flip(["foo" => 3, 99 => 2, "baz" => 1, "gaz" => 2]));
}

function test_array_flip_string() {
  var_dump(array_flip(["foo", "bar", "baz"]));
  var_dump(array_flip(["foo", "bar", "baz", "foo", "baz"]));
  var_dump(array_flip([8 => "foo", 10 => "bar", 22 => "baz", 1 => "foo"]));
  var_dump(array_flip(["xx" => "foo", "yy" => "bar", "zz" => "baz", "ww" => "foo"]));
  var_dump(array_flip(["xx" => "foo", 152 => "bar", "zz" => "baz", "yy" => "bar"]));
}

function test_array_flip_doubles() {
  var_dump(array_flip([5.5, 2.2, 3.4, 1.0]));
  var_dump(array_flip([5.5, 2.2, 3.4, 1.0, 4.0, 1.0, 5.5]));
  var_dump(array_flip([2 => 3.5, 7 => 2.0, 10 => 6.2, 1 => 1.0, 8 => 2.0, 4 => 6.2]));
  var_dump(array_flip(["foo" => 3.1, "bar" => 1.0, "baz" => 2.7, "gaz" => 3.1]));
  var_dump(array_flip(["foo" => 1.1, 99 => 2.0, "baz" => 3.0, "gaz" => 2.0]));
}

function test_array_flip_bools() {
  var_dump(array_flip([false, true]));
  var_dump(array_flip([false, true, false]));
  var_dump(array_flip([2 => false, 7 => true, 10 => false]));
  var_dump(array_flip(["foo" => false, "bar" => true, "baz" => false, "gaz" => true]));
  var_dump(array_flip(["foo" => false, 99 => true, "baz" => false, "gaz" => true]));
}

function test_array_flip_optional_ints() {
  var_dump(array_flip([false, 1, 2, false, null, 7]));
  var_dump(array_flip([false, 1, 2, false, null, 7, 1]));
  var_dump(array_flip([2 => false, 7 => 1, 10 => null, 11 => 1]));
  var_dump(array_flip(["foo" => false, "bar" => 2, "baz" => null, "gaz" => 2]));
  var_dump(array_flip(["foo" => false, 99 => 12, "baz" => null, "gaz" => 12]));
}

function test_array_flip_optional_strings() {
  var_dump(array_flip([false, "foo", "bar", false, null, "baz"]));
  var_dump(array_flip([false, "foo", "bar", false, null, "baz", "bar"]));
  var_dump(array_flip([2 => false, 7 => "foo", 10 => null, 11 => "foo"]));
  var_dump(array_flip(["xx" => false, "yy" => "foo", "zz" => null, "ww" => "foo"]));
  var_dump(array_flip(["xx" => false, 99 => "foo", "yy" => null, "zz" => "foo"]));
}

function test_array_flip_vars() {
  var_dump(array_flip([false, "foo", "bar", 7, 2.1, "baz", 7.0, "2", 2]));
  var_dump(array_flip(["xx" => false, 99 => "foo", "yy" => 9.2, "zz" => "12", "ww" => 12]));
}

test_array_flip_empty();
test_array_flip_ints();
test_array_flip_string();
test_array_flip_doubles();
test_array_flip_bools();
test_array_flip_optional_ints();
test_array_flip_optional_strings();
test_array_flip_vars();
