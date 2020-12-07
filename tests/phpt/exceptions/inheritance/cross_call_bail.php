@ok
<?php

// Test that throwing functions that have a catch are not
// treated as "non throwing" (unless they're catching Exception class).

// We will throw MyException2, but try to catch MyException1.

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
  } catch (MyException1 $e) {
    var_dump([__LINE__ => 'should not catch']);
  }
  var_dump([__LINE__ => 'unreachable']);
}

function f3() {
  f2(60);
  f2(600);
  var_dump([__LINE__ => 'unreachable']);
}

function f4() {
  f3();
  var_dump([__LINE__ => 'unreachable']);
}

function f5() {
  try {
    f4();
    var_dump([__LINE__ => 'unreachable']);
  } catch (MyException1 $e) {
    var_dump([__LINE__ => 'should not catch']);
  }
  var_dump([__LINE__ => 'unreachable']);
}

try {
  f5();
  var_dump([__LINE__ => 'unreachable']);
} catch (MyException2 $e) {
  var_dump([__LINE__ => 'should catch']);
}
