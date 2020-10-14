@kphp_should_fail
<?php

interface MyInterface2 {
    /**
     * @return int
     */
    public function foo();
}

class A2 implements MyInterface2 {
    /**
     * @return mixed
     */
    public function foo() { return true ? "asdf" : 10; }
}

class B2 implements MyInterface2 {
    /**
     * @return int
     */
    public function foo() { return 10; }
}


/** @var MyInterface2 $m */
$m = new A2();
$m = new B2();

$m->foo();
