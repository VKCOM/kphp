@ok
<?php

// Test that default generated constructors work correctly.

class MyException extends Exception {}
class MyException2 extends MyException {}

$e = new MyException('message', 40);
$e2 = new MyException2('message2', 4);

f($e);
f($e2);

function f(Exception $e) {
  var_dump($e->getLine());
  var_dump(basename($e->getFile()));
  var_dump($e->getMessage());
  var_dump($e->getCode());
}
