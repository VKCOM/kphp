@ok
<?php

class FooException extends Exception {}
class BarException extends Exception {}

/**
 * @kphp-throws FooException
 */
function throws_foo() {
  throw new FooException();
}

/**
 * Nothing is caught, all exceptions bubble up
 * @kphp-throws FooException BarException
 */
function test1($cond) {
  if ($cond) {
    throws_foo();
    throw new BarException();
    throws_foo();
  }
}

/**
 * All exceptions are caught via \Exception
 * @kphp-should-not-throw
 */
function test2($cond) {
  if ($cond) {
    try {
      throws_foo();
      throw new BarException();
      throws_foo();
    } catch (Exception $e) {
    }
  }
}

/**
 * Only BarException is caught
 * @kphp-throws FooException
 */
function test3($cond) {
  if ($cond) {
    try {
      throws_foo();
      throw new BarException();
      throws_foo();
    } catch (BarException $e) {
    }
  }
}

/**
 * Only FooException is caught
 * @kphp-throws BarException
 */
function test4($cond) {
  if ($cond) {
    try {
      throws_foo();
      throw new BarException();
      throws_foo();
    } catch (FooException $e) {
    }
  }
}

/**
 * All exceptions are caught, 2 separate try blocks
 * @kphp-should-not-throw
 */
function test5($cond) {
  try {
    if ($cond) {
      try {
        throws_foo();
        throw new BarException();
        throws_foo();
      } catch (BarException $e) {
      }
    }
  } catch (FooException $e2) {
  }
}

/**
 * Calls test5 that handles all exceptions
 * @kphp-should-not-throw
 */
function test6($cond) {
  test5($cond);
}

/**
 * All exceptions are caught via \Throwable
 * @kphp-should-not-throw
 */
function test7($cond) {
  if ($cond) {
    try {
      throws_foo();
      throw new BarException();
      throws_foo();
    } catch (Throwable $e) {
    }
  }
}

test1(false);
test2(false);
test3(false);
test4(false);
test5(false);
test6(false);
test7(false);
