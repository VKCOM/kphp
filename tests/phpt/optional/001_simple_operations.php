@ok
<?php

/**
 * @kphp-infer
 * @param string $hint
 * @param int[]|false|null $x
 */
function test_size_operations($hint, $x) {
  echo "$hint empty: "; var_dump(empty($x));
  echo "$hint count: "; var_dump(count($x));
  echo "$hint sizeof: "; var_dump(sizeof($x));
}

/**
 * @kphp-infer
 * @param string $hint
 * @param int|false|null $x
 */
function test_is_operations($hint, $x) {
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
 * @kphp-infer
 * @param string $hint
 * @param double|false|null $x
 */
function test_math_operations($hint, $x) {
  echo "$hint abs: "; var_dump(abs($x));
}

/**
 * @kphp-infer
 * @param string $hint
 * @param string|false|null $x
 */
function test_gettype_operation($hint, $x) {
  echo "$hint gettype: "; var_dump(gettype($x));
}

/**
 * @kphp-infer
 * @param string $hint
 * @param int|false|null $x
 */
function test_conv_operations($hint, $x) {
  echo "$hint in string: "; var_dump("<$x>");
  echo "$hint var_dump: "; var_dump($x);
  echo "$hint conv bool: "; var_dump((bool)$x);
  echo "$hint conv int: "; var_dump((int)$x);
  echo "$hint conv float: "; var_dump((float)$x);
  echo "$hint conv str: "; var_dump((string)$x);
  echo "$hint conv array: "; var_dump((array)$x);
}

test_size_operations("test_size_operations null", null);
test_size_operations("test_size_operations false", false);
test_size_operations("test_size_operations int[]", [1, 2, 3]);

test_is_operations("test_is_operations null", null);
test_is_operations("test_is_operations false", false);
test_is_operations("test_is_operations int", 1234);

test_math_operations("test_math_operations null", null);
test_math_operations("test_math_operations false", false);
test_math_operations("test_is_operations double", -2.1);

test_gettype_operation("test_gettype_operation null", null);
test_gettype_operation("test_gettype_operation false", false);
test_gettype_operation("test_gettype_operation string", "hello");

test_conv_operations("test_conv_operations null", null);
test_conv_operations("test_conv_operations false", false);
test_conv_operations("test_conv_operations int", 12);
