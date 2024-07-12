@ok k2_skip
<?php

#ifndef KPHP
function get_reference_counter($a) {
    return $a->rcnt;
}
#endif

class A { public $rcnt = 1; }

$a = new A();
var_dump(1 == get_reference_counter($a));

$b = $a; $a->rcnt++;
var_dump(2 == get_reference_counter($a));

$c = $a; $a->rcnt++;
var_dump(3 == get_reference_counter($a));

$d = clone $a; $d->rcnt = 1;
var_dump(3 == get_reference_counter($a));
var_dump(1 == get_reference_counter($d));
