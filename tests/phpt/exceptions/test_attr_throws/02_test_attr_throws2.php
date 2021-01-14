@ok
<?php

interface CustomExceptionInterface {}
interface OtherExceptionInterface {}

class FooException extends Exception implements CustomExceptionInterface {}
class BarException extends Exception implements CustomExceptionInterface {}

/**
 * @kphp-throws BarException FooException
 */
function foo($v) {
  if ($v === 1) {
    try {
      throw new FooException();
    } catch (OtherExceptionInterface $e) {
    }
  }
  if ($v === 2) {
    throw new BarException();
  }
}

/**
 * @kphp-throws BarException FooException
 */
function test1($cond) {
  if ($cond) {
    foo(3);
  }
}

/**
 * Has a try that catches everything, but also throws from another place
 * @kphp-throws BarException FooException
 */
function test2($cond) {
  if ($cond) {
    foo(3);
  }
  try {
    foo(3);
  } catch (Throwable $e) {}
}

/**
 * Like test2, but throws from a different place
 * @kphp-throws BarException FooException
 */
function test3($cond) {
  try {
    foo(3);
  } catch (Throwable $e) {}
  if ($cond) {
    foo(3);
  }
}

/**
 * Both BarException and FooException implement CustomExceptionInterface
 * @kphp-should-not-throw
 */
function test4($cond) {
  try {
    foo(3);
  } catch (CustomExceptionInterface $e) {}
}

test1(false);
test2(false);
test3(false);
test4(false);
