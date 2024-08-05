@ok k2_skip
<?php

class MyException1 extends \Exception {}
class MyException2 extends MyException1 {}
class MyException3 extends \Exception {}

function test() {
  $throw = function(Exception $e): int {
    throw $e;
  };

  try {
    try {
      try {
        $throw(new MyException3());
        var_dump(__LINE__, 'unreachable');
      } catch (MyException2 $e1) {
        var_dump(__LINE__, 'should never happen');
      }
    } catch (MyException1 $e2) {
      var_dump(__LINE__, 'should never happen');
    }
  } catch (MyException3 $e3) {
    var_dump(__LINE__, $e3->getLine());
  }

  var_dump(__LINE__, 'ok');
}

test();
