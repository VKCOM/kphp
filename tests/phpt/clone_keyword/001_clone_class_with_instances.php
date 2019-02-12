@ok
<?php
class B {
    public $i = 10;
    public $u = "abcd";

    /**
     * @kphp-required
     */
    public function __clone() {
        var_dump("clone:: B:: ".$this->i."; ".$this->u);
    }
}


class A {
    /** @var B */
    public $b;
    public $u = [1];

    public function __construct() {
        $this->b = new B();
    }

    public function __clone() {
        var_dump("clone:: A:: ");
        print_r($this->u);
    }
}


$a = new A();
$a2 = clone $a; 

$a->u[0] = 100;
var_dump($a->u);
var_dump($a2->u);

$a->b->i = 90;
var_dump($a2->b->i);

