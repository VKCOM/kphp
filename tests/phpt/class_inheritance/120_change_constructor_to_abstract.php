@kphp_should_fail
/Cannot make non abstract method B::__construct abstract in class A/
<?php

class A {
    public function __construct() {}
}

abstract class B extends A {
    public abstract function __construct();
}

class D extends B {
    public function __construct() {}
}

$d = new D();
