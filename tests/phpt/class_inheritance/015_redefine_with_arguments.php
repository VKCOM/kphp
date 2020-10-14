@ok
<?php

class B {
    /**
     * @param int $a
     */
    public function __construct($a) {
        var_dump("B::constructed");
    }

    /**
     * @param int $a
     * @param int $b
     * @param string $c
     */
    public function foo($a, $b = 10, $c = "asdf") {
        var_dump("B::foo", $a, $b, $c);
    }
}

class D extends B {
    public function __construct() {
        var_dump("D::constructed");
    }

    /**
     * @param int $a
     * @param int $b
     * @param string $c
     */
    public function foo($a, $b = 10, $c = "asdf") {
        parent::foo(890123);
        var_dump("D::foo", $a, $b, $c);
    }
}

class D2 extends B {
    /**
     * @param int $a
     * @param int $b
     */
    public function __construct($a, $b) {
        var_dump("D2::constructed");
    }

    /**
     * @param int $a
     * @param int $b
     * @param string $c
     * @param int $d
     */
    public function foo($a, $b = 10, $c = "asdf", $d = 123) {
        parent::foo($a);
        var_dump("D2::foo", $a, $b, $c, $d);
    }
}

/**@var B*/
$b = new B(10);
$b->foo(10);

$b = new D();
$b->foo(20);

$b = new D2(10, 20);
$b->foo(56);


