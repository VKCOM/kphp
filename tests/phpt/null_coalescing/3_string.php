@ok k2_skip
<?php

/**
 * @return string
 */
function get_str() {
  echo "call get_str\n";
  return "some string";
}

/**
 * @return int
 */
function get_one() {
  echo "call get_one\n";
  return 1;
}

function test_string_exist() {
  $str = "hello world";

  var_dump($str[1] ?? "x");
  var_dump($str[2] ?? "xxxxxx");
  var_dump($str[3] ?? 2);
  var_dump($str[4] ?? [12]);

  var_dump($str[5] ?? $str[1] ?? $str[8]);
}

function test_string_not_exist() {
  $str = "hello world";

  var_dump($str[999] ?? "x");
  var_dump($str[999] ?? "xxxxxx");
  var_dump($str[999] ?? $str[899]);
  var_dump($str[999] ?? $str[1]);
}

function test_string_strange_keys() {
  $str = "hello world";

  var_dump($str[false] ?? "f");
  var_dump($str[true] ?? "t");
  var_dump($str[null ?? false] ?? "n");
}

function test_string_in_var() {
  $str = 1 ? "hello world" : [1];

  var_dump($str[1] ?? "x");
  var_dump($str[3] ?? "xxxxxx");
  var_dump($str[999] ?? $str[1]);
  var_dump($str[1] ?? $str[199]);
}

function test_string_with_function_calls() {
  $str = "hello world";

  var_dump($str[1] ?? get_str());
  var_dump($str[100] ?? get_str());
  var_dump(get_str()[100] ?? get_str());
  var_dump(get_str()[100] ?? $str[1] ?? $str[2] ?? null);
  var_dump($str[100] && get_str()[100] ?? $str[1] ?? null);
  var_dump($str[100] && get_str()[2] ?? $str[1] ?? null);

  var_dump($str[get_one()] ?? get_str());
  var_dump($str[get_one()*100] ?? $str[get_one() + get_one()]);
}

test_string_exist();
test_string_not_exist();
test_string_strange_keys();
test_string_in_var();
test_string_with_function_calls();
