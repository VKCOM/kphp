@ok
<?php

class MyException extends Exception {}

try {
  try {
    throw new Exception();
  } catch (Exception $e) {
    var_dump('ok', __LINE__);
    throw new MyException();
  } catch (MyException $e3) {
    var_dump([__LINE__ => get_class($e3)]);
  }
} catch (MyException $e2) {
  var_dump(get_class($e2));
}
var_dump('ok', __LINE__);
