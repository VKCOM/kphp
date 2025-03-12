@ok
<?php

class MyError extends Error {}

/**
 * @kphp-should-not-throw
 */
function test1() {
  try {
    throw new MyError();
  } catch (MyError $e) {}
}

/**
 * @kphp-should-not-throw
 */
function test2() {
  try {
    throw new MyError();
  } catch (Error $e) {}
}

/**
 * @kphp-throws MyError
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

