@ok
<?php

class A {
    public $i = 10;
    public function __clone() { var_dump("clone: ".$this->i); }
}


/**
 * @param A $a
 */
function f($a) {
    var_dump($a->i);
}

$a = new A();
f($a);
f(clone $a);

$php_ver = 7;
#ifndef KPHP
$php_ver = (int)phpversion();
#endif

// php5.6 doesn't support this syntax
// var_dump((clone $a)->i);
// 
// class B { public $i = 999; }
// var_dump((clone (new B))->i);
