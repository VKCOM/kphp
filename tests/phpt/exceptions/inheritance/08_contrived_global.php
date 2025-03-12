@ok
<?php

class ExceptionA extends Exception {}
class ExceptionB extends Exception {}

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

try {
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
} catch (Exception $e) {
  var_dump([__LINE__ => 'should catch']);
}

var_dump([__LINE__ => 'ok']);
