@ok
<?php

class MyException extends Exception {}

function get_static_exception(): Exception {
  /** @var Exception $e */
  static $e = null;
  if (!$e) {
    $e = new Exception();
  }
  return $e;
}

for ($i = 0; $i < 3; $i++) {
  try {
    try {
      throw get_static_exception();
    } catch (MyException $e) {
      var_dump([__LINE__ => 'should not catch']);
    }
  } catch (Exception $e2) {
    var_dump([__LINE__ => 'should catch']);
  }
}
