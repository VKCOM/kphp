@ok
<?php

// Test throwing a pre-instantiated exception.

class MyException1 extends \Exception {}
class MyException1derived extends MyException1 {}

function get_exception(string $msg): Exception {
  return new MyException1derived($msg);
}

$e = get_exception('test');
$cond = $e !== null;
try {
  if ($cond) {
    throw $e;
  }
} catch (MyException1derived $e) {
  var_dump($e->getMessage(), get_class($e));
}
