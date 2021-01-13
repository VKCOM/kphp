@ok
<?php

class MyException extends Exception {}

try {
  throw new MyException();
} catch (MyException $e) {
  var_dump(__LINE__, 'should catch');
} catch (MyException $e2) {
  var_dump(__LINE__, 'effectively a dead code');
}
var_dump(__LINE__);
