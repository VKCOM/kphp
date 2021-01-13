@ok
<?php

/**
 * @kphp-throws Exception
 */
function test($n) {
  if ($n > 0) {
    test($n - 1);
  }
  throw new Exception();
}

try {
  test(0);
} catch (Exception $e) {}
