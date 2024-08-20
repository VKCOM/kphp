@ok k2_skip
<?php

// Test that get_class() is virtual and that derived classes can override it.

class MyException extends Exception {}
class MyException2 extends MyException {}

function dump_exception_class(Exception $e) {
  var_dump('dump_exception_class: ' . get_class($e));
}

$e1 = new Exception();
$e2 = new MyException();
$e3 = new MyException2();

var_dump([get_class($e1), get_class($e2), get_class($e3)]);
var_dump([Exception::class, MyException::class, MyException2::class]);
dump_exception_class($e1);
dump_exception_class($e2);
dump_exception_class($e3);
