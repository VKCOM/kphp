@kphp_should_fail k2_skip
/does_throw marked as @kphp-should-not-throw, but really can throw/
<?php

class MyException extends Exception {}

/** @kphp-should-not-throw */
function does_throw() {
  throw new MyException();
}

does_throw();
