@kphp_should_fail
/class A must be abstract: method I::__construct is not overridden/
<?php

interface I {
    public function __construct();
}

class A implements I {
}

$a = new A();
