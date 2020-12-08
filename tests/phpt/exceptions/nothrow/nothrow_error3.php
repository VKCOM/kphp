@kphp_should_fail
/nothrow marked as @kphp-should-not-throw, but really can throw/
<?php

class MyException extends Exception {}
class MyException2 extends MyException {}

function does_throw() {
  try {
      throw new MyException();
    } catch (MyException2 $e) {
    } catch (MyException $e2) {
    }
}

/** @kphp-should-not-throw */
function nothrow($cond) {
  if ($cond) {
    does_throw();
  }
}

nothrow(false);
