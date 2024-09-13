@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

class A {
    public $nm_a = 10;
    /** @var tuple(int, string) */
    public $t_a;
    public $a_a = [1, 2, 3];
    public $s_a = "asdf";
    public $n_a = NULL;
    /** @var tuple(string) | false */
    public $t_or_false_a = false;

    public function __construct() {
        $this->t_a = tuple(10, "asdf");
        $this->t_or_false_a = tuple("asfd");
    }
}

class B {
    public $s_b = "asdf";
    public A $a_b;
    /** @var A[]|false */
    public $arr_a_b = false;
    public function __construct() { 
        $this->a_b  = new A(); 
        $this->arr_a_b = [new A(), new A()];
    }
}


$b = new B();
var_dump(to_array_debug($b));

