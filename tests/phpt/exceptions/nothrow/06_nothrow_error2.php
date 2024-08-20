@kphp_should_fail k2_skip
/test marked as @kphp-should-not-throw, but really can throw/
<?php

class MyException extends Exception {}
class MyException2 extends MyException {}

function does_throw() {
  throw new MyException();
}

/** @kphp-should-not-throw */
function test($cond) {
  if ($cond) {
    does_throw();
  }
}

test(false);
