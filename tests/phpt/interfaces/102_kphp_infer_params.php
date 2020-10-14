@kphp_should_fail
<?php

interface MyInterface {
    /**
     * @param mixed $a
     * @return int
     */
    public function foo($a);
}

class A implements MyInterface {
    /**
     * @param int $a
     * @return int
     */
    public function foo($a) { return 10; }
}

class B implements MyInterface {
    /**
     * @param int $a
     * @return int
     */
    public function foo($a) { return 10; }
}


/** @var MyInterface $m */
$m = new A();
$m = new B();

$m->foo(10);
