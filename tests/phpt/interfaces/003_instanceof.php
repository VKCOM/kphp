@ok
<?php

require_once 'kphp_tester_include.php';

$a = new Classes\B();
$a = new Classes\A();
$b = new Classes\B();

if ($a instanceof Classes\A || $a instanceof Classes\B) {
    var_dump('$a is A or B');
}

$is_instance = true;
$is_instance &= $a instanceof Classes\A;
var_dump($is_instance);

$value = 10 + $b instanceof Classes\A;
var_dump($value);

interface I1 {
    function f1();
}
interface I2 extends I1 {
    function f2();
}

class A implements I2 {
    function f1() { echo "f1\n"; }
    function f2() { echo "f2\n"; }
    function fa() { echo "fa\n"; }
}

function demo(I1 $o) {
    if ($o instanceof I1) {
        $o->f1();
        if ($o instanceof I2) {
            $o->f2();
            if ($o instanceof A) {
                $o->fa();
            }
        }
    }
}

demo(new A);
