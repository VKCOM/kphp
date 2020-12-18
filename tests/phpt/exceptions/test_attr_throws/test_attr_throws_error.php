@ok
<?php

class MyError extends Error {}

/**
 * @kphp-test-throws Error
 */
function test() {
  $throw = function(Error $e) {
    throw $e;
  };

  $throw(new MyError());
}

try {
  test();
} catch (Throwable $e) {
  var_dump([__LINE__ => $e->getLine()]);
}
