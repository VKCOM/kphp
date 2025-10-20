@ok
<?php

/**
 * @param string $hint
 * @param int[]|false|null $x
 */
function test_size_operations_optional_array($hint, $x) {
  echo "$hint empty: "; var_dump(empty($x));
  echo "$hint count: "; var_dump(count($x));
  echo "$hint sizeof: "; var_dump(sizeof($x));
}

/**
 * @param string $hint
 * @param boolean|null $x
 */
function test_size_operations_optional_bool($hint, $x) {
  echo "$hint empty: "; var_dump(empty($x));
  echo "$hint count: "; var_dump(count($x));
  echo "$hint sizeof: "; var_dump(sizeof($x));
}

/**
 * @param string $hint
 * @param false|null $x
 */
function test_size_operations_optional_false($hint, $x) {
  echo "$hint empty: "; var_dump(empty($x));
  echo "$hint count: "; var_dump(count($x));
  echo "$hint sizeof: "; var_dump(sizeof($x));
}

/**
 * @param string $hint
 * @param int|false|null $x
 */
function test_is_operations_optional_int($hint, $x) {
  echo "$hint is_scalar: "; var_dump(is_scalar($x));
  echo "$hint is_numeric: "; var_dump(is_numeric($x));
  echo "$hint is_null: "; var_dump(is_null($x));
  echo "$hint is_bool: "; var_dump(is_bool($x));
  echo "$hint is_int: "; var_dump(is_int($x));
  echo "$hint is_integer: "; var_dump(is_integer($x));
  echo "$hint is_long: "; var_dump(is_long($x));
  echo "$hint is_finite: "; var_dump(is_finite($x));
  echo "$hint is_infinite: "; var_dump(is_infinite($x));
  echo "$hint is_nan: "; var_dump(is_nan($x));
  echo "$hint is_float: "; var_dump(is_float($x));
  echo "$hint is_double: "; var_dump(is_double($x));
  echo "$hint is_real: "; var_dump(is_real($x));
  echo "$hint is_string: "; var_dump(is_string($x));
  echo "$hint is_array: "; var_dump(is_array($x));
  echo "$hint is_object: "; var_dump(is_object($x));
}

/**
 * @param string $hint
 * @param int[]|false|null $x
 */
function test_is_operations_optional_array_int($hint, $x) {
  echo "$hint is_scalar: "; var_dump(is_scalar($x));
  echo "$hint is_numeric: "; var_dump(is_numeric($x));
  echo "$hint is_null: "; var_dump(is_null($x));
  echo "$hint is_bool: "; var_dump(is_bool($x));
  echo "$hint is_int: "; var_dump(is_int($x));
  echo "$hint is_integer: "; var_dump(is_integer($x));
  echo "$hint is_long: "; var_dump(is_long($x));
  echo "$hint is_float: "; var_dump(is_float($x));
  echo "$hint is_double: "; var_dump(is_double($x));
  echo "$hint is_real: "; var_dump(is_real($x));
  echo "$hint is_string: "; var_dump(is_string($x));
  echo "$hint is_array: "; var_dump(is_array($x));
  echo "$hint is_object: "; var_dump(is_object($x));
}

/**
 * @param string $hint
 * @param boolean|null $x
 */
function test_is_operations_optional_bool($hint, $x) {
  echo "$hint is_scalar: "; var_dump(is_scalar($x));
  echo "$hint is_numeric: "; var_dump(is_numeric($x));
  echo "$hint is_null: "; var_dump(is_null($x));
  echo "$hint is_bool: "; var_dump(is_bool($x));
  echo "$hint is_int: "; var_dump(is_int($x));
  echo "$hint is_integer: "; var_dump(is_integer($x));
  echo "$hint is_long: "; var_dump(is_long($x));
  echo "$hint is_finite: "; var_dump(is_finite($x));
  echo "$hint is_infinite: "; var_dump(is_infinite($x));
  echo "$hint is_nan: "; var_dump(is_nan($x));
  echo "$hint is_float: "; var_dump(is_float($x));
  echo "$hint is_double: "; var_dump(is_double($x));
  echo "$hint is_real: "; var_dump(is_real($x));
  echo "$hint is_string: "; var_dump(is_string($x));
  echo "$hint is_array: "; var_dump(is_array($x));
  echo "$hint is_object: "; var_dump(is_object($x));
}

/**
 * @param string $hint
 * @param false|null $x
 */
function test_is_operations_optional_false($hint, $x) {
  echo "$hint is_scalar: "; var_dump(is_scalar($x));
  echo "$hint is_numeric: "; var_dump(is_numeric($x));
  echo "$hint is_null: "; var_dump(is_null($x));
  echo "$hint is_bool: "; var_dump(is_bool($x));
  echo "$hint is_int: "; var_dump(is_int($x));
  echo "$hint is_integer: "; var_dump(is_integer($x));
  echo "$hint is_long: "; var_dump(is_long($x));
  echo "$hint is_finite: "; var_dump(is_finite($x));
  echo "$hint is_infinite: "; var_dump(is_infinite($x));
  echo "$hint is_nan: "; var_dump(is_nan($x));
  echo "$hint is_float: "; var_dump(is_float($x));
  echo "$hint is_double: "; var_dump(is_double($x));
  echo "$hint is_real: "; var_dump(is_real($x));
  echo "$hint is_string: "; var_dump(is_string($x));
  echo "$hint is_array: "; var_dump(is_array($x));
  echo "$hint is_object: "; var_dump(is_object($x));
}

/**
 * @param string $hint
 * @param int|false|null $x
 */
function test_math_operations_optional_int($hint, $x) {
  echo "$hint abs: "; var_dump(abs($x));
}

/**
 * @param string $hint
 * @param boolean|null $x
 */
function test_math_operations_optional_bool($hint, $x) {
  echo "$hint abs: "; var_dump(abs($x));
}

/**
 * @param string $hint
 * @param false|null $x
 */
function test_math_operations_optional_false($hint, $x) {
  echo "$hint abs: "; var_dump(abs($x));
}

/**
 * @param string $hint
 * @param string|false|null $x
 */
function test_gettype_operation_optional_string($hint, $x) {
  echo "$hint gettype: "; var_dump(gettype($x));
}

/**
 * @param string $hint
 * @param boolean|null $x
 */
function test_gettype_operation_optional_bool($hint, $x) {
  echo "$hint gettype: "; var_dump(gettype($x));
}

/**
 * @param string $hint
 * @param false|null $x
 */
function test_gettype_operation_optional_false($hint, $x) {
  echo "$hint gettype: "; var_dump(gettype($x));
}

/**
 * @param string $hint
 * @param int|false|null $x
 */
function test_conv_operations_optional_int($hint, $x) {
  echo "$hint in string: "; var_dump("<$x>");
  echo "$hint var_dump: "; var_dump($x);
  echo "$hint conv bool: "; var_dump((bool)$x);
  echo "$hint conv int: "; var_dump((int)$x);
  echo "$hint conv float: "; var_dump((float)$x);
  echo "$hint conv str: "; var_dump((string)$x);
  // TODO KPHP-530
  if ($x) {
    echo "$hint conv array: "; var_dump((array)$x);
  }
}

/**
 * @param string $hint
 * @param boolean|null $x
 */
function test_conv_operations_optional_bool($hint, $x) {
  echo "$hint in string: "; var_dump("<$x>");
  echo "$hint var_dump: "; var_dump($x);
  echo "$hint conv bool: "; var_dump((bool)$x);
  echo "$hint conv int: "; var_dump((int)$x);
  echo "$hint conv float: "; var_dump((float)$x);
  echo "$hint conv str: "; var_dump((string)$x);
  // TODO KPHP-530
  if ($x) {
    echo "$hint conv array: "; var_dump((array)$x);
  }
}

/**
 * @param string $hint
 * @param false|null $x
 */
function test_conv_operations_optional_false($hint, $x) {
  echo "$hint in string: "; var_dump("<$x>");
  echo "$hint var_dump: "; var_dump($x);
  echo "$hint conv bool: "; var_dump((bool)$x);
  echo "$hint conv int: "; var_dump((int)$x);
  echo "$hint conv float: "; var_dump((float)$x);
  echo "$hint conv str: "; var_dump((string)$x);
  // TODO KPHP-530
  //  echo "$hint conv array: "; var_dump((array)$x);
}

test_size_operations_optional_array("test_size_operations_optional_array null", null);
test_size_operations_optional_array("test_size_operations_optional_array false", false);
test_size_operations_optional_array("test_size_operations_optional_array int[]", [1, 2, 3]);

test_size_operations_optional_bool("test_size_operations_optional_bool null", null);
test_size_operations_optional_bool("test_size_operations_optional_bool false", false);
test_size_operations_optional_bool("test_size_operations_optional_bool true", true);

test_size_operations_optional_false("test_size_operations_optional_false null", null);
test_size_operations_optional_false("test_size_operations_optional_false false", false);


test_is_operations_optional_int("test_is_operations_optional_int null", null);
test_is_operations_optional_int("test_is_operations_optional_int false", false);
test_is_operations_optional_int("test_is_operations_optional_int int", 1234);

test_is_operations_optional_array_int("test_is_operations_optional_array_int array", null);
test_is_operations_optional_array_int("test_is_operations_optional_array_int array", false);
test_is_operations_optional_array_int("test_is_operations_optional_array_int array", [1234, 456]);

test_is_operations_optional_bool("test_is_operations_optional_bool null", null);
test_is_operations_optional_bool("test_is_operations_optional_bool false", false);
test_is_operations_optional_bool("test_is_operations_optional_bool true", true);

test_is_operations_optional_false("test_is_operations_optional_false null", null);
test_is_operations_optional_false("test_is_operations_optional_false false", false);


test_math_operations_optional_int("test_math_operations_optional_int null", null);
test_math_operations_optional_int("test_math_operations_optional_int false", false);
test_math_operations_optional_int("test_math_operations_optional_int int", -2);

test_math_operations_optional_bool("test_math_operations_optional_bool null", null);
test_math_operations_optional_bool("test_math_operations_optional_bool false", false);
test_math_operations_optional_bool("test_math_operations_optional_bool true", true);

test_math_operations_optional_false("test_math_operations_optional_false null", null);
test_math_operations_optional_false("test_math_operations_optional_false false", false);


test_gettype_operation_optional_string("test_gettype_operation_optional_string null", null);
test_gettype_operation_optional_string("test_gettype_operation_optional_string false", false);
test_gettype_operation_optional_string("test_gettype_operation_optional_string string", "hello");

test_gettype_operation_optional_bool("test_gettype_operation_optional_bool null", null);
test_gettype_operation_optional_bool("test_gettype_operation_optional_bool false", false);
test_gettype_operation_optional_bool("test_gettype_operation_optional_bool true", true);

test_gettype_operation_optional_false("test_gettype_operation_optional_false null", null);
test_gettype_operation_optional_false("test_gettype_operation_optional_false false", false);


test_conv_operations_optional_int("test_conv_operations_optional_int null", null);
test_conv_operations_optional_int("test_conv_operations_optional_int false", false);
test_conv_operations_optional_int("test_conv_operations_optional_int int", 12);

test_conv_operations_optional_bool("test_conv_operations_optional_bool null", null);
test_conv_operations_optional_bool("test_conv_operations_optional_bool false", false);
test_conv_operations_optional_bool("test_conv_operations_optional_bool true", true);

test_conv_operations_optional_false("test_conv_operations_optional_false null", null);
test_conv_operations_optional_false("test_conv_operations_optional_false false", false);
