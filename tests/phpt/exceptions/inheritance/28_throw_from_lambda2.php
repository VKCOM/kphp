@ok
<?php

class MyException1 extends \Exception {}
class MyException2 extends MyException1 {}

function test() {
  $throw = function(Exception $e) {
    throw $e;
  };

  try {
    try {
      $throw(new MyException1());
    } catch (MyException2 $e1) {
      var_dump(__LINE__, 'should never happen');
    }
  } catch (MyException1 $e2) {
    var_dump(__LINE__, 'ok', $e2->getLine());
  }

  var_dump(__LINE__, 'ok');
}

test();
