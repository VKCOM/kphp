@ok
<?php

define('AB', 200);

interface WithDefault2 {
    /**
     * @param int $x
     * @param int $y
     */
    public function foo($x, $y = 10 + 10 * AB);
}

class B implements WithDefault2 {
    /**
     * @param int $x
     * @param int $y
     */
    public function foo($x = 20, $y = 10 + 10 * AB) { var_dump($x, $y); }
}

class A implements WithDefault2 {
    /**
     * @param int $x
     * @param int $y
     * @param int $z
     */
    public function foo($x, $y = 10 + 10 * AB, $z = 10) {
        var_dump($x, $y, $z);
    }
}

$b = new B();
$b->foo();

/** @var WithDefault2 $a */
$a = $b;
$a->foo(1, 2);

$a = new A();
$a->foo(1, 2);
