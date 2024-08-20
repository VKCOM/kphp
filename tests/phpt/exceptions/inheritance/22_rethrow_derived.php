@ok k2_skip
<?php

class MyException1 extends Exception {}

class MyException2 extends MyException1 {
  public $secret = '';

  public function __construct(string $message, string $secret) {
    parent::__construct($message);
    $this->secret = $secret;
  }
}

function do_throw() {
  $e = new MyException2('a message', 'a secret');
  throw $e;
}

function test() {
  try {
    try {
      try {
        do_throw();
      } catch (Exception $e) {
        var_dump('caught as ' . get_class($e));
        var_dump(__LINE__, $e->getLine());
        throw $e;
      }
    } catch (MyException1 $e2) {
      var_dump('caught as ' . get_class($e2));
      var_dump(__LINE__, $e2->getLine());
      throw $e2;
    }
  } catch (MyException2 $e3) {
    var_dump('caught as ' . get_class($e3));
    var_dump(__LINE__, $e3->getLine());
    var_dump('secret: ' . $e3->secret);
    throw $e3;
  }
}

try {
  test();
} catch (Exception $e) {
  var_dump('caught as ' . get_class($e));
  var_dump(__LINE__, $e->getLine());
}
