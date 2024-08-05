@ok k2_skip
<?php

/**
 * @return string[]
 */
function foobar() {
  echo "foobar called\n";
  return ['a'];
}

function get_null() {
  echo "get_null()\n";
  return null;
}

function test_variable() {
  /** @var int */
  $v0 = 7;
  /** @var null|int */
  $v1 = 1 ? null : $v0;
  /** @var mixed */
  $v3 = 0 ? $v1 : "xxxx";
  /** @var string|int */
  $v4 = "";

  $v0 ??= 2;
  var_dump($v0);
  $v1 ??= 3;
  var_dump($v1);
  $v1 ??= $v0;
  var_dump($v1);
  $v3 ??= $v1;
  var_dump($v3);
  $v4 ??= $v1;
  var_dump($v4);
  $v3 ??= foobar();
  var_dump($v3);
}

function test_int() : int {
  $x = 0;
  $x ??= 15;
  var_dump($x);
  return $x;
}

function test_null() : int {
  $x = null;
  $x ??= 15; // $x is never null now, var split should make it int-typed
  var_dump($x);
  return $x;
}

class Row {
  /** @var null|int[] */
  public $values = null;

  /** @var ?int */
  public static $static_int = 0;
  /** @var ?string */
  public static $static_string = null;
}

function test_objects_with_arrays() {
  $row = new Row();

  var_dump($row->values === null);

  $row->values ??= [];
  var_dump($row->values === []);

  $row->values ??= [1, 2];
  var_dump($row->values === []);

  $row->values['foo'] ??= 9000;
  var_dump($row->values['foo'] == 9000);

  $row->values['foo'] ??= 0;
  var_dump($row->values['foo'] != 0);
}

function test_static() {
  Row::$static_int ??= 123;
  var_dump(Row::$static_int);
  Row::$static_int = null;
  Row::$static_int ??= 123;
  var_dump(Row::$static_int);

  Row::$static_string ??= get_null();
  Row::$static_string ??= get_null();
  Row::$static_string ??= "foo";
  Row::$static_string ??= get_null();
  var_dump(Row::$static_string);
}

test_int();
test_null();
test_variable();
test_objects_with_arrays();
test_static();
