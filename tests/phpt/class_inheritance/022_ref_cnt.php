@ok k2_skip
<?php

#ifndef KPHP
function get_reference_counter($a) {
    return $a->rcnt;
}
#endif

class A { 
    public $rcnt = 1; 
    public function foo() {}
}

class B extends A {
    public function foo() {}
}

/**@var A*/
$a = new A();
$a = new B();
var_dump(1 == get_reference_counter($a));

$b = new B();
$a = $b; $a->rcnt++;
var_dump(2 == get_reference_counter($a));
var_dump(2 == get_reference_counter($b));

$b = clone $b; $b->rcnt = 1; $a->rcnt = 1;
var_dump(1 == get_reference_counter($a));
var_dump(1 == get_reference_counter($b));

