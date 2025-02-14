@ok
<?php

class MyException extends Exception {}

/** @kphp-should-not-throw */
function nothrow() {
  try {
    throw new MyException();
  } catch (MyException $e) {
  } catch (Exception $e2) {
  }
}

nothrow();
