@kphp_should_fail
/nothrow marked as @kphp-should-not-throw, but really can throw/
<?php

class MyException extends Exception {}

/** @kphp-should-not-throw */
function nothrow() {
  try {
    throw new MyException();
  } catch (MyException $e) {
  }
}

nothrow();
