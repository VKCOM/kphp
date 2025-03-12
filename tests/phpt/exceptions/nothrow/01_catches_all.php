@ok
<?php

class MyException extends Exception {}

/** @kphp-should-not-throw */
function nothrow() {
  try {
    throw new MyException();
  } catch (Exception $e) {
  }
}

nothrow();
