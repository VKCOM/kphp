@ok
<?php

class A {
    public $x = 10;
    public $y = "abc";

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
