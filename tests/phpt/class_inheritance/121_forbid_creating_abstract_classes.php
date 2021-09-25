@kphp_should_fail
/Can't instantiate B, it's an abstract class/
/Can't instantiate I, it's an interface/
<?php

abstract class B {
    public function __construct() {}
}

interface I {}
trait T{}

new B;
new I;
