@ok k2_skip
<?php

class MyError extends Error {}

/**
 * @kphp-throws Error
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
