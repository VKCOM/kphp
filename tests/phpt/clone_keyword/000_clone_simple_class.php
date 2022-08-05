@ok
<?php

class A {
    public $x = 10;
    public $y = "abc";

    /**
     * @param int $x
     * @param string $y
     */
    public function __construct($x, $y) {
        $this->x = $x;
        $this->y = $y;
    }
}

$a = new A(90, "uuu");
$b = clone $a;

$a->x += 10;
$b->x += 1000;
var_dump($a->x);
var_dump($a->y);

var_dump($b->x);
var_dump($b->y);


class A2 { public $i = 10; }

function f2() {
    $x = new A2();
    return $x;
}

$a2 = f2();
var_dump((clone $a2)->i);

