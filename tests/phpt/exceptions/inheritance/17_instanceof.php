@ok k2_skip
<?php

// Test that instanceof works as expected for Exception and derived classes.

class MyException extends Exception {}
class MyException2 extends MyException {}

$e = new Exception();
$my_e = new MyException();
$my_e2 = new MyException2();

var_dump(
  $e instanceof Exception,
  $e instanceof MyException,
  $my_e instanceof Exception,
  $my_e instanceof MyException,
  $my_e2 instanceof Exception,
  $my_e2 instanceof MyException,
  $my_e2 instanceof MyException2,
  is_my_exception($e),
  is_my_exception($my_e),
  is_my_exception($my_e2)
);

function is_my_exception(Exception $e) {
  return $e instanceof MyException;
}
