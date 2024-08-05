@ok k2_skip
<?php

/**
 * @return int[]
 */
function get_arr() {
  echo "call get_arr\n";
  return [1, 2, 3, 4, 5];
}

/**
 * @return int
 */
function get_one() {
  echo "call get_one\n";
  return 1;
}

function test_array_exist() {
  $arr = ["hello", "world", "b", "c", "x", "y", "z", "key" => "value"];

  var_dump($arr[1] ?? "x");
  var_dump($arr[2] ?? "xxxxxx");
  var_dump($arr[3] ?? 2);
  var_dump($arr[4] ?? [12]);
  var_dump($arr["key"] ?? "xxxx");
  var_dump($arr["1"] ?? "x");

  var_dump($arr[5] ?? $arr[1] ?? $arr[8]);
}

function test_array_not_exist() {
  $arr = ["hello", "world", "b", "c", "x", "y", "z"];

  var_dump($arr[999] ?? "x");
  var_dump($arr[999] ?? "xxxxxx");
  var_dump($arr[999] ?? $arr[899] ?? "");
  var_dump($arr[999] ?? $arr[1]);
  var_dump($arr["key"] ?? "xxxx");
}

function test_array_strange_keys() {
  $arr = ["hello", "world", "b", "c", "x", "y", "z"];

  var_dump($arr[1.2] ?? "x");
  var_dump($arr[false] ?? "f");
  var_dump($arr[true] ?? "t");
  var_dump($arr[null ?? false] ?? "n");
}

function test_array_in_var() {
  $arr = 1 ? ["hello", "world", "b", 1, 2, [1, 2], null, "key" => "value"] : "x";

  var_dump($arr[1] ?? "x");
  var_dump($arr["key"] ?? "xxxxxx");
  var_dump($arr["key2"] ?? 2);
  var_dump($arr[4] ?? [12]);

  var_dump($arr[99] ?? $arr[6] ?? $arr["key"]);
}

function test_array_with_function_calls() {
  $arr = ["hello", "world", "b", "c", "x", "y", "z"];

  var_dump($arr[1] ?? get_arr());
  var_dump($arr[100] ?? get_arr());
  var_dump(get_arr()[100] ?? get_arr());
  var_dump(get_arr()[100] ?? $arr[1] ?? $arr[2] ?? "");
  var_dump($arr[100] && get_arr()[100] ?? $arr[1] ?? 0);
  var_dump($arr[100] && get_arr()[2] ?? $arr[1] ?? 8.5);

  var_dump($arr[get_one()] ?? get_arr());
  var_dump($arr[get_one()*100] ?? $arr[get_one() + get_one()]);
}

test_array_exist();
test_array_not_exist();
test_array_strange_keys();
test_array_in_var();
test_array_with_function_calls();
