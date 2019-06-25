@ok
<?php

class Base {
    public $x = 20;
}

class Derived1 extends Base {
}

class Derived2 extends Base {
}

/** @var Base */
$d = true ? new Derived1() : new Derived2();

var_dump($d instanceof Base);
var_dump($d instanceof Derived1);
var_dump($d instanceof Derived2);

var_dump(get_class($d));
