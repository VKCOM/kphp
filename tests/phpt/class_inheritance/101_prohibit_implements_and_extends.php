@kphp_should_fail
<?php

interface IB {
    public function foo();
}

class B {
    public function foo() {}
}

class D extends B implements IB {
    public function foo() {}
}

$d = new D();
