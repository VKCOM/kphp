@ok k2_skip
KPHP_REQUIRE_FUNCTIONS_TYPING=1
<?php

/** @return string|int[] */
function mixed_result(bool $cond) {
  return $cond ? "string" : [1];
}

try {
  throw new Exception(mixed_result(true));
} catch (Exception $e) {
  var_dump($e->getMessage());
}

try {
  throw new TypeError(mixed_result(true));
} catch (Error $e) {
  var_dump($e->getMessage());
}

class MyException extends RuntimeException {}

try {
  throw new MyException(mixed_result(true));
} catch (MyException $e) {
  var_dump($e->getMessage());
}
