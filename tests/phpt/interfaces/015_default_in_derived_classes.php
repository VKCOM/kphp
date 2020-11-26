@ok
<?php

define('AB', 200);

interface WithDefault2 {
    /**
     * @param int $x
     * @param int $y
     */
    public function foo($x, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB);
    /**
     * @param int $x
     * @param int $y
     */
    public function foo2($x, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB);
    /**
     * @param int $x
     * @param int $y
     */
    public function foo12($x, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB);
    /**
     * @param int $x
     * @param int $y
     */
    public function foo22($x, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB);
    /**
     * @param int $x
     * @param int $y
     */
    public function foo3($x, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB);
    /**
     * @param int $x
     * @param int $y
     */
    public function foo32($x, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB);
    /**
     * @param int $x
     * @param int $y
     */
    public function foo312($x, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB);
    /**
     * @param int $x
     * @param int $y
     */
    public function foo322($x, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB);
}

class B implements WithDefault2 {
    /**
     * @param int $x
     * @param int $y
     */
    public function foo($x = 20, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB) { var_dump($x, $y); }
    /**
     * @param int $x
     * @param int $y
     */
    public function foo2($x = 20, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB) { var_dump($x, $y); }
    /**
     * @param int $x
     * @param int $y
     */
    public function foo12($x = 20, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB) { var_dump($x, $y); }
    /**
     * @param int $x
     * @param int $y
     */
    public function foo22($x = 20, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB) { var_dump($x, $y); }
    /**
     * @param int $x
     * @param int $y
     */
    public function foo3($x = 20, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB) { var_dump($x, $y); }
    /**
     * @param int $x
     * @param int $y
     */
    public function foo32($x = 20, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB) { var_dump($x, $y); }
    /**
     * @param int $x
     * @param int $y
     */
    public function foo312($x = 20, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB) { var_dump($x, $y); }
    /**
     * @param int $x
     * @param int $y
     */
    public function foo322($x = 20, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB) { var_dump($x, $y); }
}

class A implements WithDefault2 {
    /**
     * @param int $x
     * @param int $y
     * @param int $z
     */
    public function foo($x, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB, $z = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB) {
        var_dump($x, $y, $z);
    }
    /**
     * @param int $x
     * @param int $y
     * @param int $z
     */
    public function foo2($x, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB, $z = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB) {
        var_dump($x, $y, $z);
    }
    /**
     * @param int $x
     * @param int $y
     * @param int $z
     */
    public function foo12($x, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB, $z = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB) {
        var_dump($x, $y, $z);
    }
    /**
     * @param int $x
     * @param int $y
     * @param int $z
     */
    public function foo22($x, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB, $z = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB) {
        var_dump($x, $y, $z);
    }
    /**
     * @param int $x
     * @param int $y
     * @param int $z
     */
    public function foo3($x, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB, $z = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB) {
        var_dump($x, $y, $z);
    }
    /**
     * @param int $x
     * @param int $y
     * @param int $z
     */
    public function foo32($x, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB, $z = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB) {
        var_dump($x, $y, $z);
    }
    /**
     * @param int $x
     * @param int $y
     * @param int $z
     */
    public function foo312($x, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB, $z = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB) {
        var_dump($x, $y, $z);
    }
    /**
     * @param int $x
     * @param int $y
     * @param int $z
     */
    public function foo322($x, $y = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB, $z = AB + AB + AB * AB + AB + AB + AB + AB * AB + AB + AB + AB + AB * AB + AB * AB + AB + AB * AB + AB + AB + AB + AB * AB + AB) {
        var_dump($x, $y, $z);
    }
}

$b = new B();
$b->foo();

/** @var WithDefault2 $a */
$a = $b;
$a->foo(1, 2);
$a->foo2(1, 2);
$a->foo12(1, 2);
$a->foo22(1, 2);
$a->foo3(1, 2);
$a->foo32(1, 2);
$a->foo312(1, 2);
$a->foo322(1, 2);

$a = new A();
$a->foo(1, 2);
