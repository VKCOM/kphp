@ok
<?php

require_once 'kphp_tester_include.php';

/**
 * @param int|null $x
 * @return int|null
 */
function foo($x) {
  echo "call foo $x\n";
  if ($x) {
    throw new Exception("foo exception $x");
  }
  return $x ? $x : null;
}

/**
 * @param int|null $x
 * @return int|null
 */
function resumable_throwing($x) {
  sched_yield_sleep(0.01);
  echo "call resumable_throwing $x";
  if ($x) {
    throw new Exception("resumable_throwing exception $x");
  }
  return $x;
}

function test_exception_lhs_try_catch_wrap() {
  try {
    $x = foo(1) ?? 0;
    var_dump($x);
  } catch (Exception $e) {
    echo __LINE__ . " Exception: " . $e->getMessage() . "\n";
  }

  try {
    $x = (foo(1) + 2) ?? 0;
    var_dump($x);
  } catch (Exception $e) {
    echo __LINE__ . " Exception: " . $e->getMessage() . "\n";
  }
}

function test_exception_lhs_no_wrap() {
  /**
   * @return string
   */
  function test_1() {
    $x = foo(1) ?? 0;
    var_dump($x);
    return "ok";
  }

  try {
    $r = test_1();
    var_dump($r);
  } catch (Exception $e) {
    echo __LINE__ . " Exception: " . $e->getMessage() . "\n";
  }


  /**
   * @return string
   */
  function test_2() {
    $x = (foo(1) + 2) ?? 0;
    var_dump($x);
    return "ok";
  }

  try {
    $r = test_2();
    var_dump($r);
  } catch (Exception $e) {
    echo __LINE__ . " Exception: " . $e->getMessage() . "\n";
  }
}

function test_exception_rhs_try_catch_wrap() {
  $y = null;
  try {
    $x = $y ?? foo(1);
    var_dump($x);
  } catch (Exception $e) {
    echo __LINE__ . " Exception: " . $e->getMessage() . "\n";
  }

  try {
    $x = $y ?? foo(0) ?? foo(null) ?? foo(1) ?? foo(0) ?? foo(2);
    var_dump($x);
  } catch (Exception $e) {
    echo __LINE__ . " Exception: " . $e->getMessage() . "\n";
  }

  try {
    $x = $y ?? (5 + foo(1));
    var_dump($x);
  } catch (Exception $e) {
    echo __LINE__ . " Exception: " . $e->getMessage() . "\n";
  }
}

function test_exception_rhs_try_no_wrap() {
  /**
   * @return string
   */
  function test_3() {
    $y = null;
    $x = $y ?? foo(1);
    var_dump($x);
    return "ok";
  }

  try {
    $r = test_3();
    var_dump($r);
  } catch (Exception $e) {
    echo __LINE__ . " Exception: " . $e->getMessage() . "\n";
  }


  /**
   * @return string
   */
  function test_4() {
    $y = null;
    $x = $y ?? foo(0) ?? foo(null) ?? foo(1) ?? foo(0) ?? foo(2);
    var_dump($x);
    return "ok";
  }

  try {
    $r = test_4();
    var_dump($r);
  } catch (Exception $e) {
    echo __LINE__ . " Exception: " . $e->getMessage() . "\n";
  }


  /**
   * @return string
   */
  function test_5() {
    $y = null;
    $x = $y ?? foo(0) ?? foo(null) ?? foo(1) ?? foo(0) ?? foo(2);
    var_dump($x);
    return "ok";
  }

  try {
    $r = test_5();
    var_dump($r);
  } catch (Exception $e) {
    echo __LINE__ . " Exception: " . $e->getMessage() . "\n";
  }
}

function test_exception_rhs_resumable_throwing() {
  try {
    $r = foo(null) ?? resumable_throwing(7);
  } catch (Exception $e) {
    echo __LINE__ . " Exception: " . $e->getMessage() . "\n";
  }

  $r = foo(null) ?? resumable_throwing(null);
  echo "no wrap resumable_throwing result: $r";
}

test_exception_lhs_try_catch_wrap();
test_exception_lhs_no_wrap();

test_exception_rhs_try_catch_wrap();
test_exception_rhs_try_no_wrap();

test_exception_rhs_resumable_throwing();
