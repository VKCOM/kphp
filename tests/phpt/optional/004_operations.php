@ok
<?php

/**
 * @param int|false|null $val
 */
function test_unary_operations_optional_int($val) {
  echo "+ "; var_dump(+$val);
  echo "- "; var_dump(-$val);
  echo "! "; var_dump(!$val);
  if ($val !== false && !is_null($val)) {
    echo "~ "; var_dump(~$val);

    // TODO KPHP-530
    $x = $val;
    echo "++pre "; var_dump(++$x);
    $x = $val;
    echo "++post "; var_dump($x++);
    $x = $val;
    echo "--pre "; var_dump(--$x);
    $x = $val;
    echo "--post "; var_dump($x--);
  }
}

/**
 * @param null $val
 */
function test_unary_operations_null($val) {
  echo "+ "; var_dump(+$val);
  echo "- "; var_dump(-$val);
  echo "! "; var_dump(!$val);
}

/**
 * @param null|false $val
 */
function test_unary_operations_null_false($val) {
  echo "+ "; var_dump(+$val);
  echo "- "; var_dump(-$val);
  echo "! "; var_dump(!$val);
}

/**
 * @param int|false|null $lhs
 * @param int|false|null $rhs
 */
function test_binary_operations_optional_int($lhs, $rhs) {
  echo "binary + "; var_dump($lhs + $rhs);
  echo "binary - "; var_dump($lhs - $rhs);
  echo "* "; var_dump($lhs * $rhs);
  echo "** "; var_dump((float)($lhs ** $rhs));
  if ($rhs) {
    echo "/ "; var_dump((float)abs($lhs / $rhs));
    echo "% "; var_dump($lhs % $rhs);
  }

  echo "& "; var_dump($lhs & $rhs);
  echo "| "; var_dump($lhs | $rhs);
  echo "^ "; var_dump($lhs ^ $rhs);
  if (!is_int($rhs) || $rhs >= 0) {
    echo "<< "; var_dump($lhs << $rhs);
    echo ">> "; var_dump($lhs >> $rhs);
  }

  echo "&& "; var_dump($lhs && $rhs);
  echo "|| "; var_dump($lhs || $rhs);

  echo "< "; var_dump($lhs < $rhs);
  echo "<= "; var_dump($lhs <= $rhs);
  echo "> "; var_dump($lhs > $rhs);
  echo ">= "; var_dump($lhs >= $rhs);
  echo "<> "; var_dump($lhs <> $rhs);
  echo "<=> "; var_dump($lhs <=> $rhs);
  echo "== "; var_dump($lhs == $rhs);
  echo "!= "; var_dump($lhs != $rhs);

  echo "=== "; var_dump($lhs === $rhs);
  echo "!== "; var_dump($lhs !== $rhs);

  echo ". "; var_dump($lhs . $rhs);
}

/**
 * @param null|false $lhs
 * @param false|null $rhs
 */
function test_binary_operations_null_false($lhs, $rhs) {
  echo "binary + "; var_dump($lhs + $rhs);
  echo "binary - "; var_dump($lhs - $rhs);
  echo "* "; var_dump($lhs * $rhs);
  echo "** "; var_dump((float)($lhs ** $rhs));

  echo "& "; var_dump($lhs & $rhs);
  echo "| "; var_dump($lhs | $rhs);
  echo "^ "; var_dump($lhs ^ $rhs);
  if (!is_int($rhs) || $rhs >= 0) {
    echo "<< "; var_dump($lhs << $rhs);
    echo ">> "; var_dump($lhs >> $rhs);
  }

  echo "&& "; var_dump($lhs && $rhs);
  echo "|| "; var_dump($lhs || $rhs);

  echo "< "; var_dump($lhs < $rhs);
  echo "<= "; var_dump($lhs <= $rhs);
  echo "> "; var_dump($lhs > $rhs);
  echo ">= "; var_dump($lhs >= $rhs);
  echo "<> "; var_dump($lhs <> $rhs);
  echo "<=> "; var_dump($lhs <=> $rhs);
  echo "== "; var_dump($lhs == $rhs);
  echo "!= "; var_dump($lhs != $rhs);

  echo "=== "; var_dump($lhs === $rhs);
  echo "!== "; var_dump($lhs !== $rhs);

  echo ". "; var_dump($lhs . $rhs);
}

/**
 * @param int|false|null $lhs
 * @param int|false|null $rhs
 */
function test_assign_binary_operations_optional_int($lhs, $rhs) {
  $res1 = $lhs;

  $res1 += $rhs;
  echo "+= "; var_dump($res1);
  $res1 -= $rhs;
  echo "-= "; var_dump($res1);
  $res1 *= $rhs;
  echo "*= "; var_dump($res1);
  $res2 = $lhs;
  $res2 **= $rhs;
  echo "**= "; var_dump((float)$res2);
  if ($rhs) {
    $res3 = $lhs;
    $res3 /= $rhs;
    echo "/= "; var_dump((float)abs($res3));
    $res4 = $lhs;
    $res4 %= $rhs;
    echo "%= "; var_dump($res4);
  }

  $res1 = $lhs;
  $res1 &= $rhs;
  echo "&= "; var_dump($res1);
  $res1 |= $rhs;
  echo "| "; var_dump($res1);
  $res1 ^= $rhs;
  echo "^ "; var_dump($res1);

  if (!is_int($rhs) || $rhs >= 0) {
    $res1 <<= $rhs;
    echo "<<= "; var_dump($res1);
    $res1 >>= $rhs;
    echo ">>= "; var_dump($res1);
  }

  $res5 = $lhs;
  $res5 .= $rhs;
  echo ".= "; var_dump($res5);
}

/**
 * @param null|false $lhs
 * @param false|null $rhs
 */
function test_assign_binary_operations_null_false($lhs, $rhs) {
  $res1 = $lhs;

  $res1 += $rhs;
  echo "+= "; var_dump((int)$res1);
  $res1 -= $rhs;
  echo "-= "; var_dump((int)$res1);
  $res1 *= $rhs;
  echo "*= "; var_dump((int)$res1);
  $res2 = $lhs;
  $res2 **= $rhs;
  echo "**= "; var_dump((float)$res2);

  $res1 = $lhs;
  $res1 &= $rhs;
  echo "&= "; var_dump($res1);
  $res1 |= $rhs;
  echo "| "; var_dump($res1);
  $res1 ^= $rhs;
  echo "^ "; var_dump($res1);

  if (!is_int($rhs) || $rhs >= 0) {
    $res1 <<= $rhs;
    echo "<<= "; var_dump($res1);
    $res1 >>= $rhs;
    echo ">>= "; var_dump($res1);
  }

  $res5 = $lhs;
  $res5 .= $rhs;
  echo ".= "; var_dump($res5);
}

/**
 * @param null|false|int $lhs
 * @param mixed $rhs
 */
function test_cmp_operation_optional_int($lhs, $rhs) {
  var_dump($lhs > $rhs);
  var_dump($lhs >= $rhs);
  var_dump($lhs < $rhs);
  var_dump($lhs <= $rhs);
  var_dump($lhs == $rhs);
  var_dump($lhs != $rhs);
  var_dump($lhs === $rhs);
  var_dump($lhs !== $rhs);
}

/**
 * @param null|false $lhs
 * @param mixed $rhs
 */
function test_cmp_operation_null_false($lhs, $rhs) {
  var_dump($lhs > $rhs);
  var_dump($lhs >= $rhs);
  var_dump($lhs < $rhs);
  var_dump($lhs <= $rhs);
  var_dump($lhs == $rhs);
  var_dump($lhs != $rhs);
  var_dump($lhs === $rhs);
  var_dump($lhs !== $rhs);
}

class A {
  public $x = 1;
}

/**
 * @param A $lhs
 * @param null|false $rhs
 */
function test_cmp_operation_class_instance_null_false($lhs, $rhs) {
  var_dump((bool)$lhs == $rhs);
  var_dump((bool)$lhs != $rhs);
}

test_unary_operations_null(null);
test_unary_operations_null_false(null);
test_unary_operations_null_false(false);

test_binary_operations_null_false(null, null);
test_binary_operations_null_false(false, null);
test_binary_operations_null_false(null, false);
test_binary_operations_null_false(false, false);

test_assign_binary_operations_null_false(null, null);
test_assign_binary_operations_null_false(false, null);
test_assign_binary_operations_null_false(null, false);
test_assign_binary_operations_null_false(false, false);

test_cmp_operation_class_instance_null_false(new A, null);
test_cmp_operation_class_instance_null_false(new A, false);
test_cmp_operation_class_instance_null_false(null, false);
test_cmp_operation_class_instance_null_false(null, null);

$optional_integers = [5, false, null, 0, -2];
foreach ($optional_integers as $optional_integer1) {
  test_unary_operations_optional_int($optional_integer1);

  foreach ($optional_integers as $optional_integer2) {
    test_binary_operations_optional_int($optional_integer1, $optional_integer2);
    test_assign_binary_operations_optional_int($optional_integer1, $optional_integer2);
  }
}

$mixed_values = [5, false, null, 0, -2, 1, "", 0.0, 1.0, "0", "1", [], [false], [""]];
foreach ($mixed_values as $mixed_value) {
  foreach ($optional_integers as $optional_integer) {
    test_cmp_operation_optional_int($optional_integer, $mixed_value);
  }

  test_cmp_operation_null_false(false, $mixed_value);
  test_cmp_operation_null_false(null, $mixed_value);
}

foreach ($optional_integers as $optional_integer) {
  test_cmp_operation_optional_int($optional_integer, $mixed_values);
}
test_cmp_operation_null_false(null, $mixed_values);
test_cmp_operation_null_false(false, $mixed_values);
