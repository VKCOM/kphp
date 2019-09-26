@kphp_should_fail
/Function result cannot be used as lvalue/
<?php

class B {
  public $b = 0;
}

class A {
    public $x = 10;
    /** @var B */
    public $b;

    function __construct() { $this->b = new B; }

    /** @return B */
    function getB() { return $this->b; }
}

/**
 * @kphp-infer
 * @param A[]|false $a
 */
function run($a) {
    $a[0]->getB() = new B;
    var_dump($a[0]->getB());
}

run(true ? [new A] : false);

