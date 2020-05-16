@kphp_should_fail
/Cannot instantiate abstract class B/
<?php

abstract class B {
    public function __construct() {}
}

$d = new B();
