@ok
<?php

class Base {
    var $x = 20;
}

class Derived extends Base {
    var $y = "asdf";

    public function __clone() {
        var_dump("CLONE DERIVED");
    }
}

/**
 * @param Base $b
 * @return Base
 */
function foo(Base $b) {
    $d = clone $b;
    if ($d instanceof Derived) {
        var_dump($d->y);
        return $d;
    }

    return $d;
}

$d = foo(new Derived);
var_dump($d instanceof Derived);

$d = foo(new Base);
var_dump($d instanceof Derived);
