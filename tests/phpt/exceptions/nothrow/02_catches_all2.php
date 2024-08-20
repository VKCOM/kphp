@ok k2_skip
<?php

class MyException extends Exception {}

/** @kphp-should-not-throw */
function nothrow() {
  try {
    throw new MyException();
  } catch (Exception $e) {
  } catch (MyException $e2) {
  }
}

nothrow();
