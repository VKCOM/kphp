@kphp_should_fail
/Can not make non-abstract method A::__construct abstract in class B/
/Can not make non-abstract method A::stf abstract in class D/
<?php

class A {
    public function __construct() {}
    public static function stf() {}
}

abstract class B extends A {
    public abstract function __construct();
}

class D extends B {
    public function __construct() {}
    public abstract static function stf();
}

$d = new D();
B::stf();
