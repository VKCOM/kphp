@ok k2_skip
<?php


function test_drop_or_null() {
  /** @var int|null */
  $x_or_null__int = 0 ? null : 123;
  /** @var int|null */
  $x_or_null__null = 1 ? null : 123;
  /** @var int|false */
  $x_or_false__int = 0 ? false : 123;
  /** @var int|false */
  $x_or_false__false = 1 ? false : 123;
  /** @var int|null|false */
  $x_or_null_false__int = 0 ? null : $x_or_false__int;
  /** @var int|null|false */
  $x_or_null_false__false = 0 ? null : $x_or_false__false;
  /** @var int|null|false */
  $x_or_null_false__null = 1 ? null : $x_or_false__int;

  /** @var int */
  $x1 = $x_or_null__int ?? 0;
  /** @var int */
  $x2 = $x_or_null__null ?? 0;
  /** @var int|false */
  $x3 = $x_or_false__int ?? 0;
  /** @var int|false */
  $x4 = $x_or_false__false ?? 0;
  /** @var int|false */
  $x5 = $x_or_null_false__int ?? 0;
  /** @var int|false */
  $x6 = $x_or_null_false__false ?? 0;
  /** @var int|false */
  $x7 = $x_or_null_false__null ?? 0;

  var_dump($x_or_null__int);
  var_dump($x_or_null__null);
  var_dump($x_or_false__int);
  var_dump($x_or_false__false);
  var_dump($x_or_null_false__int);
  var_dump($x_or_null_false__false);
  var_dump($x_or_null_false__null);

  var_dump($x1);
  var_dump($x2);
  var_dump($x3);
  var_dump($x4);
  var_dump($x5);
  var_dump($x6);
  var_dump($x7);
}

function test_constants() {
  /** @var int[] */
  $x1 = null ?? [1];
  /** @var int[]|false */
  $x2 = null ?? [1] ?? false;
  /** @var false */
  $x3 = null ?? null ?? false;
  /** @var false|null */
  $x4 = false ?? null;
  /** @var int[]|null */
  $x5 = [1] ?? null;
  /** @var int[]|false */
  $x6 = [1] ?? null ?? false;
  /** @var int[]|false|null */
  $x7 = [1] ?? false ?? null;

  var_dump($x1);
  var_dump($x2);
  var_dump($x3);
  var_dump($x4);
  var_dump($x5);
  var_dump($x6);
  var_dump($x7);
}

function test_string_type() {
  $var_str = 0 ? [1] : "var string";
  $pure_str = "pure string";

  /** @var mixed */
  $var_x = $var_str[1] ?? "x";
  /** @var string */
  $pure_x = $pure_str[1] ?? "x";

  var_dump($var_x);
  var_dump($pure_x);
}

function test_array_type() {
  $var_arr = 0 ? "str" : ["var", "array"];
  $pure_arr = ["pure", "string"];

  /** @var mixed */
  $var_x = $var_arr[1] ?? "x";
  /** @var string */
  $pure_x = $pure_arr[1] ?? "xyz";

  var_dump($var_x);
  var_dump($pure_x);
}

test_drop_or_null();
test_constants();
test_string_type();
test_array_type();
