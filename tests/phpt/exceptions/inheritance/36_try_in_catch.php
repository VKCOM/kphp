@ok
<?php

class MyException extends Exception {}

try {
  throw new MyException();
} catch (Exception $e) {
  try {
    throw $e;
  } catch (MyException $e2) {
    var_dump(__LINE__, $e2->getLine(), get_class($e2));
  }
  var_dump(__LINE__, get_class($e));
}
