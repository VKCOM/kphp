@ok k2_skip
<?php

class MyException extends Exception {}

function do_throw() {
  throw new MyException();
}

try {
  try {
    throw new Exception();
  } catch (Exception $e) {
    var_dump('ok', __LINE__);
    do_throw();
  }
} catch (MyException $e2) {
  var_dump(get_class($e2));
}
var_dump('ok', __LINE__);
