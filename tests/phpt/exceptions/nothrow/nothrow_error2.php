@kphp_should_fail
/nothrow marked as @kphp-should-not-throw, but really can throw/
<?php

class MyException extends Exception {}
class MyException2 extends MyException {}

/** @kphp-should-not-throw */
function nothrow() {
  try {
    throw new MyException();
  } catch (MyException2 $e) {
  } catch (MyException $e2) {
  }
}

nothrow();
