@ok
<?php

class MyException1 extends Exception {}
class MyException2 extends Exception {}

function do_throw() {
  throw new MyException2();
}

function f1($x) {
  var_dump([__LINE__ => $x]);
  do_throw();
  var_dump([__LINE__ => 'unreachable']);
}

function f2($x) {
  try {
    f1($x + 1);
    var_dump([__LINE__ => 'unreachable']);
  } catch (MyException1 $e) {
    var_dump([__LINE__ => 'should not catch']);
  }
  var_dump([__LINE__ => 'unreachable']);
}

try {
  f2(5);
  var_dump([__LINE__ => 'unreachable']);
} catch (MyException2 $e) {
  var_dump([__LINE__ => 'should catch']);
}
