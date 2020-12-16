@ok
<?php

class MyException extends Exception {}

/**
 * @kphp-test-throws MyException
 */
function test() {
  $throw = function(MyException $e) {
    throw $e;
  };

  $throw(new MyException());
}

try {
  test();
} catch (Throwable $e) {
  var_dump([__LINE__ => $e->getLine()]);
}
