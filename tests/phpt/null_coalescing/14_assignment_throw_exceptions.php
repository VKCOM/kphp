@ok k2_skip
<?php

/**
 * @param int|null $v
 * @param bool $throw
 * @return int|null
 */
function foo($v, $throw) {
  echo "call foo($v, $throw)\n";
  if ($throw) {
    throw new Exception("foo exception $v");
  }
  return $v;
}

function test_exception_rhs_try_catch_wrap() {
  /** @var ?int */
  $x = null;
  try {
    $x ??= foo(1, true);
    echo "unreachable\n";
  } catch (Exception $e) {
    echo __LINE__ . " Exception: " . $e->getMessage() . "\n";
  }
  var_dump($x);

  try {
    $x ??= foo(1, false);
    $x ??= foo(null, true); // not evaluated: shouldn't throw
    var_dump($x);
  } catch (Exception $e) {
    echo __LINE__ . " Exception: " . $e->getMessage() . "\n";
  }
}

function test_exception_rhs_no_wrap() {
  /**
   * @return string
   */
  function test_1() {
    $x = null;
    $x ??= foo(1, true);
    var_dump($x);
    return "ok";
  }

  try {
    $r = test_1();
    var_dump($r);
  } catch (Exception $e) {
    echo __LINE__ . " Exception: " . $e->getMessage() . "\n";
  }
}

test_exception_rhs_try_catch_wrap();
test_exception_rhs_no_wrap();
