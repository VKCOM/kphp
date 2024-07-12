@kphp_should_fail k2_skip
/You cannot override final method: MyException::getLine/
<?php

// Exception methods are "final".

class MyException extends Exception {
  public function getLine(): int {
    return 1212;
  }
}

$e = new MyException();
$e2 = clone $e;
var_dump($e->getLine());
