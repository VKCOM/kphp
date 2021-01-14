@ok
<?php

class Exception1 extends Exception {}
class Exception2 extends Exception {}
class Exception3 extends Exception {}

/**
 * @kphp-throws Exception1 Exception2 Exception3
 */
function f1($n) {
    var_dump([__LINE__ => $n]);
    if ($n == 0) {
        return;
    }
    if ($n == -1) {
      throw new Exception1();
    }
    f2($n-1);
}

/**
 * @kphp-throws Exception1 Exception2 Exception3
 */
function f2($n) {
    var_dump([__LINE__ => $n]);
    if ($n == 0) {
        return;
    }
    if ($n == -1) {
      throw new Exception2();
    }
    f3($n);
}

/**
 * @kphp-throws Exception1 Exception2 Exception3
 */
function f3($n) {
    var_dump([__LINE__ => $n]);
    if ($n == 0) {
        return;
    }
    if ($n == -1) {
      throw new Exception3();
    }
    f1($n);
}

f1(3);
