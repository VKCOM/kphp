@ok k2_skip
<?php

class MyException extends Exception {}

function f() {
  try {
    throw new Exception();
  } catch (MyException $e) {
    var_dump([__LINE__ => $e->getLine()]);
  } catch (Throwable $e) {
    var_dump([__LINE__ => $e->getLine()]);
  }
}

f();
