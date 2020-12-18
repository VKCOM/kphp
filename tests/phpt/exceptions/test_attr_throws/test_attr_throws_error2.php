@ok
<?php

class MyError extends Error {}

/**
 * @kphp-test-throws
 */
function test1() {
  try {
    throw new MyError();
  } catch (MyError $e) {}
}

/**
 * @kphp-test-throws
 */
function test2() {
  try {
    throw new MyError();
  } catch (Error $e) {}
}

/**
 * @kphp-test-throws MyError
 */
function test3() {
  try {
    throw new MyError();
  } catch (Exception $e) {}
}

test1();
test2();
try {
  test3();
} catch (Throwable $e) {
  var_dump([__LINE__ => $e->getLine()]);
}

