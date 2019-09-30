@ok
<?php

/**
 * @kphp-infer
 * @param int|false|null $val
 */
function test_unary_operations($val) {
  echo "+ "; var_dump(+$val);
  echo "- "; var_dump(-$val);
  echo "! "; var_dump(!$val);
  if ($val !== false && $val !== null) {
    echo "~ "; var_dump(~$val);
  }
  $x = $val;
  echo "++pre "; var_dump(++$x);
  $x = $val;
  echo "++post "; var_dump($x++);
  $x = $val;
  echo "--pre "; var_dump(--$x);
  $x = $val;
  echo "--post "; var_dump($x--);
}

/**
 * @kphp-infer
 * @param int|false|null $lhs
 * @param int|false|null $rhs
 */
function test_binary_operations($lhs, $rhs) {
  echo "binary + "; var_dump($lhs + $rhs);
  echo "binary - "; var_dump($lhs - $rhs);
  echo "* "; var_dump($lhs * $rhs);
  echo "** "; var_dump((float)($lhs ** $rhs));
  if ($rhs) {
    echo "/ "; var_dump((float)($lhs / $rhs));
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

  echo "?: "; var_dump($lhs ?: $rhs);

  echo ". "; var_dump($lhs . $rhs);
}

/**
 * @kphp-infer
 * @param int|false|null $lhs
 * @param int|false|null $rhs
 */
function test_assign_binary_operations($lhs, $rhs) {
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
    echo "/ "; var_dump((float)$res3);
    $res4 = $lhs;
    $res4 %= $rhs;
    echo "% "; var_dump($res4);
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


$optional_integers = [5, false, null, 0, -2];

foreach ($optional_integers as $optional_integer1) {
  test_unary_operations($optional_integer1);

  foreach ($optional_integers as $optional_integer2) {
    test_binary_operations($optional_integer1, $optional_integer2);
    test_assign_binary_operations($optional_integer1, $optional_integer2);
  }
}
