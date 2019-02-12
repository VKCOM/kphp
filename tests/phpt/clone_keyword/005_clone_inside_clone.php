@ok
<?php

class B {
    public $x = 10;
}

class A {
    /** @var B */
    public $b;
    public $i = 100;

    public function __construct() {
        $this->b = new B();
    }

    public function __clone() {
        $this->b = clone $this->b;
    }
}


$a = new A();
$a2 = clone $a;

$a->b->x = 1;
$a2->b->x = 2;

$a->i = 10;
$a2->i = 20;

var_dump($a->b->x);
var_dump($a2->b->x);

var_dump($a->i);
var_dump($a2->i);
