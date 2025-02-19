@ok
<?php

class FooException extends Exception {}
class BarException extends Exception {}

function throws_foo() { throw new FooException(); }

/**
 * All exceptions are caught by multi-catch
 * @kphp-should-not-throw
 */
function test1($cond) {
  if ($cond) {
    try {
      throws_foo();
      throw new BarException();
    } catch (BarException $e) {
    } catch (FooException $e2) {
    }
  }
}

/**
 * Throws nothing in a try block
 * @kphp-should-not-throw
 */
function test2() {
  try {
  } catch (Exception $e) {}
}

test1(false);
test2();
