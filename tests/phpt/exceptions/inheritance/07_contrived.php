@ok
<?php

class ExceptionA extends Exception {}
class ExceptionB extends Exception {}

function f1() {
  for ($i = 0; $i < 3; $i++) {
    try {
      for ($j = 0; $j < 10; $j++) {
        try {
          if ($j) {
            throw new ExceptionA();
          }
        } catch (ExceptionB $e) {
          var_dump([__LINE__ => 'should not catch']);
        }
        var_dump([__LINE__ => $j]);
      }
      var_dump([__LINE__ => 'after the inner loop']);
    } catch (ExceptionA $e2) {
      var_dump([__LINE__ => 'should catch']);
    }
    var_dump([__LINE__ => $i]);
  }
  var_dump([__LINE__ => 'after the outer loop']);
}

function f2() {
  for ($i = 0; $i < 3; $i++) {
    try {
      for ($j = 0; $j < 10; $j++) {
        try {
          if ($j) {
            throw new ExceptionB();
          }
        } catch (ExceptionB $e) {
          var_dump([__LINE__ => 'should catch']);
        }
        var_dump([__LINE__ => $j]);
      }
      var_dump([__LINE__ => 'after the inner loop']);
    } catch (ExceptionA $e2) {
      var_dump([__LINE__ => 'should not catch']);
    }
    var_dump([__LINE__ => $i]);
  }
  var_dump([__LINE__ => 'after the outer loop']);
}

function f3() {
  for ($i = 0; $i < 3; $i++) {
    try {
      for ($j = 0; $j < 10; $j++) {
        try {
          if ($j) {
            throw new ExceptionB();
          }
        } catch (ExceptionA $e) {
          var_dump([__LINE__ => 'should not catch']);
        }
        var_dump([__LINE__ => $j]);
      }
      var_dump([__LINE__ => 'after the inner loop']);
    } catch (ExceptionA $e2) {
      var_dump([__LINE__ => 'should not catch']);
    }
    var_dump([__LINE__ => $i]);
  }
  var_dump([__LINE__ => 'after the outer loop']);
}

function test() {
  f1();
  f2();
  f3();
}

try {
  test();
} catch (Exception $e) {
  var_dump([__LINE__ => 'should catch from f3']);
}
