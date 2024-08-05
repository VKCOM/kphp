@ok k2_skip
<?php

class MyException1 extends Exception {}
class MyException2 extends Exception {}

function do_throw() {
  throw new MyException2();
}

function f1($x) {
  for ($i = 0; $i < 10; $i++) {
    var_dump([__LINE__ => "i=$i"]);
    try {
      $x++;
      try {
        var_dump([__LINE__ => $x]);
        if ($x) {
          do_throw();
        }
        var_dump([__LINE__ => 'unreachable']);
      } catch (MyException1 $e) {
        var_dump([__LINE__ => 'should not catch']);
      }
      var_dump([__LINE__ => 'unreachable']);
    } catch (MyException1 $e2) {
      var_dump([__LINE__ => 'should not catch']);
    }
  }
  var_dump([__LINE__ => 'unreachable']);
}

function f2($x) {
  while (true) {
    try {
      $x++;
      try {
        f1($x + 1);
        var_dump([__LINE__ => 'unreachable']);
      } catch (Exception $e) {
        var_dump([__LINE__ => 'should catch']);
        return;
      }
      var_dump([__LINE__ => 'unreachable']);
    } catch (Exception $e2) {
      var_dump([__LINE__ => 'should not catch']);
    }
  }
  var_dump([__LINE__ => 'unreachable']);
}

try {
  for ($i = 0; $i < 5; $i++) {
    f2($i);
  }
  var_dump([__LINE__ => 'reached']);
} catch (Exception $e) {
  var_dump([__LINE__ => 'should not catch']);
}
