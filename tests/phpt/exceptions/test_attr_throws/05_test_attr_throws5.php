@ok k2_skip
<?php

class MyException extends Exception {}

/**
 * @kphp-throws Exception
 */
function test() {
  $throw = function(Exception $e) {
    throw $e;
  };

  $throw(new MyException());
}

try {
  test();
} catch (Throwable $e) {
  var_dump([__LINE__ => $e->getLine()]);
}
