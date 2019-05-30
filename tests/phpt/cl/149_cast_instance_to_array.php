@ok
<?php

#ifndef KittenPHP
function tuple(...$args) { return $args; }
function instance_to_array($instance) { return (array) $instance; }
#endif

class A {
    public $nm_a = 10;
    public $t_a;
    public $a_a = [1, 2, 3];
    public $s_a = "asdf";
    public $n_a = NULL;
    public $t_or_false_a = false;

    public function __construct() {
        $this->t_a = tuple(10, "asdf");
        $this->t_or_false_a = tuple("asfd");
    }
}

class B {
    public $s_b = "asdf";
    public $a_b;
    public $arr_a_b = false;
    public function __construct() { 
        $this->a_b  = new A(); 
        $this->arr_a_b = [new A(), new A()];
    }
}

$b = new B();
$arr_b = instance_to_array($b);

#ifndef KittenPHP
$arr_b["a_b"] = (array) $arr_b["a_b"];
$arr_b["arr_a_b"] = array_map("instance_to_array", $arr_b["arr_a_b"]);
#endif 

var_dump($arr_b["s_b"]);
var_dump($arr_b["a_b"]["nm_a"]);
var_dump($arr_b["a_b"]["t_a"]);
var_dump($arr_b["arr_a_b"][0]["t_a"]);
var_dump($arr_b["arr_a_b"][0]["t_or_false_a"]);

