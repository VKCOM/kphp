@ok
<?php

interface WithDefault2 {
    public function foo($x, $y);
}

class B implements WithDefault2 {
    public function foo($x = 4, $y = 10) { var_dump($x, $y); }
}

class A implements WithDefault2 {
    public function foo($x, $y = 20, $z = 10) {
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
