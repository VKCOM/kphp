@kphp_should_fail
/Using trait Tr as a type is invalid/
<?php

trait Tr {
    public function foo() {}
}

class B {
    use Tr;
}

class A {
    /**@var Tr*/
    public $tr;

    public function __construct() {
        $tr = new B();
    }
}

$a = new A();
$a->tr = 20;
