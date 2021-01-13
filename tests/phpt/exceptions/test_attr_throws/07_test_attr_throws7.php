@ok
<?php

/**
 * @kphp-should-not-throw
 */
function test1($cond) {
  try {
    if ($cond) {
      throw new BadFunctionCallException();
    } else {
      throw new BadMethodCallException();
    }
  } catch (BadFunctionCallException $e) {
    var_dump($e->getLine());
  }
}

/**
 * @kphp-throws BadFunctionCallException
 */
function test2($cond) {
  try {
    if ($cond) {
      throw new BadFunctionCallException();
    } else {
      throw new BadMethodCallException();
    }
  } catch (BadMethodCallException $e) {
    var_dump($e->getLine());
  }
}

/**
 * @kphp-throws BadFunctionCallException Exception
 */
function test3($type) {
  try {
    switch ($type) {
    case 'BadFunctionCallException':
      throw new BadFunctionCallException();
    case 'BadMethodCallException':
      throw new BadMethodCallException();
    default:
      throw new Exception();
    }
  } catch (BadMethodCallException $e) {
    var_dump($e->getLine());
  }
}

test1(true);
test1(false);
try {
  test2(true);
} catch (Throwable $e) {}
test2(false);
try {
  test3('BadFunctionCallException');
} catch (BadFunctionCallException $e) {}
test3('BadMethodCallException');
try {
  test3('Exception');
} catch (Throwable $e) {}
