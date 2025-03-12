@ok
<?php

class MyException extends Exception {}
class MyException2 extends MyException {}

try {
  var_dump(__LINE__, 'throwing');
  throw new MyException();
} catch (Exception $e) {
  var_dump([__LINE__ => ($e instanceof Exception)]);
  var_dump([__LINE__ => ($e instanceof MyException)]);
  var_dump([__LINE__ => ($e instanceof MyException2)]);
}

try {
  var_dump(__LINE__, 'throwing');
  throw new MyException2();
} catch (MyException2 $e2) {
  var_dump([__LINE__ => ($e2 instanceof Exception)]);
  var_dump([__LINE__ => ($e2 instanceof MyException)]);
  var_dump([__LINE__ => ($e2 instanceof MyException2)]);
}
