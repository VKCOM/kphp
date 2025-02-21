@kphp_should_fail
/It's forbidden to `clone` built-in classes/
<?php

// PHP exceptions are uncloneable.

class MyException extends Exception {}
class AnotherMyException extends MyException {}

$e1 = new Exception();
$e2 = new MyException();
$e3 = new AnotherMyException();

$clone1 = clone $e1; // error
$clone2 = clone $e2; // TODO: error (see #57)
$clone3 = clone $e3; // TODO: error (see #57)
