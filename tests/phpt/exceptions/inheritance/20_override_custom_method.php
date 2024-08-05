@ok k2_skip
<?php

class MyException extends Exception {
  public function f() { return 'f1'; }
}

class MyException2 extends MyException {
  public function f() { return 'f2'; }
}

$e = new MyException();
var_dump($e->f(), $e->getLine());

$e2 = new MyException2();
var_dump($e2->f(), $e->getLine());
