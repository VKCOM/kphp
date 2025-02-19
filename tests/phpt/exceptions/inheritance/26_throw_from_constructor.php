@ok
<?php

// Test throwing from constructor.

class MyException1 extends \Exception {}
class MyException2 extends MyException1 {}

class Example {
  public function __construct(Exception $to_throw) {
    if ($to_throw) {
      throw $to_throw;
    }
  }
}

try {
  try {
    $_ = new Example(new Exception());
  } catch (MyException2 $e1) {
    var_dump(__LINE__, 'should never happen');
  }
} catch (Exception $e2) {
  var_dump(__LINE__, 'ok', $e2->getLine());
}

try {
  try {
    $_ = new Example(new MyException1());
  } catch (MyException2 $e1) {
    var_dump(__LINE__, 'should never happen');
  }
} catch (MyException1 $e2) {
  var_dump(__LINE__, 'ok', $e2->getLine());
}

var_dump(__LINE__, 'ok');
