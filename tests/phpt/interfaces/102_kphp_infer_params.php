@kphp_should_fail
<?php

interface MyInterface {
    /**
     * @kphp-infer
     * @param mixed $a
     * @return int
     */
    public function foo($a);
}

class A implements MyInterface {
    /**
     * @kphp-infer
     * @param int $a
     * @return int
     */
    public function foo($a) { return 10; }
}

class B implements MyInterface {
    /**
     * @kphp-infer
     * @param int $a
     * @return int
     */
    public function foo($a) { return 10; }
}


/** @var MyInterface $m */
$m = new A();
$m = new B();

$m->foo(10);
